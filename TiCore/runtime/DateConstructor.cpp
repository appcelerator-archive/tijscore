/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
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
#include "DateConstructor.h"

#include "DateConversion.h"
#include "DateInstance.h"
#include "DatePrototype.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "TiString.h"
#include "TiStringBuilder.h"
#include "ObjectPrototype.h"
#include <math.h>
#include <time.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>

#if OS(WINCE) && !PLATFORM(QT)
extern "C" time_t time(time_t* timer); // Provided by libce.
#endif

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if HAVE(SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

using namespace WTI;

namespace TI {

static EncodedTiValue JSC_HOST_CALL dateParse(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateNow(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateUTC(TiExcState*);

}

#include "DateConstructor.lut.h"

namespace TI {

const ClassInfo DateConstructor::s_info = { "Function", &InternalFunction::s_info, 0, TiExcState::dateConstructorTable };

/* Source for DateConstructor.lut.h
@begin dateConstructorTable
  parse     dateParse   DontEnum|Function 1
  UTC       dateUTC     DontEnum|Function 7
  now       dateNow     DontEnum|Function 0
@end
*/

ASSERT_CLASS_FITS_IN_CELL(DateConstructor);

DateConstructor::DateConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, DatePrototype* datePrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, datePrototype->classInfo()->className))
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, datePrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(7), ReadOnly | DontEnum | DontDelete);
}

bool DateConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, TiExcState::dateConstructorTable(exec), this, propertyName, slot);
}

bool DateConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<InternalFunction>(exec, TiExcState::dateConstructorTable(exec), this, propertyName, descriptor);
}

// ECMA 15.9.3
TiObject* constructDate(TiExcState* exec, TiGlobalObject* globalObject, const ArgList& args)
{
    int numArgs = args.size();

    double value;

    if (numArgs == 0) // new Date() ECMA 15.9.3.3
        value = jsCurrentTime();
    else if (numArgs == 1) {
        if (args.at(0).inherits(&DateInstance::s_info))
            value = asDateInstance(args.at(0))->internalNumber();
        else {
            TiValue primitive = args.at(0).toPrimitive(exec);
            if (primitive.isString())
                value = parseDate(exec, primitive.getString(exec));
            else
                value = primitive.toNumber(exec);
        }
    } else {
        double doubleArguments[7] = {
            args.at(0).toNumber(exec), 
            args.at(1).toNumber(exec), 
            args.at(2).toNumber(exec), 
            args.at(3).toNumber(exec), 
            args.at(4).toNumber(exec), 
            args.at(5).toNumber(exec), 
            args.at(6).toNumber(exec)
        };
        if (isnan(doubleArguments[0])
                || isnan(doubleArguments[1])
                || (numArgs >= 3 && isnan(doubleArguments[2]))
                || (numArgs >= 4 && isnan(doubleArguments[3]))
                || (numArgs >= 5 && isnan(doubleArguments[4]))
                || (numArgs >= 6 && isnan(doubleArguments[5]))
                || (numArgs >= 7 && isnan(doubleArguments[6])))
            value = NaN;
        else {
            GregorianDateTime t;
            int year = TI::toInt32(doubleArguments[0]);
            t.year = (year >= 0 && year <= 99) ? year : year - 1900;
            t.month = TI::toInt32(doubleArguments[1]);
            t.monthDay = (numArgs >= 3) ? TI::toInt32(doubleArguments[2]) : 1;
            t.hour = TI::toInt32(doubleArguments[3]);
            t.minute = TI::toInt32(doubleArguments[4]);
            t.second = TI::toInt32(doubleArguments[5]);
            t.isDST = -1;
            double ms = (numArgs >= 7) ? doubleArguments[6] : 0;
            value = gregorianDateTimeToMS(exec, t, ms, false);
        }
    }

    return new (exec) DateInstance(exec, globalObject->dateStructure(), value);
}
    
static EncodedTiValue JSC_HOST_CALL constructWithDateConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructDate(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

ConstructType DateConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithDateConstructor;
    return ConstructTypeHost;
}

// ECMA 15.9.2
static EncodedTiValue JSC_HOST_CALL callDate(TiExcState* exec)
{
    time_t localTime = time(0);
    tm localTM;
    getLocalTime(&localTime, &localTM);
    GregorianDateTime ts(exec, localTM);
    DateConversionBuffer date;
    DateConversionBuffer time;
    formatDate(ts, date);
    formatTime(ts, time);
    return TiValue::encode(jsMakeNontrivialString(exec, date, " ", time));
}

CallType DateConstructor::getCallData(CallData& callData)
{
    callData.native.function = callDate;
    return CallTypeHost;
}

static EncodedTiValue JSC_HOST_CALL dateParse(TiExcState* exec)
{
    return TiValue::encode(jsNumber(parseDate(exec, exec->argument(0).toString(exec))));
}

static EncodedTiValue JSC_HOST_CALL dateNow(TiExcState*)
{
    return TiValue::encode(jsNumber(jsCurrentTime()));
}

static EncodedTiValue JSC_HOST_CALL dateUTC(TiExcState* exec) 
{
    double doubleArguments[7] = {
        exec->argument(0).toNumber(exec), 
        exec->argument(1).toNumber(exec), 
        exec->argument(2).toNumber(exec), 
        exec->argument(3).toNumber(exec), 
        exec->argument(4).toNumber(exec), 
        exec->argument(5).toNumber(exec), 
        exec->argument(6).toNumber(exec)
    };
    int n = exec->argumentCount();
    if (isnan(doubleArguments[0])
            || isnan(doubleArguments[1])
            || (n >= 3 && isnan(doubleArguments[2]))
            || (n >= 4 && isnan(doubleArguments[3]))
            || (n >= 5 && isnan(doubleArguments[4]))
            || (n >= 6 && isnan(doubleArguments[5]))
            || (n >= 7 && isnan(doubleArguments[6])))
        return TiValue::encode(jsNaN());

    GregorianDateTime t;
    int year = TI::toInt32(doubleArguments[0]);
    t.year = (year >= 0 && year <= 99) ? year : year - 1900;
    t.month = TI::toInt32(doubleArguments[1]);
    t.monthDay = (n >= 3) ? TI::toInt32(doubleArguments[2]) : 1;
    t.hour = TI::toInt32(doubleArguments[3]);
    t.minute = TI::toInt32(doubleArguments[4]);
    t.second = TI::toInt32(doubleArguments[5]);
    double ms = (n >= 7) ? doubleArguments[6] : 0;
    return TiValue::encode(jsNumber(timeClip(gregorianDateTimeToMS(exec, t, ms, true))));
}

} // namespace TI
