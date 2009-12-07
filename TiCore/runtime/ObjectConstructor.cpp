/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "TiFunction.h"
#include "TiArray.h"
#include "TiGlobalObject.h"
#include "ObjectPrototype.h"
#include "PropertyDescriptor.h"
#include "PropertyNameArray.h"
#include "PrototypeFunction.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(ObjectConstructor);

static TiValue JSC_HOST_CALL objectConstructorGetPrototypeOf(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectConstructorGetOwnPropertyDescriptor(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectConstructorKeys(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectConstructorDefineProperty(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectConstructorDefineProperties(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectConstructorCreate(TiExcState*, TiObject*, TiValue, const ArgList&);

ObjectConstructor::ObjectConstructor(TiExcState* exec, NonNullPassRefPtr<Structure> structure, ObjectPrototype* objectPrototype, Structure* prototypeFunctionStructure)
: InternalFunction(&exec->globalData(), structure, Identifier(exec, "Object"))
{
    // ECMA 15.2.3.1
    putDirectWithoutTransition(exec->propertyNames().prototype, objectPrototype, DontEnum | DontDelete | ReadOnly);
    
    // no. of arguments for constructor
    putDirectWithoutTransition(exec->propertyNames().length, jsNumber(exec, 1), ReadOnly | DontEnum | DontDelete);
    
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().getPrototypeOf, objectConstructorGetPrototypeOf), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 2, exec->propertyNames().getOwnPropertyDescriptor, objectConstructorGetOwnPropertyDescriptor), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().keys, objectConstructorKeys), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 3, exec->propertyNames().defineProperty, objectConstructorDefineProperty), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 2, exec->propertyNames().defineProperties, objectConstructorDefineProperties), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 2, exec->propertyNames().create, objectConstructorCreate), DontEnum);
}

// ECMA 15.2.2
static ALWAYS_INLINE TiObject* constructObject(TiExcState* exec, const ArgList& args)
{
    TiValue arg = args.at(0);
    if (arg.isUndefinedOrNull())
        return new (exec) TiObject(exec->lexicalGlobalObject()->emptyObjectStructure());
    return arg.toObject(exec);
}

static TiObject* constructWithObjectConstructor(TiExcState* exec, TiObject*, const ArgList& args)
{
    return constructObject(exec, args);
}

ConstructType ObjectConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithObjectConstructor;
    return ConstructTypeHost;
}

static TiValue JSC_HOST_CALL callObjectConstructor(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return constructObject(exec, args);
}

CallType ObjectConstructor::getCallData(CallData& callData)
{
    callData.native.function = callObjectConstructor;
    return CallTypeHost;
}

TiValue JSC_HOST_CALL objectConstructorGetPrototypeOf(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    if (!args.at(0).isObject())
        return throwError(exec, TypeError, "Requested prototype of a value that is not an object.");
    return asObject(args.at(0))->prototype();
}

TiValue JSC_HOST_CALL objectConstructorGetOwnPropertyDescriptor(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    if (!args.at(0).isObject())
        return throwError(exec, TypeError, "Requested property descriptor of a value that is not an object.");
    UString propertyName = args.at(1).toString(exec);
    if (exec->hadException())
        return jsNull();
    TiObject* object = asObject(args.at(0));
    PropertyDescriptor descriptor;
    if (!object->getOwnPropertyDescriptor(exec, Identifier(exec, propertyName), descriptor))
        return jsUndefined();
    if (exec->hadException())
        return jsUndefined();

    TiObject* description = constructEmptyObject(exec);
    if (!descriptor.isAccessorDescriptor()) {
        description->putDirect(exec->propertyNames().value, descriptor.value() ? descriptor.value() : jsUndefined(), 0);
        description->putDirect(exec->propertyNames().writable, jsBoolean(descriptor.writable()), 0);
    } else {
        description->putDirect(exec->propertyNames().get, descriptor.getter() ? descriptor.getter() : jsUndefined(), 0);
        description->putDirect(exec->propertyNames().set, descriptor.setter() ? descriptor.setter() : jsUndefined(), 0);
    }
    
    description->putDirect(exec->propertyNames().enumerable, jsBoolean(descriptor.enumerable()), 0);
    description->putDirect(exec->propertyNames().configurable, jsBoolean(descriptor.configurable()), 0);

    return description;
}

// FIXME: Use the enumeration cache.
TiValue JSC_HOST_CALL objectConstructorKeys(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    if (!args.at(0).isObject())
        return throwError(exec, TypeError, "Requested keys of a value that is not an object.");
    PropertyNameArray properties(exec);
    asObject(args.at(0))->getOwnPropertyNames(exec, properties);
    TiArray* keys = constructEmptyArray(exec);
    size_t numProperties = properties.size();
    for (size_t i = 0; i < numProperties; i++)
        keys->push(exec, jsOwnedString(exec, properties[i].ustring()));
    return keys;
}

// ES5 8.10.5 ToPropertyDescriptor
static bool toPropertyDescriptor(TiExcState* exec, TiValue in, PropertyDescriptor& desc)
{
    if (!in.isObject()) {
        throwError(exec, TypeError, "Property description must be an object.");
        return false;
    }
    TiObject* description = asObject(in);

    PropertySlot enumerableSlot;
    if (description->getPropertySlot(exec, exec->propertyNames().enumerable, enumerableSlot)) {
        desc.setEnumerable(enumerableSlot.getValue(exec, exec->propertyNames().enumerable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    PropertySlot configurableSlot;
    if (description->getPropertySlot(exec, exec->propertyNames().configurable, configurableSlot)) {
        desc.setConfigurable(configurableSlot.getValue(exec, exec->propertyNames().configurable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    TiValue value;
    PropertySlot valueSlot;
    if (description->getPropertySlot(exec, exec->propertyNames().value, valueSlot)) {
        desc.setValue(valueSlot.getValue(exec, exec->propertyNames().value));
        if (exec->hadException())
            return false;
    }

    PropertySlot writableSlot;
    if (description->getPropertySlot(exec, exec->propertyNames().writable, writableSlot)) {
        desc.setWritable(writableSlot.getValue(exec, exec->propertyNames().writable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    PropertySlot getSlot;
    if (description->getPropertySlot(exec, exec->propertyNames().get, getSlot)) {
        TiValue get = getSlot.getValue(exec, exec->propertyNames().get);
        if (exec->hadException())
            return false;
        if (!get.isUndefined()) {
            CallData callData;
            if (get.getCallData(callData) == CallTypeNone) {
                throwError(exec, TypeError, "Getter must be a function.");
                return false;
            }
        } else
            get = TiValue();
        desc.setGetter(get);
    }

    PropertySlot setSlot;
    if (description->getPropertySlot(exec, exec->propertyNames().set, setSlot)) {
        TiValue set = setSlot.getValue(exec, exec->propertyNames().set);
        if (exec->hadException())
            return false;
        if (!set.isUndefined()) {
            CallData callData;
            if (set.getCallData(callData) == CallTypeNone) {
                throwError(exec, TypeError, "Setter must be a function.");
                return false;
            }
        } else
            set = TiValue();

        desc.setSetter(set);
    }

    if (!desc.isAccessorDescriptor())
        return true;

    if (desc.value()) {
        throwError(exec, TypeError, "Invalid property.  'value' present on property with getter or setter.");
        return false;
    }

    if (desc.writablePresent()) {
        throwError(exec, TypeError, "Invalid property.  'writable' present on property with getter or setter.");
        return false;
    }
    return true;
}

TiValue JSC_HOST_CALL objectConstructorDefineProperty(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    if (!args.at(0).isObject())
        return throwError(exec, TypeError, "Properties can only be defined on Objects.");
    TiObject* O = asObject(args.at(0));
    UString propertyName = args.at(1).toString(exec);
    if (exec->hadException())
        return jsNull();
    PropertyDescriptor descriptor;
    if (!toPropertyDescriptor(exec, args.at(2), descriptor))
        return jsNull();
    ASSERT((descriptor.attributes() & (Getter | Setter)) || (!descriptor.isAccessorDescriptor()));
    ASSERT(!exec->hadException());
    O->defineOwnProperty(exec, Identifier(exec, propertyName), descriptor, true);
    return O;
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

TiValue JSC_HOST_CALL objectConstructorDefineProperties(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    if (!args.at(0).isObject())
        return throwError(exec, TypeError, "Properties can only be defined on Objects.");
    if (!args.at(1).isObject())
        return throwError(exec, TypeError, "Property descriptor list must be an Object.");
    return defineProperties(exec, asObject(args.at(0)), asObject(args.at(1)));
}

TiValue JSC_HOST_CALL objectConstructorCreate(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    if (!args.at(0).isObject() && !args.at(0).isNull())
        return throwError(exec, TypeError, "Object prototype may only be an Object or null.");
    TiObject* newObject = constructEmptyObject(exec);
    newObject->setPrototype(args.at(0));
    if (args.at(1).isUndefined())
        return newObject;
    if (!args.at(1).isObject())
        return throwError(exec, TypeError, "Property descriptor list must be an Object.");
    return defineProperties(exec, newObject, asObject(args.at(1)));
}

} // namespace TI
