/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#include "NumberObject.h"
#include "NumberPrototype.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(NumberConstructor);

static TiValue numberConstructorNaNValue(TiExcState*, const Identifier&, const PropertySlot&);
static TiValue numberConstructorNegInfinity(TiExcState*, const Identifier&, const PropertySlot&);
static TiValue numberConstructorPosInfinity(TiExcState*, const Identifier&, const PropertySlot&);
static TiValue numberConstructorMaxValue(TiExcState*, const Identifier&, const PropertySlot&);
static TiValue numberConstructorMinValue(TiExcState*, const Identifier&, const PropertySlot&);

} // namespace TI

#include "NumberConstructor.lut.h"

namespace TI {

const ClassInfo NumberConstructor::info = { "Function", &InternalFunction::info, 0, TiExcState::numberTable };

/* Source for NumberConstructor.lut.h
@begin numberTable
   NaN                   numberConstructorNaNValue       DontEnum|DontDelete|ReadOnly
   NEGATIVE_INFINITY     numberConstructorNegInfinity    DontEnum|DontDelete|ReadOnly
   POSITIVE_INFINITY     numberConstructorPosInfinity    DontEnum|DontDelete|ReadOnly
   MAX_VALUE             numberConstructorMaxValue       DontEnum|DontDelete|ReadOnly
   MIN_VALUE             numberConstructorMinValue       DontEnum|DontDelete|ReadOnly
@end
*/

NumberConstructor::NumberConstructor(TiExcState* exec, NonNullPassRefPtr<Structure> structure, NumberPrototype* numberPrototype)
    : InternalFunction(&exec->globalData(), structure, Identifier(exec, numberPrototype->info.className))
{
    // Number.Prototype
    putDirectWithoutTransition(exec->propertyNames().prototype, numberPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->propertyNames().length, jsNumber(exec, 1), ReadOnly | DontEnum | DontDelete);
}

bool NumberConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<NumberConstructor, InternalFunction>(exec, TiExcState::numberTable(exec), this, propertyName, slot);
}

bool NumberConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<NumberConstructor, InternalFunction>(exec, TiExcState::numberTable(exec), this, propertyName, descriptor);
}

static TiValue numberConstructorNaNValue(TiExcState* exec, const Identifier&, const PropertySlot&)
{
    return jsNaN(exec);
}

static TiValue numberConstructorNegInfinity(TiExcState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, -Inf);
}

static TiValue numberConstructorPosInfinity(TiExcState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, Inf);
}

static TiValue numberConstructorMaxValue(TiExcState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, 1.7976931348623157E+308);
}

static TiValue numberConstructorMinValue(TiExcState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, 5E-324);
}

// ECMA 15.7.1
static TiObject* constructWithNumberConstructor(TiExcState* exec, TiObject*, const ArgList& args)
{
    NumberObject* object = new (exec) NumberObject(exec->lexicalGlobalObject()->numberObjectStructure());
    double n = args.isEmpty() ? 0 : args.at(0).toNumber(exec);
    object->setInternalValue(jsNumber(exec, n));
    return object;
}

ConstructType NumberConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithNumberConstructor;
    return ConstructTypeHost;
}

// ECMA 15.7.2
static TiValue JSC_HOST_CALL callNumberConstructor(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, args.isEmpty() ? 0 : args.at(0).toNumber(exec));
}

CallType NumberConstructor::getCallData(CallData& callData)
{
    callData.native.function = callNumberConstructor;
    return CallTypeHost;
}

} // namespace TI
