/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "ObjectConstructor.h"

#include "Error.h"
#include "ExceptionHelpers.h"
#include "TiFunction.h"
#include "TiArray.h"
#include "TiGlobalObject.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "PropertyDescriptor.h"
#include "PropertyNameArray.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(ObjectConstructor);

static EncodedTiValue JSC_HOST_CALL objectConstructorGetPrototypeOf(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorGetOwnPropertyDescriptor(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorGetOwnPropertyNames(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorKeys(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorDefineProperty(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorDefineProperties(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorCreate(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorSeal(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorFreeze(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorPreventExtensions(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorIsSealed(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorIsFrozen(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectConstructorIsExtensible(TiExcState*);

}

#include "ObjectConstructor.lut.h"

namespace TI {

const ClassInfo ObjectConstructor::s_info = { "Function", &InternalFunction::s_info, 0, TiExcState::objectConstructorTable };

/* Source for ObjectConstructor.lut.h
@begin objectConstructorTable
  getPrototypeOf            objectConstructorGetPrototypeOf             DontEnum|Function 1
  getOwnPropertyDescriptor  objectConstructorGetOwnPropertyDescriptor   DontEnum|Function 2
  getOwnPropertyNames       objectConstructorGetOwnPropertyNames        DontEnum|Function 1
  keys                      objectConstructorKeys                       DontEnum|Function 1
  defineProperty            objectConstructorDefineProperty             DontEnum|Function 3
  defineProperties          objectConstructorDefineProperties           DontEnum|Function 2
  create                    objectConstructorCreate                     DontEnum|Function 2
  seal                      objectConstructorSeal                       DontEnum|Function 1
  freeze                    objectConstructorFreeze                     DontEnum|Function 1
  preventExtensions         objectConstructorPreventExtensions          DontEnum|Function 1
  isSealed                  objectConstructorIsSealed                   DontEnum|Function 1
  isFrozen                  objectConstructorIsFrozen                   DontEnum|Function 1
  isExtensible              objectConstructorIsExtensible               DontEnum|Function 1
@end
*/

ObjectConstructor::ObjectConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, ObjectPrototype* objectPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, "Object"))
{
    // ECMA 15.2.3.1
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, objectPrototype, DontEnum | DontDelete | ReadOnly);
    // no. of arguments for constructor
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool ObjectConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<TiObject>(exec, TiExcState::objectConstructorTable(exec), this, propertyName, slot);
}

bool ObjectConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<TiObject>(exec, TiExcState::objectConstructorTable(exec), this, propertyName, descriptor);
}

// ECMA 15.2.2
static ALWAYS_INLINE TiObject* constructObject(TiExcState* exec, TiGlobalObject* globalObject, const ArgList& args)
{
    TiValue arg = args.at(0);
    if (arg.isUndefinedOrNull())
        return constructEmptyObject(exec, globalObject);
    return arg.toObject(exec, globalObject);
}

static EncodedTiValue JSC_HOST_CALL constructWithObjectConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructObject(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

ConstructType ObjectConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithObjectConstructor;
    return ConstructTypeHost;
}

static EncodedTiValue JSC_HOST_CALL callObjectConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructObject(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

CallType ObjectConstructor::getCallData(CallData& callData)
{
    callData.native.function = callObjectConstructor;
    return CallTypeHost;
}

EncodedTiValue JSC_HOST_CALL objectConstructorGetPrototypeOf(TiExcState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, "Requested prototype of a value that is not an object."));
    return TiValue::encode(asObject(exec->argument(0))->prototype());
}

EncodedTiValue JSC_HOST_CALL objectConstructorGetOwnPropertyDescriptor(TiExcState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, "Requested property descriptor of a value that is not an object."));
    UString propertyName = exec->argument(1).toString(exec);
    if (exec->hadException())
        return TiValue::encode(jsNull());
    TiObject* object = asObject(exec->argument(0));
    PropertyDescriptor descriptor;
    if (!object->getOwnPropertyDescriptor(exec, Identifier(exec, propertyName), descriptor))
        return TiValue::encode(jsUndefined());
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiObject* description = constructEmptyObject(exec);
    if (!descriptor.isAccessorDescriptor()) {
        description->putDirect(exec->globalData(), exec->propertyNames().value, descriptor.value() ? descriptor.value() : jsUndefined(), 0);
        description->putDirect(exec->globalData(), exec->propertyNames().writable, jsBoolean(descriptor.writable()), 0);
    } else {
        description->putDirect(exec->globalData(), exec->propertyNames().get, descriptor.getter() ? descriptor.getter() : jsUndefined(), 0);
        description->putDirect(exec->globalData(), exec->propertyNames().set, descriptor.setter() ? descriptor.setter() : jsUndefined(), 0);
    }
    
    description->putDirect(exec->globalData(), exec->propertyNames().enumerable, jsBoolean(descriptor.enumerable()), 0);
    description->putDirect(exec->globalData(), exec->propertyNames().configurable, jsBoolean(descriptor.configurable()), 0);

    return TiValue::encode(description);
}

// FIXME: Use the enumeration cache.
EncodedTiValue JSC_HOST_CALL objectConstructorGetOwnPropertyNames(TiExcState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, "Requested property names of a value that is not an object."));
    PropertyNameArray properties(exec);
    asObject(exec->argument(0))->getOwnPropertyNames(exec, properties, IncludeDontEnumProperties);
    TiArray* names = constructEmptyArray(exec);
    size_t numProperties = properties.size();
    for (size_t i = 0; i < numProperties; i++)
        names->push(exec, jsOwnedString(exec, properties[i].ustring()));
    return TiValue::encode(names);
}

// FIXME: Use the enumeration cache.
EncodedTiValue JSC_HOST_CALL objectConstructorKeys(TiExcState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, "Requested keys of a value that is not an object."));
    PropertyNameArray properties(exec);
    asObject(exec->argument(0))->getOwnPropertyNames(exec, properties);
    TiArray* keys = constructEmptyArray(exec);
    size_t numProperties = properties.size();
    for (size_t i = 0; i < numProperties; i++)
        keys->push(exec, jsOwnedString(exec, properties[i].ustring()));
    return TiValue::encode(keys);
}

// ES5 8.10.5 ToPropertyDescriptor
static bool toPropertyDescriptor(TiExcState* exec, TiValue in, PropertyDescriptor& desc)
{
    if (!in.isObject()) {
        throwError(exec, createTypeError(exec, "Property description must be an object."));
        return false;
    }
    TiObject* description = asObject(in);

    PropertySlot enumerableSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().enumerable, enumerableSlot)) {
        desc.setEnumerable(enumerableSlot.getValue(exec, exec->propertyNames().enumerable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    PropertySlot configurableSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().configurable, configurableSlot)) {
        desc.setConfigurable(configurableSlot.getValue(exec, exec->propertyNames().configurable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    TiValue value;
    PropertySlot valueSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().value, valueSlot)) {
        desc.setValue(valueSlot.getValue(exec, exec->propertyNames().value));
        if (exec->hadException())
            return false;
    }

    PropertySlot writableSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().writable, writableSlot)) {
        desc.setWritable(writableSlot.getValue(exec, exec->propertyNames().writable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    PropertySlot getSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().get, getSlot)) {
        TiValue get = getSlot.getValue(exec, exec->propertyNames().get);
        if (exec->hadException())
            return false;
        if (!get.isUndefined()) {
            CallData callData;
            if (getCallData(get, callData) == CallTypeNone) {
                throwError(exec, createTypeError(exec, "Getter must be a function."));
                return false;
            }
        } else
            get = TiValue();
        desc.setGetter(get);
    }

    PropertySlot setSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().set, setSlot)) {
        TiValue set = setSlot.getValue(exec, exec->propertyNames().set);
        if (exec->hadException())
            return false;
        if (!set.isUndefined()) {
            CallData callData;
            if (getCallData(set, callData) == CallTypeNone) {
                throwError(exec, createTypeError(exec, "Setter must be a function."));
                return false;
            }
        } else
            set = TiValue();

        desc.setSetter(set);
    }

    if (!desc.isAccessorDescriptor())
        return true;

    if (desc.value()) {
        throwError(exec, createTypeError(exec, "Invalid property.  'value' present on property with getter or setter."));
        return false;
    }

    if (desc.writablePresent()) {
        throwError(exec, createTypeError(exec, "Invalid property.  'writable' present on property with getter or setter."));
        return false;
    }
    return true;
}

EncodedTiValue JSC_HOST_CALL objectConstructorDefineProperty(TiExcState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, "Properties can only be defined on Objects."));
    TiObject* O = asObject(exec->argument(0));
    UString propertyName = exec->argument(1).toString(exec);
    if (exec->hadException())
        return TiValue::encode(jsNull());
    PropertyDescriptor descriptor;
    if (!toPropertyDescriptor(exec, exec->argument(2), descriptor))
        return TiValue::encode(jsNull());
    ASSERT((descriptor.attributes() & (Getter | Setter)) || (!descriptor.isAccessorDescriptor()));
    ASSERT(!exec->hadException());
    O->defineOwnProperty(exec, Identifier(exec, propertyName), descriptor, true);
    return TiValue::encode(O);
}

static TiValue defineProperties(TiExcState* exec, TiObject* object, TiObject* properties)
{
    PropertyNameArray propertyNames(exec);
    asObject(properties)->getOwnPropertyNames(exec, propertyNames);
    size_t numProperties = propertyNames.size();
    Vector<PropertyDescriptor> descriptors;
    MarkedArgumentBuffer markBuffer;
    for (size_t i = 0; i < numProperties; i++) {
        PropertySlot slot;
        TiValue prop = properties->get(exec, propertyNames[i]);
        if (exec->hadException())
            return jsNull();
        PropertyDescriptor descriptor;
        if (!toPropertyDescriptor(exec, prop, descriptor))
            return jsNull();
        descriptors.append(descriptor);
        // Ensure we mark all the values that we're accumulating
        if (descriptor.isDataDescriptor() && descriptor.value())
            markBuffer.append(descriptor.value());
        if (descriptor.isAccessorDescriptor()) {
            if (descriptor.getter())
                markBuffer.append(descriptor.getter());
            if (descriptor.setter())
                markBuffer.append(descriptor.setter());
        }
    }
    for (size_t i = 0; i < numProperties; i++) {
        object->defineOwnProperty(exec, propertyNames[i], descriptors[i], true);
        if (exec->hadException())
            return jsNull();
    }
    return object;
}

EncodedTiValue JSC_HOST_CALL objectConstructorDefineProperties(TiExcState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, "Properties can only be defined on Objects."));
    if (!exec->argument(1).isObject())
        return throwVMError(exec, createTypeError(exec, "Property descriptor list must be an Object."));
    return TiValue::encode(defineProperties(exec, asObject(exec->argument(0)), asObject(exec->argument(1))));
}

EncodedTiValue JSC_HOST_CALL objectConstructorCreate(TiExcState* exec)
{
    if (!exec->argument(0).isObject() && !exec->argument(0).isNull())
        return throwVMError(exec, createTypeError(exec, "Object prototype may only be an Object or null."));
    TiValue proto = exec->argument(0);
    TiObject* newObject = proto.isObject() ? constructEmptyObject(exec, asObject(proto)->inheritorID(exec->globalData())) : constructEmptyObject(exec, exec->lexicalGlobalObject()->nullPrototypeObjectStructure());
    if (exec->argument(1).isUndefined())
        return TiValue::encode(newObject);
    if (!exec->argument(1).isObject())
        return throwVMError(exec, createTypeError(exec, "Property descriptor list must be an Object."));
    return TiValue::encode(defineProperties(exec, newObject, asObject(exec->argument(1))));
}

EncodedTiValue JSC_HOST_CALL objectConstructorSeal(TiExcState* exec)
{
    TiValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, "Object.seal can only be called on Objects."));
    asObject(obj)->seal(exec->globalData());
    return TiValue::encode(obj);
}

EncodedTiValue JSC_HOST_CALL objectConstructorFreeze(TiExcState* exec)
{
    TiValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, "Object.freeze can only be called on Objects."));
    asObject(obj)->freeze(exec->globalData());
    return TiValue::encode(obj);
}

EncodedTiValue JSC_HOST_CALL objectConstructorPreventExtensions(TiExcState* exec)
{
    TiValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, "Object.preventExtensions can only be called on Objects."));
    asObject(obj)->preventExtensions(exec->globalData());
    return TiValue::encode(obj);
}

EncodedTiValue JSC_HOST_CALL objectConstructorIsSealed(TiExcState* exec)
{
    TiValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, "Object.isSealed can only be called on Objects."));
    return TiValue::encode(jsBoolean(asObject(obj)->isSealed(exec->globalData())));
}

EncodedTiValue JSC_HOST_CALL objectConstructorIsFrozen(TiExcState* exec)
{
    TiValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, "Object.isFrozen can only be called on Objects."));
    return TiValue::encode(jsBoolean(asObject(obj)->isFrozen(exec->globalData())));
}

EncodedTiValue JSC_HOST_CALL objectConstructorIsExtensible(TiExcState* exec)
{
    TiValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, "Object.isExtensible can only be called on Objects."));
    return TiValue::encode(jsBoolean(asObject(obj)->isExtensible()));
}

} // namespace TI
