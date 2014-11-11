/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ObjectPrototype.h"

#include "Error.h"
#include "GetterSetter.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Operations.h"
#include "StructureRareDataInlines.h"

namespace TI {

static EncodedTiValue JSC_HOST_CALL objectProtoFuncValueOf(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineGetter(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineSetter(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupGetter(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupSetter(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(ExecState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncToLocaleString(ExecState*);

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ObjectPrototype);

const ClassInfo ObjectPrototype::s_info = { "Object", &JSNonFinalObject::s_info, 0, 0, CREATE_METHOD_TABLE(ObjectPrototype) };

ObjectPrototype::ObjectPrototype(VM& vm, Structure* stucture)
    : JSNonFinalObject(vm, stucture)
{
}

void ObjectPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);
    
    JSC_NATIVE_FUNCTION(vm.propertyNames->toString, objectProtoFuncToString, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->toLocaleString, objectProtoFuncToLocaleString, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->valueOf, objectProtoFuncValueOf, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->hasOwnProperty, objectProtoFuncHasOwnProperty, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->propertyIsEnumerable, objectProtoFuncPropertyIsEnumerable, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->isPrototypeOf, objectProtoFuncIsPrototypeOf, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__defineGetter__, objectProtoFuncDefineGetter, DontEnum, 2);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__defineSetter__, objectProtoFuncDefineSetter, DontEnum, 2);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__lookupGetter__, objectProtoFuncLookupGetter, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__lookupSetter__, objectProtoFuncLookupSetter, DontEnum, 1);
}

ObjectPrototype* ObjectPrototype::create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
{
    ObjectPrototype* prototype = new (NotNull, allocateCell<ObjectPrototype>(vm.heap)) ObjectPrototype(vm, structure);
    prototype->finishCreation(vm, globalObject);
    return prototype;
}

// ------------------------------ Functions --------------------------------

EncodedTiValue JSC_HOST_CALL objectProtoFuncValueOf(ExecState* exec)
{
    TiValue thisValue = exec->hostThisValue().toThis(exec, StrictMode);
    return TiValue::encode(thisValue.toObject(exec));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(ExecState* exec)
{
    TiValue thisValue = exec->hostThisValue().toThis(exec, StrictMode);
    return TiValue::encode(jsBoolean(thisValue.toObject(exec)->hasOwnProperty(exec, Identifier(exec, exec->argument(0).toString(exec)->value(exec)))));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(ExecState* exec)
{
    TiValue thisValue = exec->hostThisValue().toThis(exec, StrictMode);
    JSObject* thisObj = thisValue.toObject(exec);

    if (!exec->argument(0).isObject())
        return TiValue::encode(jsBoolean(false));

    TiValue v = asObject(exec->argument(0))->prototype();

    while (true) {
        if (!v.isObject())
            return TiValue::encode(jsBoolean(false));
        if (v == thisObj)
            return TiValue::encode(jsBoolean(true));
        v = asObject(v)->prototype();
    }
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineGetter(ExecState* exec)
{
    JSObject* thisObject = exec->hostThisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue get = exec->argument(1);
    CallData callData;
    if (getCallData(get, callData) == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("invalid getter usage")));

    PropertyDescriptor descriptor;
    descriptor.setGetter(get);
    descriptor.setEnumerable(true);
    descriptor.setConfigurable(true);
    thisObject->methodTable()->defineOwnProperty(thisObject, exec, Identifier(exec, exec->argument(0).toString(exec)->value(exec)), descriptor, false);

    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineSetter(ExecState* exec)
{
    JSObject* thisObject = exec->hostThisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue set = exec->argument(1);
    CallData callData;
    if (getCallData(set, callData) == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("invalid setter usage")));

    PropertyDescriptor descriptor;
    descriptor.setSetter(set);
    descriptor.setEnumerable(true);
    descriptor.setConfigurable(true);
    thisObject->methodTable()->defineOwnProperty(thisObject, exec, Identifier(exec, exec->argument(0).toString(exec)->value(exec)), descriptor, false);

    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupGetter(ExecState* exec)
{
    JSObject* thisObject = exec->hostThisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    PropertySlot slot(thisObject);
    if (thisObject->getPropertySlot(exec, Identifier(exec, exec->argument(0).toString(exec)->value(exec)), slot)
        && slot.isAccessor())
        return TiValue::encode(slot.getterSetter()->getter());

    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupSetter(ExecState* exec)
{
    JSObject* thisObject = exec->hostThisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    PropertySlot slot(thisObject);
    if (thisObject->getPropertySlot(exec, Identifier(exec, exec->argument(0).toString(exec)->value(exec)), slot)
        && slot.isAccessor())
        return TiValue::encode(slot.getterSetter()->setter());

    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(ExecState* exec)
{
    JSObject* thisObject = exec->hostThisValue().toThis(exec, StrictMode).toObject(exec);
    Identifier propertyName(exec, exec->argument(0).toString(exec)->value(exec));

    PropertyDescriptor descriptor;
    bool enumerable = thisObject->getOwnPropertyDescriptor(exec, propertyName, descriptor) && descriptor.enumerable();
    return TiValue::encode(jsBoolean(enumerable));
}

// 15.2.4.3 Object.prototype.toLocaleString()
EncodedTiValue JSC_HOST_CALL objectProtoFuncToLocaleString(ExecState* exec)
{
    // 1. Let O be the result of calling ToObject passing the this value as the argument.
    JSObject* object = exec->hostThisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    // 2. Let toString be the result of calling the [[Get]] internal method of O passing "toString" as the argument.
    TiValue toString = object->get(exec, exec->propertyNames().toString);

    // 3. If IsCallable(toString) is false, throw a TypeError exception.
    CallData callData;
    CallType callType = getCallData(toString, callData);
    if (callType == CallTypeNone)
        return TiValue::encode(jsUndefined());

    // 4. Return the result of calling the [[Call]] internal method of toString passing O as the this value and no arguments.
    return TiValue::encode(call(exec, toString, callType, callData, object, exec->emptyList()));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncToString(ExecState* exec)
{
    TiValue thisValue = exec->hostThisValue().toThis(exec, StrictMode);
    if (thisValue.isUndefinedOrNull())
        return TiValue::encode(jsNontrivialString(exec, String(thisValue.isUndefined() ? ASCIILiteral("[object Undefined]") : ASCIILiteral("[object Null]"))));
    JSObject* thisObject = thisValue.toObject(exec);

    JSString* result = thisObject->structure()->objectToStringValue();
    if (!result) {
        RefPtr<StringImpl> newString = WTI::tryMakeString("[object ", thisObject->methodTable()->className(thisObject), "]");
        if (!newString)
            return TiValue::encode(throwOutOfMemoryError(exec));

        result = jsNontrivialString(exec, newString.release());
        thisObject->structure()->setObjectToStringValue(exec->vm(), thisObject, result);
    }
    return TiValue::encode(result);
}

} // namespace TI
