/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(NumberConstructor);

static TiValue numberConstructorNaNValue(TiExcState*, TiValue, const Identifier&);
static TiValue numberConstructorNegInfinity(TiExcState*, TiValue, const Identifier&);
static TiValue numberConstructorPosInfinity(TiExcState*, TiValue, const Identifier&);
static TiValue numberConstructorMaxValue(TiExcState*, TiValue, const Identifier&);
static TiValue numberConstructorMinValue(TiExcState*, TiValue, const Identifier&);

} // namespace TI

#include "NumberConstructor.lut.h"

namespace TI {

const ClassInfo NumberConstructor::s_info = { "Function", &InternalFunction::s_info, 0, TiExcState::numberConstructorTable };

/* Source for NumberConstructor.lut.h
@begin numberConstructorTable
   NaN                   numberConstructorNaNValue       DontEnum|DontDelete|ReadOnly
   NEGATIVE_INFINITY     numberConstructorNegInfinity    DontEnum|DontDelete|ReadOnly
   POSITIVE_INFINITY     numberConstructorPosInfinity    DontEnum|DontDelete|ReadOnly
   MAX_VALUE             numberConstructorMaxValue       DontEnum|DontDelete|ReadOnly
   MIN_VALUE             numberConstructorMinValue       DontEnum|DontDelete|ReadOnly
@end
*/

NumberConstructor::NumberConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, NumberPrototype* numberPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, numberPrototype->s_info.className))
{
    ASSERT(inherits(&s_info));

    // Number.Prototype
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, numberPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool NumberConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<NumberConstructor, InternalFunction>(exec, TiExcState::numberConstructorTable(exec), this, propertyName, slot);
}

bool NumberConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<NumberConstructor, InternalFunction>(exec, TiExcState::numberConstructorTable(exec), this, propertyName, descriptor);
}

static TiValue numberConstructorNaNValue(TiExcState*, TiValue, const Identifier&)
{
    return jsNaN();
}

static TiValue numberConstructorNegInfinity(TiExcState*, TiValue, const Identifier&)
{
    return jsNumber(-Inf);
}

static TiValue numberConstructorPosInfinity(TiExcState*, TiValue, const Identifier&)
{
    return jsNumber(Inf);
}

static TiValue numberConstructorMaxValue(TiExcState*, TiValue, const Identifier&)
{
    return jsNumber(1.7976931348623157E+308);
}

static TiValue numberConstructorMinValue(TiExcState*, TiValue, const Identifier&)
{
    return jsNumber(5E-324);
}

// ECMA 15.7.1
static EncodedTiValue JSC_HOST_CALL constructWithNumberConstructor(TiExcState* exec)
{
    NumberObject* object = new (exec) NumberObject(exec->globalData(), asInternalFunction(exec->callee())->globalObject()->numberObjectStructure());
    double n = exec->argumentCount() ? exec->argument(0).toNumber(exec) : 0;
    object->setInternalValue(exec->globalData(), jsNumber(n));
    return TiValue::encode(object);
}

ConstructType NumberConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithNumberConstructor;
    return ConstructTypeHost;
}

// ECMA 15.7.2
static EncodedTiValue JSC_HOST_CALL callNumberConstructor(TiExcState* exec)
{
    return TiValue::encode(jsNumber(!exec->argumentCount() ? 0 : exec->argument(0).toNumber(exec)));
}

CallType NumberConstructor::getCallData(CallData& callData)
{
    callData.native.function = callNumberConstructor;
    return CallTypeHost;
}

} // namespace TI
