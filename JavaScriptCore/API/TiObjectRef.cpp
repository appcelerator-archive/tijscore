/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Kelvin W Sherlock (ksherlock@gmail.com)
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

#include "config.h"
#include "TiObjectRef.h"
#include "TiObjectRefPrivate.h"

#include "APICast.h"
#include "ButterflyInlines.h"
#include "CodeBlock.h"
#include "CopiedSpaceInlines.h"
#include "DateConstructor.h"
#include "ErrorConstructor.h"
#include "FunctionConstructor.h"
#include "Identifier.h"
#include "InitializeThreading.h"
#include "JSAPIWrapperObject.h"
#include "JSArray.h"
#include "JSCallbackConstructor.h"
#include "JSCallbackFunction.h"
#include "JSCallbackObject.h"
#include "TiClassRef.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "JSRetainPtr.h"
#include "JSString.h"
#include "TiValueRef.h"
#include "ObjectConstructor.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "PropertyNameArray.h"
#include "RegExpConstructor.h"

using namespace TI;

TiClassRef TiClassCreate(const TiClassDefinition* definition)
{
    initializeThreading();
    RefPtr<OpaqueTiClass> jsClass = (definition->attributes & kTiClassAttributeNoAutomaticPrototype)
        ? OpaqueTiClass::createNoAutomaticPrototype(definition)
        : OpaqueTiClass::create(definition);
    
    return jsClass.release().leakRef();
}

TiClassRef TiClassRetain(TiClassRef jsClass)
{
    jsClass->ref();
    return jsClass;
}

void TiClassRelease(TiClassRef jsClass)
{
    jsClass->deref();
}

TiObjectRef TiObjectMake(TiContextRef ctx, TiClassRef jsClass, void* data)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    if (!jsClass)
        return toRef(constructEmptyObject(exec));

    JSCallbackObject<JSDestructibleObject>* object = JSCallbackObject<JSDestructibleObject>::create(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackObjectStructure(), jsClass, data);
    if (JSObject* prototype = jsClass->prototype(exec))
        object->setPrototype(exec->vm(), prototype);

    return toRef(object);
}

TiObjectRef TiObjectMakeFunctionWithCallback(TiContextRef ctx, TiStringRef name, TiObjectCallAsFunctionCallback callAsFunction)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    return toRef(JSCallbackFunction::create(exec->vm(), exec->lexicalGlobalObject(), callAsFunction, name ? name->string() : ASCIILiteral("anonymous")));
}

TiObjectRef TiObjectMakeConstructor(TiContextRef ctx, TiClassRef jsClass, TiObjectCallAsConstructorCallback callAsConstructor)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsPrototype = jsClass ? jsClass->prototype(exec) : 0;
    if (!jsPrototype)
        jsPrototype = exec->lexicalGlobalObject()->objectPrototype();

    JSCallbackConstructor* constructor = JSCallbackConstructor::create(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackConstructorStructure(), jsClass, callAsConstructor);
    constructor->putDirect(exec->vm(), exec->propertyNames().prototype, jsPrototype, DontEnum | DontDelete | ReadOnly);
    return toRef(constructor);
}

TiObjectRef TiObjectMakeFunction(TiContextRef ctx, TiStringRef name, unsigned parameterCount, const TiStringRef parameterNames[], TiStringRef body, TiStringRef sourceURL, int startingLineNumber, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    Identifier nameID = name ? name->identifier(&exec->vm()) : Identifier(exec, "anonymous");
    
    MarkedArgumentBuffer args;
    for (unsigned i = 0; i < parameterCount; i++)
        args.append(jsString(exec, parameterNames[i]->string()));
    args.append(jsString(exec, body->string()));

    JSObject* result = constructFunction(exec, exec->lexicalGlobalObject(), args, nameID, sourceURL->string(), TextPosition(OrdinalNumber::fromOneBasedInt(startingLineNumber), OrdinalNumber::first()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    return toRef(result);
}

TiObjectRef TiObjectMakeArray(TiContextRef ctx, size_t argumentCount, const TiValueRef arguments[],  TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* result;
    if (argumentCount) {
        MarkedArgumentBuffer argList;
        for (size_t i = 0; i < argumentCount; ++i)
            argList.append(toJS(exec, arguments[i]));

        result = constructArray(exec, static_cast<ArrayAllocationProfile*>(0), argList);
    } else
        result = constructEmptyArray(exec, 0);

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }

    return toRef(result);
}

TiObjectRef TiObjectMakeDate(TiContextRef ctx, size_t argumentCount, const TiValueRef arguments[],  TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; ++i)
        argList.append(toJS(exec, arguments[i]));

    JSObject* result = constructDate(exec, exec->lexicalGlobalObject(), argList);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }

    return toRef(result);
}

TiObjectRef TiObjectMakeError(TiContextRef ctx, size_t argumentCount, const TiValueRef arguments[],  TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue message = argumentCount ? toJS(exec, arguments[0]) : jsUndefined();
    Structure* errorStructure = exec->lexicalGlobalObject()->errorStructure();
    JSObject* result = ErrorInstance::create(exec, errorStructure, message);

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }

    return toRef(result);
}

TiObjectRef TiObjectMakeRegExp(TiContextRef ctx, size_t argumentCount, const TiValueRef arguments[],  TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; ++i)
        argList.append(toJS(exec, arguments[i]));

    JSObject* result = constructRegExp(exec, exec->lexicalGlobalObject(),  argList);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    
    return toRef(result);
}

TiValueRef TiObjectGetPrototype(TiContextRef ctx, TiObjectRef object)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    return toRef(exec, jsObject->prototype());
}

void TiObjectSetPrototype(TiContextRef ctx, TiObjectRef object, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    TiValue jsValue = toJS(exec, value);

    jsObject->setPrototypeWithCycleCheck(exec, jsValue.isObject() ? jsValue : jsNull());
}

bool TiObjectHasProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    
    return jsObject->hasProperty(exec, propertyName->identifier(&exec->vm()));
}

TiValueRef TiObjectGetProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    TiValue jsValue = jsObject->get(exec, propertyName->identifier(&exec->vm()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return toRef(exec, jsValue);
}

void TiObjectSetProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiPropertyAttributes attributes, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    Identifier name(propertyName->identifier(&exec->vm()));
    TiValue jsValue = toJS(exec, value);

    if (attributes && !jsObject->hasProperty(exec, name)) {
        PropertyDescriptor desc(jsValue, attributes);
        jsObject->methodTable()->defineOwnProperty(jsObject, exec, name, desc, false);
    } else {
        PutPropertySlot slot(jsObject);
        jsObject->methodTable()->put(jsObject, exec, name, jsValue, slot);
    }

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
}

TiValueRef TiObjectGetPropertyAtIndex(TiContextRef ctx, TiObjectRef object, unsigned propertyIndex, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    TiValue jsValue = jsObject->get(exec, propertyIndex);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return toRef(exec, jsValue);
}


void TiObjectSetPropertyAtIndex(TiContextRef ctx, TiObjectRef object, unsigned propertyIndex, TiValueRef value, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    TiValue jsValue = toJS(exec, value);
    
    jsObject->methodTable()->putByIndex(jsObject, exec, propertyIndex, jsValue, false);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
}

bool TiObjectDeleteProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);

    bool result = jsObject->methodTable()->deleteProperty(jsObject, exec, propertyName->identifier(&exec->vm()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

void* TiObjectGetPrivate(TiObjectRef object)
{
    JSObject* jsObject = uncheckedToJS(object);
    
    if (jsObject->inherits(JSCallbackObject<JSGlobalObject>::info()))
        return jsCast<JSCallbackObject<JSGlobalObject>*>(jsObject)->getPrivate();
    if (jsObject->inherits(JSCallbackObject<JSDestructibleObject>::info()))
        return jsCast<JSCallbackObject<JSDestructibleObject>*>(jsObject)->getPrivate();
#if JSC_OBJC_API_ENABLED
    if (jsObject->inherits(JSCallbackObject<JSAPIWrapperObject>::info()))
        return jsCast<JSCallbackObject<JSAPIWrapperObject>*>(jsObject)->getPrivate();
#endif
    
    return 0;
}

bool TiObjectSetPrivate(TiObjectRef object, void* data)
{
    JSObject* jsObject = uncheckedToJS(object);
    
    if (jsObject->inherits(JSCallbackObject<JSGlobalObject>::info())) {
        jsCast<JSCallbackObject<JSGlobalObject>*>(jsObject)->setPrivate(data);
        return true;
    }
    if (jsObject->inherits(JSCallbackObject<JSDestructibleObject>::info())) {
        jsCast<JSCallbackObject<JSDestructibleObject>*>(jsObject)->setPrivate(data);
        return true;
    }
#if JSC_OBJC_API_ENABLED
    if (jsObject->inherits(JSCallbackObject<JSAPIWrapperObject>::info())) {
        jsCast<JSCallbackObject<JSAPIWrapperObject>*>(jsObject)->setPrivate(data);
        return true;
    }
#endif
        
    return false;
}

TiValueRef TiObjectGetPrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* jsObject = toJS(object);
    TiValue result;
    Identifier name(propertyName->identifier(&exec->vm()));
    if (jsObject->inherits(JSCallbackObject<JSGlobalObject>::info()))
        result = jsCast<JSCallbackObject<JSGlobalObject>*>(jsObject)->getPrivateProperty(name);
    else if (jsObject->inherits(JSCallbackObject<JSDestructibleObject>::info()))
        result = jsCast<JSCallbackObject<JSDestructibleObject>*>(jsObject)->getPrivateProperty(name);
#if JSC_OBJC_API_ENABLED
    else if (jsObject->inherits(JSCallbackObject<JSAPIWrapperObject>::info()))
        result = jsCast<JSCallbackObject<JSAPIWrapperObject>*>(jsObject)->getPrivateProperty(name);
#endif
    return toRef(exec, result);
}

bool TiObjectSetPrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* jsObject = toJS(object);
    TiValue jsValue = value ? toJS(exec, value) : TiValue();
    Identifier name(propertyName->identifier(&exec->vm()));
    if (jsObject->inherits(JSCallbackObject<JSGlobalObject>::info())) {
        jsCast<JSCallbackObject<JSGlobalObject>*>(jsObject)->setPrivateProperty(exec->vm(), name, jsValue);
        return true;
    }
    if (jsObject->inherits(JSCallbackObject<JSDestructibleObject>::info())) {
        jsCast<JSCallbackObject<JSDestructibleObject>*>(jsObject)->setPrivateProperty(exec->vm(), name, jsValue);
        return true;
    }
#if JSC_OBJC_API_ENABLED
    if (jsObject->inherits(JSCallbackObject<JSAPIWrapperObject>::info())) {
        jsCast<JSCallbackObject<JSAPIWrapperObject>*>(jsObject)->setPrivateProperty(exec->vm(), name, jsValue);
        return true;
    }
#endif
    return false;
}

bool TiObjectDeletePrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* jsObject = toJS(object);
    Identifier name(propertyName->identifier(&exec->vm()));
    if (jsObject->inherits(JSCallbackObject<JSGlobalObject>::info())) {
        jsCast<JSCallbackObject<JSGlobalObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
    if (jsObject->inherits(JSCallbackObject<JSDestructibleObject>::info())) {
        jsCast<JSCallbackObject<JSDestructibleObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
#if JSC_OBJC_API_ENABLED
    if (jsObject->inherits(JSCallbackObject<JSAPIWrapperObject>::info())) {
        jsCast<JSCallbackObject<JSAPIWrapperObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
#endif
    return false;
}

bool TiObjectIsFunction(TiContextRef, TiObjectRef object)
{
    if (!object)
        return false;
    CallData callData;
    JSCell* cell = toJS(object);
    return cell->methodTable()->getCallData(cell, callData) != CallTypeNone;
}

TiValueRef TiObjectCallAsFunction(TiContextRef ctx, TiObjectRef object, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    if (!object)
        return 0;

    JSObject* jsObject = toJS(object);
    JSObject* jsThisObject = toJS(thisObject);

    if (!jsThisObject)
        jsThisObject = exec->globalThisValue();

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; i++)
        argList.append(toJS(exec, arguments[i]));

    CallData callData;
    CallType callType = jsObject->methodTable()->getCallData(jsObject, callData);
    if (callType == CallTypeNone)
        return 0;

    TiValueRef result = toRef(exec, call(exec, jsObject, callType, callData, jsThisObject, argList));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    return result;
}

bool TiObjectIsConstructor(TiContextRef, TiObjectRef object)
{
    if (!object)
        return false;
    JSObject* jsObject = toJS(object);
    ConstructData constructData;
    return jsObject->methodTable()->getConstructData(jsObject, constructData) != ConstructTypeNone;
}

TiObjectRef TiObjectCallAsConstructor(TiContextRef ctx, TiObjectRef object, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    if (!object)
        return 0;

    JSObject* jsObject = toJS(object);

    ConstructData constructData;
    ConstructType constructType = jsObject->methodTable()->getConstructData(jsObject, constructData);
    if (constructType == ConstructTypeNone)
        return 0;

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; i++)
        argList.append(toJS(exec, arguments[i]));
    TiObjectRef result = toRef(construct(exec, jsObject, constructType, constructData, argList));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        result = 0;
    }
    return result;
}

struct OpaqueTiPropertyNameArray {
    WTF_MAKE_FAST_ALLOCATED;
public:
    OpaqueTiPropertyNameArray(VM* vm)
        : refCount(0)
        , vm(vm)
    {
    }
    
    unsigned refCount;
    VM* vm;
    Vector<JSRetainPtr<TiStringRef>> array;
};

TiPropertyNameArrayRef TiObjectCopyPropertyNames(TiContextRef ctx, TiObjectRef object)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    JSObject* jsObject = toJS(object);
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    VM* vm = &exec->vm();

    TiPropertyNameArrayRef propertyNames = new OpaqueTiPropertyNameArray(vm);
    PropertyNameArray array(vm);
    jsObject->methodTable()->getPropertyNames(jsObject, exec, array, ExcludeDontEnumProperties);

    size_t size = array.size();
    propertyNames->array.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i)
        propertyNames->array.uncheckedAppend(JSRetainPtr<TiStringRef>(Adopt, OpaqueTiString::create(array[i].string()).leakRef()));
    
    return TiPropertyNameArrayRetain(propertyNames);
}

TiPropertyNameArrayRef TiPropertyNameArrayRetain(TiPropertyNameArrayRef array)
{
    ++array->refCount;
    return array;
}

void TiPropertyNameArrayRelease(TiPropertyNameArrayRef array)
{
    if (--array->refCount == 0) {
        APIEntryShim entryShim(array->vm, false);
        delete array;
    }
}

size_t TiPropertyNameArrayGetCount(TiPropertyNameArrayRef array)
{
    return array->array.size();
}

TiStringRef TiPropertyNameArrayGetNameAtIndex(TiPropertyNameArrayRef array, size_t index)
{
    return array->array[static_cast<unsigned>(index)].get();
}

void TiPropertyNameAccumulatorAddName(TiPropertyNameAccumulatorRef array, TiStringRef propertyName)
{
    PropertyNameArray* propertyNames = toJS(array);
    APIEntryShim entryShim(propertyNames->vm());
    propertyNames->add(propertyName->identifier(propertyNames->vm()));
}
