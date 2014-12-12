/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#import "config.h"
#import "TiManagedValue.h"

#if JSC_OBJC_API_ENABLED

#import "APICast.h"
#import "Heap.h"
#import "TiContextInternal.h"
#import "TiValueInternal.h"
#import "Weak.h"
#import "WeakHandleOwner.h"
#import "ObjcRuntimeExtras.h"
#import "Operations.h"

class TiManagedValueHandleOwner : public TI::WeakHandleOwner {
public:
    virtual void finalize(TI::Handle<TI::Unknown>, void* context) OVERRIDE;
    virtual bool isReachableFromOpaqueRoots(TI::Handle<TI::Unknown>, void* context, TI::SlotVisitor&) OVERRIDE;
};

static TiManagedValueHandleOwner* managedValueHandleOwner()
{
    DEFINE_STATIC_LOCAL(TiManagedValueHandleOwner, jsManagedValueHandleOwner, ());
    return &jsManagedValueHandleOwner;
}

class WeakValueRef {
public:
    WeakValueRef()
        : m_tag(NotSet)
    {
    }

    ~WeakValueRef()
    {
        clear();
    }

    void clear()
    {
        switch (m_tag) {
        case NotSet:
            return;
        case Primitive:
            u.m_primitive = TI::TiValue();
            return;
        case Object:
            u.m_object.clear();
            return;
        case String:
            u.m_string.clear();
            return;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    bool isClear() const
    {
        switch (m_tag) {
        case NotSet:
            return true;
        case Primitive:
            return !u.m_primitive;
        case Object:
            return !u.m_object;
        case String:
            return !u.m_string;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    bool isSet() const { return m_tag != NotSet; }
    bool isPrimitive() const { return m_tag == Primitive; }
    bool isObject() const { return m_tag == Object; }
    bool isString() const { return m_tag == String; }

    void setPrimitive(TI::TiValue primitive)
    {
        ASSERT(!isSet());
        ASSERT(!u.m_primitive);
        ASSERT(primitive.isPrimitive());
        m_tag = Primitive;
        u.m_primitive = primitive;
    }

    void setObject(TI::JSObject* object, void* context)
    {
        ASSERT(!isSet());
        ASSERT(!u.m_object);
        m_tag = Object;
        TI::Weak<TI::JSObject> weak(object, managedValueHandleOwner(), context);
        u.m_object.swap(weak);
    }

    void setString(TI::JSString* string, void* context)
    {
        ASSERT(!isSet());
        ASSERT(!u.m_object);
        m_tag = String;
        TI::Weak<TI::JSString> weak(string, managedValueHandleOwner(), context);
        u.m_string.swap(weak);
    }

    TI::JSObject* object()
    {
        ASSERT(isObject());
        return u.m_object.get();
    }

    TI::TiValue primitive()
    {
        ASSERT(isPrimitive());
        return u.m_primitive;
    }

    TI::JSString* string()
    {
        ASSERT(isString());
        return u.m_string.get();
    }

private:
    enum WeakTypeTag { NotSet, Primitive, Object, String };
    WeakTypeTag m_tag;
    union WeakValueUnion {
    public:
        WeakValueUnion ()
            : m_primitive(TI::TiValue())
        {
        }

        ~WeakValueUnion()
        {
            ASSERT(!m_primitive);
        }

        TI::TiValue m_primitive;
        TI::Weak<TI::JSObject> m_object;
        TI::Weak<TI::JSString> m_string;
    } u;
};

@implementation TiManagedValue {
    TI::Weak<TI::JSGlobalObject> m_globalObject;
    WeakValueRef m_weakValue;
}

+ (TiManagedValue *)managedValueWithValue:(TiValue *)value
{
    return [[[self alloc] initWithValue:value] autorelease];
}

- (instancetype)init
{
    return [self initWithValue:nil];
}

- (instancetype)initWithValue:(TiValue *)value
{
    self = [super init];
    if (!self)
        return nil;
    
    if (!value)
        return self;

    TI::ExecState* exec = toJS([value.context TiGlobalContextRef]);
    TI::JSGlobalObject* globalObject = exec->lexicalGlobalObject();
    TI::Weak<TI::JSGlobalObject> weak(globalObject, managedValueHandleOwner(), self);
    m_globalObject.swap(weak);

    TI::TiValue jsValue = toJS(exec, [value TiValueRef]);
    if (jsValue.isObject())
        m_weakValue.setObject(TI::jsCast<TI::JSObject*>(jsValue.asCell()), self);
    else if (jsValue.isString())
        m_weakValue.setString(TI::jsCast<TI::JSString*>(jsValue.asCell()), self);
    else
        m_weakValue.setPrimitive(jsValue);
    return self;
}

- (TiValue *)value
{
    if (!m_globalObject)
        return nil;
    if (m_weakValue.isClear())
        return nil;
    TI::ExecState* exec = m_globalObject->globalExec();
    TiContext *context = [TiContext contextWithTiGlobalContextRef:toGlobalRef(exec)];
    TI::TiValue value;
    if (m_weakValue.isPrimitive())
        value = m_weakValue.primitive();
    else if (m_weakValue.isString())
        value = m_weakValue.string();
    else
        value = m_weakValue.object();
    return [TiValue valueWithTiValueRef:toRef(exec, value) inContext:context];
}

- (void)disconnectValue
{
    m_globalObject.clear();
    m_weakValue.clear();
}

@end

@interface TiManagedValue (PrivateMethods)
- (void)disconnectValue;
@end

bool TiManagedValueHandleOwner::isReachableFromOpaqueRoots(TI::Handle<TI::Unknown>, void* context, TI::SlotVisitor& visitor)
{
    TiManagedValue *managedValue = static_cast<TiManagedValue *>(context);
    return visitor.containsOpaqueRoot(managedValue);
}

void TiManagedValueHandleOwner::finalize(TI::Handle<TI::Unknown>, void* context)
{
    TiManagedValue *managedValue = static_cast<TiManagedValue *>(context);
    [managedValue disconnectValue];
}

#endif // JSC_OBJC_API_ENABLED
