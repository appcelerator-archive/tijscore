/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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
#include "CodeBlock.h"
#include "DateConstructor.h"
#include "ErrorConstructor.h"
#include "FunctionConstructor.h"
#include "Identifier.h"
#include "InitializeThreading.h"
#include "TiArray.h"
#include "TiCallbackConstructor.h"
#include "TiCallbackFunction.h"
#include "TiCallbackObject.h"
#include "TiClassRef.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "TiObject.h"
#include "TiRetainPtr.h"
#include "TiString.h"
#include "TiValueRef.h"
#include "ObjectPrototype.h"
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    if (!jsClass)
        return toRef(constructEmptyObject(exec));

    TiCallbackObject<TiObjectWithGlobalObject>* object = new (exec) TiCallbackObject<TiObjectWithGlobalObject>(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackObjectStructure(), jsClass, data);
    if (TiObject* prototype = jsClass->prototype(exec))
        object->setPrototype(exec->globalData(), prototype);

    return toRef(object);
}

TiObjectRef TiObjectMakeFunctionWithCallback(TiContextRef ctx, TiStringRef name, TiObjectCallAsFunctionCallback callAsFunction)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    Identifier nameID = name ? name->identifier(&exec->globalData()) : Identifier(exec, "anonymous");
    
    return toRef(new (exec) TiCallbackFunction(exec, exec->lexicalGlobalObject(), callAsFunction, nameID));
}

TiObjectRef TiObjectMakeConstructor(TiContextRef ctx, TiClassRef jsClass, TiObjectCallAsConstructorCallback callAsConstructor)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsPrototype = jsClass ? jsClass->prototype(exec) : 0;
    if (!jsPrototype)
        jsPrototype = exec->lexicalGlobalObject()->objectPrototype();

    TiCallbackConstructor* constructor = new (exec) TiCallbackConstructor(exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackConstructorStructure(), jsClass, callAsConstructor);
    constructor->putDirect(exec->globalData(), exec->propertyNames().prototype, jsPrototype, DontEnum | DontDelete | ReadOnly);
    return toRef(constructor);
}

TiObjectRef TiObjectMakeFunction(TiContextRef ctx, TiStringRef name, unsigned parameterCount, const TiStringRef parameterNames[], TiStringRef body, TiStringRef sourceURL, int startingLineNumber, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    Identifier nameID = name ? name->identifier(&exec->globalData()) : Identifier(exec, "anonymous");
    
    MarkedArgumentBuffer args;
    for (unsigned i = 0; i < parameterCount; i++)
        args.append(jsString(exec, parameterNames[i]->ustring()));
    args.append(jsString(exec, body->ustring()));

    TiObject* result = constructFunction(exec, exec->lexicalGlobalObject(), args, nameID, sourceURL->ustring(), startingLineNumber);
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* result;
    if (argumentCount) {
        MarkedArgumentBuffer argList;
        for (size_t i = 0; i < argumentCount; ++i)
            argList.append(toJS(exec, arguments[i]));

        result = constructArray(exec, argList);
    } else
        result = constructEmptyArray(exec);

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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; ++i)
        argList.append(toJS(exec, arguments[i]));

    TiObject* result = constructDate(exec, exec->lexicalGlobalObject(), argList);
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue message = argumentCount ? toJS(exec, arguments[0]) : jsUndefined();
    Structure* errorStructure = exec->lexicalGlobalObject()->errorStructure();
    TiObject* result = ErrorInstance::create(exec, errorStructure, message);

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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; ++i)
        argList.append(toJS(exec, arguments[i]));

    TiObject* result = constructRegExp(exec, exec->lexicalGlobalObject(),  argList);
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);
    return toRef(exec, jsObject->prototype());
}

void TiObjectSetPrototype(TiContextRef ctx, TiObjectRef object, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);
    TiValue jsValue = toJS(exec, value);

    jsObject->setPrototypeWithCycleCheck(exec->globalData(), jsValue.isObject() ? jsValue : jsNull());
}

bool TiObjectHasProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);
    
    return jsObject->hasProperty(exec, propertyName->identifier(&exec->globalData()));
}

TiValueRef TiObjectGetProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);

    TiValue jsValue = jsObject->get(exec, propertyName->identifier(&exec->globalData()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return toRef(exec, jsValue);
}

void TiObjectSetProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiPropertyAttributes attributes, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);
    Identifier name(propertyName->identifier(&exec->globalData()));
    TiValue jsValue = toJS(exec, value);

    if (attributes && !jsObject->hasProperty(exec, name))
        jsObject->putWithAttributes(exec, name, jsValue, attributes);
    else {
        PutPropertySlot slot;
        jsObject->put(exec, name, jsValue, slot);
    }

    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
}

TiValueRef TiObjectGetPropertyAtIndex(TiContextRef ctx, TiObjectRef object, unsigned propertyIndex, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);

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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);
    TiValue jsValue = toJS(exec, value);
    
    jsObject->put(exec, propertyIndex, jsValue);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
}

bool TiObjectDeleteProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);

    bool result = jsObject->deleteProperty(exec, propertyName->identifier(&exec->globalData()));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

void* TiObjectGetPrivate(TiObjectRef object)
{
    TiObject* jsObject = toJS(object);
    
    if (jsObject->inherits(&TiCallbackObject<TiGlobalObject>::s_info))
        return static_cast<TiCallbackObject<TiGlobalObject>*>(jsObject)->getPrivate();
    if (jsObject->inherits(&TiCallbackObject<TiObjectWithGlobalObject>::s_info))
        return static_cast<TiCallbackObject<TiObjectWithGlobalObject>*>(jsObject)->getPrivate();
    
    return 0;
}

bool TiObjectSetPrivate(TiObjectRef object, void* data)
{
    TiObject* jsObject = toJS(object);
    
    if (jsObject->inherits(&TiCallbackObject<TiGlobalObject>::s_info)) {
        static_cast<TiCallbackObject<TiGlobalObject>*>(jsObject)->setPrivate(data);
        return true;
    }
    if (jsObject->inherits(&TiCallbackObject<TiObjectWithGlobalObject>::s_info)) {
        static_cast<TiCallbackObject<TiObjectWithGlobalObject>*>(jsObject)->setPrivate(data);
        return true;
    }
        
    return false;
}

TiValueRef TiObjectGetPrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    TiObject* jsObject = toJS(object);
    TiValue result;
    Identifier name(propertyName->identifier(&exec->globalData()));
    if (jsObject->inherits(&TiCallbackObject<TiGlobalObject>::s_info))
        result = static_cast<TiCallbackObject<TiGlobalObject>*>(jsObject)->getPrivateProperty(name);
    else if (jsObject->inherits(&TiCallbackObject<TiObjectWithGlobalObject>::s_info))
        result = static_cast<TiCallbackObject<TiObjectWithGlobalObject>*>(jsObject)->getPrivateProperty(name);
    return toRef(exec, result);
}

bool TiObjectSetPrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    TiObject* jsObject = toJS(object);
    TiValue jsValue = value ? toJS(exec, value) : TiValue();
    Identifier name(propertyName->identifier(&exec->globalData()));
    if (jsObject->inherits(&TiCallbackObject<TiGlobalObject>::s_info)) {
        static_cast<TiCallbackObject<TiGlobalObject>*>(jsObject)->setPrivateProperty(exec->globalData(), name, jsValue);
        return true;
    }
    if (jsObject->inherits(&TiCallbackObject<TiObjectWithGlobalObject>::s_info)) {
        static_cast<TiCallbackObject<TiObjectWithGlobalObject>*>(jsObject)->setPrivateProperty(exec->globalData(), name, jsValue);
        return true;
    }
    return false;
}

bool TiObjectDeletePrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    TiObject* jsObject = toJS(object);
    Identifier name(propertyName->identifier(&exec->globalData()));
    if (jsObject->inherits(&TiCallbackObject<TiGlobalObject>::s_info)) {
        static_cast<TiCallbackObject<TiGlobalObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
    if (jsObject->inherits(&TiCallbackObject<TiObjectWithGlobalObject>::s_info)) {
        static_cast<TiCallbackObject<TiObjectWithGlobalObject>*>(jsObject)->deletePrivateProperty(name);
        return true;
    }
    return false;
}

bool TiObjectIsFunction(TiContextRef, TiObjectRef object)
{
    CallData callData;
    return toJS(object)->getCallData(callData) != CallTypeNone;
}

TiValueRef TiObjectCallAsFunction(TiContextRef ctx, TiObjectRef object, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);
    TiObject* jsThisObject = toJS(thisObject);

    if (!jsThisObject)
        jsThisObject = exec->globalThisValue();

    MarkedArgumentBuffer argList;
    for (size_t i = 0; i < argumentCount; i++)
        argList.append(toJS(exec, arguments[i]));

    CallData callData;
    CallType callType = jsObject->getCallData(callData);
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
    TiObject* jsObject = toJS(object);
    ConstructData constructData;
    return jsObject->getConstructData(constructData) != ConstructTypeNone;
}

TiObjectRef TiObjectCallAsConstructor(TiContextRef ctx, TiObjectRef object, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiObject* jsObject = toJS(object);

    ConstructData constructData;
    ConstructType constructType = jsObject->getConstructData(constructData);
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
    OpaqueTiPropertyNameArray(TiGlobalData* globalData)
        : refCount(0)
        , globalData(globalData)
    {
    }
    
    unsigned refCount;
    TiGlobalData* globalData;
    Vector<TiRetainPtr<TiStringRef> > array;
};

TiPropertyNameArrayRef TiObjectCopyPropertyNames(TiContextRef ctx, TiObjectRef object)
{
    TiObject* jsObject = toJS(object);
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiGlobalData* globalData = &exec->globalData();

    TiPropertyNameArrayRef propertyNames = new OpaqueTiPropertyNameArray(globalData);
    PropertyNameArray array(globalData);
    jsObject->getPropertyNames(exec, array);

    size_t size = array.size();
    propertyNames->array.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i)
        propertyNames->array.append(TiRetainPtr<TiStringRef>(Adopt, OpaqueTiString::create(array[i].ustring()).leakRef()));
    
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
        APIEntryShim entryShim(array->globalData, false);
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
    APIEntryShim entryShim(propertyNames->globalData());
    propertyNames->add(propertyName->identifier(propertyNames->globalData()));
}
