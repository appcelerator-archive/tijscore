/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef JSCallbackObject_h
#define JSCallbackObject_h

#include "TiObjectRef.h"
#include "TiValueRef.h"
#include "JSObject.h"
#include <wtf/PassOwnPtr.h>

namespace TI {

struct JSCallbackObjectData : WeakHandleOwner {
    JSCallbackObjectData(void* privateData, TiClassRef jsClass)
        : privateData(privateData)
        , jsClass(jsClass)
    {
        TiClassRetain(jsClass);
    }
    
    virtual ~JSCallbackObjectData()
    {
        TiClassRelease(jsClass);
    }
    
    TiValue getPrivateProperty(const Identifier& propertyName) const
    {
        if (!m_privateProperties)
            return TiValue();
        return m_privateProperties->getPrivateProperty(propertyName);
    }
    
    void setPrivateProperty(VM& vm, JSCell* owner, const Identifier& propertyName, TiValue value)
    {
        if (!m_privateProperties)
            m_privateProperties = adoptPtr(new JSPrivatePropertyMap);
        m_privateProperties->setPrivateProperty(vm, owner, propertyName, value);
    }
    
    void deletePrivateProperty(const Identifier& propertyName)
    {
        if (!m_privateProperties)
            return;
        m_privateProperties->deletePrivateProperty(propertyName);
    }

    void visitChildren(SlotVisitor& visitor)
    {
        if (!m_privateProperties)
            return;
        m_privateProperties->visitChildren(visitor);
    }

    void* privateData;
    TiClassRef jsClass;
    struct JSPrivatePropertyMap {
        TiValue getPrivateProperty(const Identifier& propertyName) const
        {
            PrivatePropertyMap::const_iterator location = m_propertyMap.find(propertyName.impl());
            if (location == m_propertyMap.end())
                return TiValue();
            return location->value.get();
        }
        
        void setPrivateProperty(VM& vm, JSCell* owner, const Identifier& propertyName, TiValue value)
        {
            WriteBarrier<Unknown> empty;
            m_propertyMap.add(propertyName.impl(), empty).iterator->value.set(vm, owner, value);
        }
        
        void deletePrivateProperty(const Identifier& propertyName)
        {
            m_propertyMap.remove(propertyName.impl());
        }

        void visitChildren(SlotVisitor& visitor)
        {
            for (PrivatePropertyMap::iterator ptr = m_propertyMap.begin(); ptr != m_propertyMap.end(); ++ptr) {
                if (ptr->value)
                    visitor.append(&ptr->value);
            }
        }

    private:
        typedef HashMap<RefPtr<StringImpl>, WriteBarrier<Unknown>, IdentifierRepHash> PrivatePropertyMap;
        PrivatePropertyMap m_propertyMap;
    };
    OwnPtr<JSPrivatePropertyMap> m_privateProperties;
    virtual void finalize(Handle<Unknown>, void*) OVERRIDE;
};

    
template <class Parent>
class JSCallbackObject : public Parent {
protected:
    JSCallbackObject(ExecState*, Structure*, TiClassRef, void* data);
    JSCallbackObject(VM&, TiClassRef, Structure*);

    void finishCreation(ExecState*);
    void finishCreation(VM&);

public:
    typedef Parent Base;

    static JSCallbackObject* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, TiClassRef classRef, void* data)
    {
        ASSERT_UNUSED(globalObject, !structure->globalObject() || structure->globalObject() == globalObject);
        JSCallbackObject* callbackObject = new (NotNull, allocateCell<JSCallbackObject>(*exec->heap())) JSCallbackObject(exec, structure, classRef, data);
        callbackObject->finishCreation(exec);
        return callbackObject;
    }
    static JSCallbackObject<Parent>* create(VM&, TiClassRef, Structure*);

    static const bool needsDestruction;
    static void destroy(JSCell* cell)
    {
        static_cast<JSCallbackObject*>(cell)->JSCallbackObject::~JSCallbackObject();
    }

    void setPrivate(void* data);
    void* getPrivate();

    DECLARE_INFO;

    TiClassRef classRef() const { return m_callbackObjectData->jsClass; }
    bool inherits(TiClassRef) const;

    static Structure* createStructure(VM&, JSGlobalObject*, TiValue);
    
    TiValue getPrivateProperty(const Identifier& propertyName) const
    {
        return m_callbackObjectData->getPrivateProperty(propertyName);
    }
    
    void setPrivateProperty(VM& vm, const Identifier& propertyName, TiValue value)
    {
        m_callbackObjectData->setPrivateProperty(vm, this, propertyName, value);
    }
    
    void deletePrivateProperty(const Identifier& propertyName)
    {
        m_callbackObjectData->deletePrivateProperty(propertyName);
    }

    using Parent::methodTable;

protected:
    static const unsigned StructureFlags = ProhibitsPropertyCaching | OverridesGetOwnPropertySlot | InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | ImplementsHasInstance | OverridesHasInstance | OverridesVisitChildren | OverridesGetPropertyNames | Parent::StructureFlags;

private:
    static String className(const JSObject*);

    static TiValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);

    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSObject*, ExecState*, unsigned propertyName, PropertySlot&);
    
    static void put(JSCell*, ExecState*, PropertyName, TiValue, PutPropertySlot&);
    static void putByIndex(JSCell*, ExecState*, unsigned, TiValue, bool shouldThrow);

    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned);

    static bool customHasInstance(JSObject*, ExecState*, TiValue);

    static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);

    static ConstructType getConstructData(JSCell*, ConstructData&);
    static CallType getCallData(JSCell*, CallData&);

    static void visitChildren(JSCell* cell, SlotVisitor& visitor)
    {
        JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
        ASSERT_GC_OBJECT_INHERITS((static_cast<Parent*>(thisObject)), JSCallbackObject<Parent>::info());
        COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
        ASSERT(thisObject->Parent::structure()->typeInfo().overridesVisitChildren());
        Parent::visitChildren(thisObject, visitor);
        thisObject->m_callbackObjectData->visitChildren(visitor);
    }

    void init(ExecState*);
 
    static JSCallbackObject* asCallbackObject(TiValue);
    static JSCallbackObject* asCallbackObject(EncodedTiValue);
 
    static EncodedTiValue JSC_HOST_CALL call(ExecState*);
    static EncodedTiValue JSC_HOST_CALL construct(ExecState*);
   
    TiValue getStaticValue(ExecState*, PropertyName);
    static EncodedTiValue staticFunctionGetter(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
    static EncodedTiValue callbackGetter(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);

    OwnPtr<JSCallbackObjectData> m_callbackObjectData;
};

} // namespace TI

// include the actual template class implementation
#include "JSCallbackObjectFunctions.h"

#endif // JSCallbackObject_h
