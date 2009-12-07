/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All Rights Reserved.
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
#include "MathObject.h"

#include "ObjectPrototype.h"
#include "Operations.h"
#include <time.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/RandomNumber.h>
#include <wtf/RandomNumberSeed.h>

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(MathObject);

static TiValue JSC_HOST_CALL mathProtoFuncAbs(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncACos(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncASin(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncATan(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncATan2(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncCeil(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncCos(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncExp(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncFloor(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncLog(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncMax(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncMin(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncPow(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncRandom(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncRound(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncSin(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncSqrt(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL mathProtoFuncTan(TiExcState*, TiObject*, TiValue, const ArgList&);

}

#include "MathObject.lut.h"

namespace TI {

// ------------------------------ MathObject --------------------------------

const ClassInfo MathObject::info = { "Math", 0, 0, TiExcState::mathTable };

/* Source for MathObject.lut.h
@begin mathTable
  abs           mathProtoFuncAbs               DontEnum|Function 1
  acos          mathProtoFuncACos              DontEnum|Function 1
  asin          mathProtoFuncASin              DontEnum|Function 1
  atan          mathProtoFuncATan              DontEnum|Function 1
  atan2         mathProtoFuncATan2             DontEnum|Function 2
  ceil          mathProtoFuncCeil              DontEnum|Function 1
  cos           mathProtoFuncCos               DontEnum|Function 1
  exp           mathProtoFuncExp               DontEnum|Function 1
  floor         mathProtoFuncFloor             DontEnum|Function 1
  log           mathProtoFuncLog               DontEnum|Function 1
  max           mathProtoFuncMax               DontEnum|Function 2
  min           mathProtoFuncMin               DontEnum|Function 2
  pow           mathProtoFuncPow               DontEnum|Function 2
  random        mathProtoFuncRandom            DontEnum|Function 0 
  round         mathProtoFuncRound             DontEnum|Function 1
  sin           mathProtoFuncSin               DontEnum|Function 1
  sqrt          mathProtoFuncSqrt              DontEnum|Function 1
  tan           mathProtoFuncTan               DontEnum|Function 1
@end
*/

MathObject::MathObject(TiExcState* exec, NonNullPassRefPtr<Structure> structure)
    : TiObject(structure)
{
    putDirectWithoutTransition(Identifier(exec, "E"), jsNumber(exec, exp(1.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "LN2"), jsNumber(exec, log(2.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "LN10"), jsNumber(exec, log(10.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "LOG2E"), jsNumber(exec, 1.0 / log(2.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "LOG10E"), jsNumber(exec, 1.0 / log(10.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "PI"), jsNumber(exec, piDouble), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "SQRT1_2"), jsNumber(exec, sqrt(0.5)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(Identifier(exec, "SQRT2"), jsNumber(exec, sqrt(2.0)), DontDelete | DontEnum | ReadOnly);
}

// ECMA 15.8

bool MathObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<TiObject>(exec, TiExcState::mathTable(exec), this, propertyName, slot);
}

bool MathObject::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<TiObject>(exec, TiExcState::mathTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions --------------------------------

TiValue JSC_HOST_CALL mathProtoFuncAbs(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, fabs(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncACos(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, acos(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncASin(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, asin(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncATan(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, atan(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncATan2(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, atan2(args.at(0).toNumber(exec), args.at(1).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncCeil(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, ceil(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncCos(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, cos(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncExp(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, exp(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncFloor(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, floor(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncLog(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, log(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncMax(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    unsigned argsCount = args.size();
    double result = -Inf;
    for (unsigned k = 0; k < argsCount; ++k) {
        double val = args.at(k).toNumber(exec);
        if (isnan(val)) {
            result = NaN;
            break;
        }
        if (val > result || (val == 0 && result == 0 && !signbit(val)))
            result = val;
    }
    return jsNumber(exec, result);
}

TiValue JSC_HOST_CALL mathProtoFuncMin(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    unsigned argsCount = args.size();
    double result = +Inf;
    for (unsigned k = 0; k < argsCount; ++k) {
        double val = args.at(k).toNumber(exec);
        if (isnan(val)) {
            result = NaN;
            break;
        }
        if (val < result || (val == 0 && result == 0 && signbit(val)))
            result = val;
    }
    return jsNumber(exec, result);
}

TiValue JSC_HOST_CALL mathProtoFuncPow(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    // ECMA 15.8.2.1.13

    double arg = args.at(0).toNumber(exec);
    double arg2 = args.at(1).toNumber(exec);

    if (isnan(arg2))
        return jsNaN(exec);
    if (isinf(arg2) && fabs(arg) == 1)
        return jsNaN(exec);
    return jsNumber(exec, pow(arg, arg2));
}

TiValue JSC_HOST_CALL mathProtoFuncRandom(TiExcState* exec, TiObject*, TiValue, const ArgList&)
{
    return jsNumber(exec, exec->globalData().weakRandom.get());
}

TiValue JSC_HOST_CALL mathProtoFuncRound(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    double arg = args.at(0).toNumber(exec);
    if (signbit(arg) && arg >= -0.5)
         return jsNumber(exec, -0.0);
    return jsNumber(exec, floor(arg + 0.5));
}

TiValue JSC_HOST_CALL mathProtoFuncSin(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, sin(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncSqrt(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, sqrt(args.at(0).toNumber(exec)));
}

TiValue JSC_HOST_CALL mathProtoFuncTan(TiExcState* exec, TiObject*, TiValue, const ArgList& args)
{
    return jsNumber(exec, tan(args.at(0).toNumber(exec)));
}

} // namespace TI
