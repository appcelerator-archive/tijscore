/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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

#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include <time.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/RandomNumber.h>
#include <wtf/RandomNumberSeed.h>

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(MathObject);

static EncodedTiValue JSC_HOST_CALL mathProtoFuncAbs(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncACos(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncASin(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncATan(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncATan2(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncCeil(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncCos(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncExp(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncFloor(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncLog(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncMax(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncMin(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncPow(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncRandom(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncRound(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncSin(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncSqrt(TiExcState*);
static EncodedTiValue JSC_HOST_CALL mathProtoFuncTan(TiExcState*);

}

#include "MathObject.lut.h"

namespace TI {

const ClassInfo MathObject::s_info = { "Math", &TiObjectWithGlobalObject::s_info, 0, TiExcState::mathTable };

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

MathObject::MathObject(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure)
    : TiObjectWithGlobalObject(globalObject, structure)
{
    ASSERT(inherits(&s_info));

    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "E"), jsNumber(exp(1.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LN2"), jsNumber(log(2.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LN10"), jsNumber(log(10.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LOG2E"), jsNumber(1.0 / log(2.0)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "LOG10E"), jsNumber(0.4342944819032518), DontDelete | DontEnum | ReadOnly); // See ECMA-262 15.8.1.5
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "PI"), jsNumber(piDouble), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "SQRT1_2"), jsNumber(sqrt(0.5)), DontDelete | DontEnum | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "SQRT2"), jsNumber(sqrt(2.0)), DontDelete | DontEnum | ReadOnly);
}

bool MathObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<TiObject>(exec, TiExcState::mathTable(exec), this, propertyName, slot);
}

bool MathObject::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<TiObject>(exec, TiExcState::mathTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions --------------------------------

EncodedTiValue JSC_HOST_CALL mathProtoFuncAbs(TiExcState* exec)
{
    return TiValue::encode(jsNumber(fabs(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncACos(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(acos(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncASin(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(asin(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncATan(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(atan(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncATan2(TiExcState* exec)
{
    double arg0 = exec->argument(0).toNumber(exec);
    double arg1 = exec->argument(1).toNumber(exec);
    return TiValue::encode(jsDoubleNumber(atan2(arg0, arg1)));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncCeil(TiExcState* exec)
{
    return TiValue::encode(jsNumber(ceil(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncCos(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(cos(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncExp(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(exp(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncFloor(TiExcState* exec)
{
    return TiValue::encode(jsNumber(floor(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncLog(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(log(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncMax(TiExcState* exec)
{
    unsigned argsCount = exec->argumentCount();
    double result = -Inf;
    for (unsigned k = 0; k < argsCount; ++k) {
        double val = exec->argument(k).toNumber(exec);
        if (isnan(val)) {
            result = NaN;
            break;
        }
        if (val > result || (val == 0 && result == 0 && !signbit(val)))
            result = val;
    }
    return TiValue::encode(jsNumber(result));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncMin(TiExcState* exec)
{
    unsigned argsCount = exec->argumentCount();
    double result = +Inf;
    for (unsigned k = 0; k < argsCount; ++k) {
        double val = exec->argument(k).toNumber(exec);
        if (isnan(val)) {
            result = NaN;
            break;
        }
        if (val < result || (val == 0 && result == 0 && signbit(val)))
            result = val;
    }
    return TiValue::encode(jsNumber(result));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncPow(TiExcState* exec)
{
    // ECMA 15.8.2.1.13

    double arg = exec->argument(0).toNumber(exec);
    double arg2 = exec->argument(1).toNumber(exec);

    if (isnan(arg2))
        return TiValue::encode(jsNaN());
    if (isinf(arg2) && fabs(arg) == 1)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(pow(arg, arg2)));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncRandom(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(exec->lexicalGlobalObject()->weakRandomNumber()));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncRound(TiExcState* exec)
{
    double arg = exec->argument(0).toNumber(exec);
    double integer = ceil(arg);
    return TiValue::encode(jsNumber(integer - (integer - arg > 0.5)));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncSin(TiExcState* exec)
{
    return TiValue::encode(exec->globalData().cachedSin(exec->argument(0).toNumber(exec)));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncSqrt(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(sqrt(exec->argument(0).toNumber(exec))));
}

EncodedTiValue JSC_HOST_CALL mathProtoFuncTan(TiExcState* exec)
{
    return TiValue::encode(jsDoubleNumber(tan(exec->argument(0).toNumber(exec))));
}

} // namespace TI
