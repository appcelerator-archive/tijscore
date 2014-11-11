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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#import "APICast.h"
#import "APIShims.h"
#import "TiContextInternal.h"
#import "JSGlobalObject.h"
#import "TiValueInternal.h"
#import "TiVirtualMachineInternal.h"
#import "JSWrapperMap.h"
#import "TiCore.h"
#import "ObjcRuntimeExtras.h"
#import "Operations.h"
#import "StrongInlines.h"
#import <wtf/HashSet.h>

#if JSC_OBJC_API_ENABLED

@implementation TiContext {
    TiVirtualMachine *m_virtualMachine;
    TiGlobalContextRef m_context;
    JSWrapperMap *m_wrapperMap;
    TI::Strong<TI::JSObject> m_exception;
}

@synthesize exceptionHandler;

- (TiGlobalContextRef)TiGlobalContextRef
{
    return m_context;
}

- (instancetype)init
{
    return [self initWithVirtualMachine:[[[TiVirtualMachine alloc] init] autorelease]];
}

- (instancetype)initWithVirtualMachine:(TiVirtualMachine *)virtualMachine
{
    self = [super init];
    if (!self)
        return nil;

    m_virtualMachine = [virtualMachine retain];
    m_context = TiGlobalContextCreateInGroup(getGroupFromVirtualMachine(virtualMachine), 0);
    m_wrapperMap = [[JSWrapperMap alloc] initWithContext:self];

    self.exceptionHandler = ^(TiContext *context, TiValue *exceptionValue) {
        context.exception = exceptionValue;
    };

    [m_virtualMachine addContext:self forGlobalContextRef:m_context];

    return self;
}

- (void)dealloc
{
    [m_wrapperMap release];
    TiGlobalContextRelease(m_context);
    [m_virtualMachine release];
    [self.exceptionHandler release];
    [super dealloc];
}

- (TiValue *)evaluateScript:(NSString *)script
{
    TiValueRef exceptionValue = 0;
    TiStringRef scriptJS = TiStringCreateWithCFString((CFStringRef)script);
    TiValueRef result = TiEvalScript(m_context, scriptJS, 0, 0, 0, &exceptionValue);
    TiStringRelease(scriptJS);

    if (exceptionValue)
        return [self valueFromNotifyException:exceptionValue];

    return [TiValue valueWithTiValueRef:result inContext:self];
}

- (void)setException:(TiValue *)value
{
    if (value)
        m_exception.set(toJS(m_context)->vm(), toJS(TiValueToObject(m_context, valueInternalValue(value), 0)));
    else
        m_exception.clear();
}

- (TiValue *)exception
{
    if (!m_exception)
        return nil;
    return [TiValue valueWithTiValueRef:toRef(m_exception.get()) inContext:self];
}

- (JSWrapperMap *)wrapperMap
{
    return m_wrapperMap;
}

- (TiValue *)globalObject
{
    return [TiValue valueWithTiValueRef:TiContextGetGlobalObject(m_context) inContext:self];
}

+ (TiContext *)currentContext
{
    WTFThreadData& threadData = wtfThreadData();
    CallbackData *entry = (CallbackData *)threadData.m_apiData;
    return entry ? entry->context : nil;
}

+ (TiValue *)currentThis
{
    WTFThreadData& threadData = wtfThreadData();
    CallbackData *entry = (CallbackData *)threadData.m_apiData;
    if (!entry)
        return nil;
    return [TiValue valueWithTiValueRef:entry->thisValue inContext:[TiContext currentContext]];
}

+ (NSArray *)currentArguments
{
    WTFThreadData& threadData = wtfThreadData();
    CallbackData *entry = (CallbackData *)threadData.m_apiData;

    if (!entry)
        return nil;

    if (!entry->currentArguments) {
        TiContext *context = [TiContext currentContext];
        size_t count = entry->argumentCount;
        TiValue * argumentArray[count];
        for (size_t i =0; i < count; ++i)
            argumentArray[i] = [TiValue valueWithTiValueRef:entry->arguments[i] inContext:context];
        entry->currentArguments = [[NSArray alloc] initWithObjects:argumentArray count:count];
    }

    return entry->currentArguments;
}

- (TiVirtualMachine *)virtualMachine
{
    return m_virtualMachine;
}

- (NSString *)name
{
    TiStringRef name = TiGlobalContextCopyName(m_context);
    if (!name)
        return nil;

    return [(NSString *)TiStringCopyCFString(kCFAllocatorDefault, name) autorelease];
}

- (void)setName:(NSString *)name
{
    TiStringRef nameJS = TiStringCreateWithCFString((CFStringRef)[name copy]);
    TiGlobalContextSetName(m_context, nameJS);
    TiStringRelease(nameJS);
}

@end

@implementation TiContext(SubscriptSupport)

- (TiValue *)objectForKeyedSubscript:(id)key
{
    return [self globalObject][key];
}

- (void)setObject:(id)object forKeyedSubscript:(NSObject <NSCopying> *)key
{
    [self globalObject][key] = object;
}

@end

@implementation TiContext (Internal)

- (instancetype)initWithGlobalContextRef:(TiGlobalContextRef)context
{
    self = [super init];
    if (!self)
        return nil;

    TI::JSGlobalObject* globalObject = toJS(context)->lexicalGlobalObject();
    m_virtualMachine = [[TiVirtualMachine virtualMachineWithContextGroupRef:toRef(&globalObject->vm())] retain];
    ASSERT(m_virtualMachine);
    m_context = TiGlobalContextRetain(context);
    m_wrapperMap = [[JSWrapperMap alloc] initWithContext:self];

    self.exceptionHandler = ^(TiContext *context, TiValue *exceptionValue) {
        context.exception = exceptionValue;
    };

    [m_virtualMachine addContext:self forGlobalContextRef:m_context];

    return self;
}

- (void)notifyException:(TiValueRef)exceptionValue
{
    self.exceptionHandler(self, [TiValue valueWithTiValueRef:exceptionValue inContext:self]);
}

- (TiValue *)valueFromNotifyException:(TiValueRef)exceptionValue
{
    [self notifyException:exceptionValue];
    return [TiValue valueWithUndefinedInContext:self];
}

- (BOOL)boolFromNotifyException:(TiValueRef)exceptionValue
{
    [self notifyException:exceptionValue];
    return NO;
}

- (void)beginCallbackWithData:(CallbackData *)callbackData thisValue:(TiValueRef)thisValue argumentCount:(size_t)argumentCount arguments:(const TiValueRef *)arguments
{
    WTFThreadData& threadData = wtfThreadData();
    [self retain];
    CallbackData *prevStack = (CallbackData *)threadData.m_apiData;
    *callbackData = (CallbackData){ prevStack, self, [self.exception retain], thisValue, argumentCount, arguments, nil };
    threadData.m_apiData = callbackData;
    self.exception = nil;
}

- (void)endCallbackWithData:(CallbackData *)callbackData
{
    WTFThreadData& threadData = wtfThreadData();
    self.exception = callbackData->preservedException;
    [callbackData->preservedException release];
    [callbackData->currentArguments release];
    threadData.m_apiData = callbackData->next;
    [self release];
}

- (TiValue *)wrapperForObjCObject:(id)object
{
    // Lock access to m_wrapperMap
    TI::JSLockHolder lock(toJS(m_context));
    return [m_wrapperMap jsWrapperForObject:object];
}

- (TiValue *)wrapperForJSObject:(TiValueRef)value
{
    TI::JSLockHolder lock(toJS(m_context));
    return [m_wrapperMap objcWrapperForTiValueRef:value];
}

+ (TiContext *)contextWithTiGlobalContextRef:(TiGlobalContextRef)globalContext
{
    TiVirtualMachine *virtualMachine = [TiVirtualMachine virtualMachineWithContextGroupRef:toRef(&toJS(globalContext)->vm())];
    TiContext *context = [virtualMachine contextForGlobalContextRef:globalContext];
    if (!context)
        context = [[[TiContext alloc] initWithGlobalContextRef:globalContext] autorelease];
    return context;
}

@end

WeakContextRef::WeakContextRef(TiContext *context)
{
    objc_initWeak(&m_weakContext, context);
}

WeakContextRef::~WeakContextRef()
{
    objc_destroyWeak(&m_weakContext);
}

TiContext * WeakContextRef::get()
{
    return objc_loadWeak(&m_weakContext);
}

void WeakContextRef::set(TiContext *context)
{
    objc_storeWeak(&m_weakContext, context);
}

#endif
