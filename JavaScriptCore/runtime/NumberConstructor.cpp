/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "NumberConstructor.h"

#include "Lookup.h"
#include "NumberObject.h"
#include "NumberPrototype.h"
#include "Operations.h"

namespace TI {

static EncodedTiValue numberConstructorNaNValue(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue numberConstructorNegInfinity(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue numberConstructorPosInfinity(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue numberConstructorMaxValue(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue numberConstructorMinValue(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);

} // namespace TI

#include "NumberConstructor.lut.h"

namespace TI {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(NumberConstructor);

const ClassInfo NumberConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::numberConstructorTable, CREATE_METHOD_TABLE(NumberConstructor) };

/* Source for NumberConstructor.lut.h
@begin numberConstructorTable
   NaN                   numberConstructorNaNValue       DontEnum|DontDelete|ReadOnly
   NEGATIVE_INFINITY     numberConstructorNegInfinity    DontEnum|DontDelete|ReadOnly
   POSITIVE_INFINITY     numberConstructorPosInfinity    DontEnum|DontDelete|ReadOnly
   MAX_VALUE             numberConstructorMaxValue       DontEnum|DontDelete|ReadOnly
   MIN_VALUE             numberConstructorMinValue       DontEnum|DontDelete|ReadOnly
@end
*/

NumberConstructor::NumberConstructor(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
}

void NumberConstructor::finishCreation(VM& vm, NumberPrototype* numberPrototype)
{
    Base::finishCreation(vm, NumberPrototype::info()->className);
    ASSERT(inherits(info()));

    // Number.Prototype
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, numberPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool NumberConstructor::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<NumberConstructor, InternalFunction>(exec, ExecState::numberConstructorTable(exec), jsCast<NumberConstructor*>(object), propertyName, slot);
}

void NumberConstructor::put(JSCell* cell, ExecState* exec, PropertyName propertyName, TiValue value, PutPropertySlot& slot)
{
    lookupPut<NumberConstructor, InternalFunction>(exec, propertyName, value, ExecState::numberConstructorTable(exec), jsCast<NumberConstructor*>(cell), slot);
}

static EncodedTiValue numberConstructorNaNValue(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName)
{
    return TiValue::encode(jsNaN());
}

static EncodedTiValue numberConstructorNegInfinity(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName)
{
    return TiValue::encode(jsNumber(-std::numeric_limits<double>::infinity()));
}

static EncodedTiValue numberConstructorPosInfinity(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName)
{
    return TiValue::encode(jsNumber(std::numeric_limits<double>::infinity()));
}

static EncodedTiValue numberConstructorMaxValue(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName)
{
    return TiValue::encode(jsNumber(1.7976931348623157E+308));
}

static EncodedTiValue numberConstructorMinValue(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName)
{
    return TiValue::encode(jsNumber(5E-324));
}

// ECMA 15.7.1
static EncodedTiValue JSC_HOST_CALL constructWithNumberConstructor(ExecState* exec)
{
    NumberObject* object = NumberObject::create(exec->vm(), asInternalFunction(exec->callee())->globalObject()->numberObjectStructure());
    double n = exec->argumentCount() ? exec->uncheckedArgument(0).toNumber(exec) : 0;
    object->setInternalValue(exec->vm(), jsNumber(n));
    return TiValue::encode(object);
}

ConstructType NumberConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithNumberConstructor;
    return ConstructTypeHost;
}

// ECMA 15.7.2
static EncodedTiValue JSC_HOST_CALL callNumberConstructor(ExecState* exec)
{
    return TiValue::encode(jsNumber(!exec->argumentCount() ? 0 : exec->uncheckedArgument(0).toNumber(exec)));
}

CallType NumberConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callNumberConstructor;
    return CallTypeHost;
}

} // namespace TI
