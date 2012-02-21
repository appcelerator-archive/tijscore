/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2008, 2009 Torch Mobile, Inc. All rights reserved.
 *  Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "DatePrototype.h"

#include "DateConversion.h"
#include "DateInstance.h"
#include "Error.h"
#include "TiString.h"
#include "TiStringBuilder.h"
#include "Lookup.h"
#include "ObjectPrototype.h"

#if !PLATFORM(MAC) && HAVE(LANGINFO_H)
#include <langinfo.h>
#endif

#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <wtf/Assertions.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/UnusedParam.h>

#if HAVE(SYS_PARAM_H)
#include <sys/param.h>
#endif

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if HAVE(SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#include <CoreFoundation/CoreFoundation.h>

#if OS(WINCE) && !PLATFORM(QT)
extern "C" size_t strftime(char * const s, const size_t maxsize, const char * const format, const struct tm * const t); //provided by libce
#endif

using namespace WTI;

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(DatePrototype);

static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetDate(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetDay(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetFullYear(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetHours(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetMilliSeconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetMinutes(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetMonth(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetSeconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetTime(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetTimezoneOffset(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCDate(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCDay(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCFullYear(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCHours(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCMilliseconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCMinutes(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCMonth(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCSeconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncGetYear(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetDate(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetFullYear(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetHours(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetMilliSeconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetMinutes(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetMonth(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetSeconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetTime(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCDate(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCFullYear(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCHours(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCMilliseconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCMinutes(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCMonth(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCSeconds(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncSetYear(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToDateString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToGMTString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToLocaleDateString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToLocaleString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToLocaleTimeString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToTimeString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToUTCString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToISOString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL dateProtoFuncToJSON(TiExcState*);

}

#include "DatePrototype.lut.h"

namespace TI {

enum LocaleDateTimeFormat { LocaleDateAndTime, LocaleDate, LocaleTime };
 

// FIXME: Since this is superior to the strftime-based version, why limit this to PLATFORM(MAC)?
// Instead we should consider using this whenever USE(CF) is true.

static CFDateFormatterStyle styleFromArgString(const UString& string, CFDateFormatterStyle defaultStyle)
{
    if (string == "short")
        return kCFDateFormatterShortStyle;
    if (string == "medium")
        return kCFDateFormatterMediumStyle;
    if (string == "long")
        return kCFDateFormatterLongStyle;
    if (string == "full")
        return kCFDateFormatterFullStyle;
    return defaultStyle;
}

static TiCell* formatLocaleDate(TiExcState* exec, DateInstance*, double timeInMilliseconds, LocaleDateTimeFormat format)
{
    CFDateFormatterStyle dateStyle = (format != LocaleTime ? kCFDateFormatterLongStyle : kCFDateFormatterNoStyle);
    CFDateFormatterStyle timeStyle = (format != LocaleDate ? kCFDateFormatterLongStyle : kCFDateFormatterNoStyle);

    bool useCustomFormat = false;
    UString customFormatString;

    UString arg0String = exec->argument(0).toString(exec);
    if (arg0String == "custom" && !exec->argument(1).isUndefined()) {
        useCustomFormat = true;
        customFormatString = exec->argument(1).toString(exec);
    } else if (format == LocaleDateAndTime && !exec->argument(1).isUndefined()) {
        dateStyle = styleFromArgString(arg0String, dateStyle);
        timeStyle = styleFromArgString(exec->argument(1).toString(exec), timeStyle);
    } else if (format != LocaleTime && !exec->argument(0).isUndefined())
        dateStyle = styleFromArgString(arg0String, dateStyle);
    else if (format != LocaleDate && !exec->argument(0).isUndefined())
        timeStyle = styleFromArgString(arg0String, timeStyle);

    CFLocaleRef locale = CFLocaleCopyCurrent();
    CFDateFormatterRef formatter = CFDateFormatterCreate(0, locale, dateStyle, timeStyle);
    CFRelease(locale);

    if (useCustomFormat) {
        CFStringRef customFormatCFString = CFStringCreateWithCharacters(0, customFormatString.characters(), customFormatString.length());
        CFDateFormatterSetFormat(formatter, customFormatCFString);
        CFRelease(customFormatCFString);
    }

    CFStringRef string = CFDateFormatterCreateStringWithAbsoluteTime(0, formatter, floor(timeInMilliseconds / msPerSecond) - kCFAbsoluteTimeIntervalSince1970);

    CFRelease(formatter);

    // We truncate the string returned from CFDateFormatter if it's absurdly long (> 200 characters).
    // That's not great error handling, but it just won't happen so it doesn't matter.
    UChar buffer[200];
    const size_t bufferLength = WTF_ARRAY_LENGTH(buffer);
    size_t length = CFStringGetLength(string);
    ASSERT(length <= bufferLength);
    if (length > bufferLength)
        length = bufferLength;
    CFStringGetCharacters(string, CFRangeMake(0, length), buffer);

    CFRelease(string);

    return jsNontrivialString(exec, UString(buffer, length));
}


// Converts a list of arguments sent to a Date member function into milliseconds, updating
// ms (representing milliseconds) and t (representing the rest of the date structure) appropriately.
//
// Format of member function: f([hour,] [min,] [sec,] [ms])
static bool fillStructuresUsingTimeArgs(TiExcState* exec, int maxArgs, double* ms, GregorianDateTime* t)
{
    double milliseconds = 0;
    bool ok = true;
    int idx = 0;
    int numArgs = exec->argumentCount();
    
    // JS allows extra trailing arguments -- ignore them
    if (numArgs > maxArgs)
        numArgs = maxArgs;

    // hours
    if (maxArgs >= 4 && idx < numArgs) {
        t->hour = 0;
        double hours = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = isfinite(hours);
        milliseconds += hours * msPerHour;
    }

    // minutes
    if (maxArgs >= 3 && idx < numArgs && ok) {
        t->minute = 0;
        double minutes = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = isfinite(minutes);
        milliseconds += minutes * msPerMinute;
    }
    
    // seconds
    if (maxArgs >= 2 && idx < numArgs && ok) {
        t->second = 0;
        double seconds = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = isfinite(seconds);
        milliseconds += seconds * msPerSecond;
    }
    
    if (!ok)
        return false;
        
    // milliseconds
    if (idx < numArgs) {
        double millis = exec->argument(idx).toIntegerPreserveNaN(exec);
        ok = isfinite(millis);
        milliseconds += millis;
    } else
        milliseconds += *ms;
    
    *ms = milliseconds;
    return ok;
}

// Converts a list of arguments sent to a Date member function into years, months, and milliseconds, updating
// ms (representing milliseconds) and t (representing the rest of the date structure) appropriately.
//
// Format of member function: f([years,] [months,] [days])
static bool fillStructuresUsingDateArgs(TiExcState *exec, int maxArgs, double *ms, GregorianDateTime *t)
{
    int idx = 0;
    bool ok = true;
    int numArgs = exec->argumentCount();
  
    // JS allows extra trailing arguments -- ignore them
    if (numArgs > maxArgs)
        numArgs = maxArgs;
  
    // years
    if (maxArgs >= 3 && idx < numArgs) {
        double years = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = isfinite(years);
        t->year = toInt32(years - 1900);
    }
    // months
    if (maxArgs >= 2 && idx < numArgs && ok) {
        double months = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = isfinite(months);
        t->month = toInt32(months);
    }
    // days
    if (idx < numArgs && ok) {
        double days = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = isfinite(days);
        t->monthDay = 0;
        *ms += days * msPerDay;
    }
    
    return ok;
}

const ClassInfo DatePrototype::s_info = {"Date", &DateInstance::s_info, 0, TiExcState::dateTable};

/* Source for DatePrototype.lut.h
@begin dateTable
  toString              dateProtoFuncToString                DontEnum|Function       0
  toISOString           dateProtoFuncToISOString             DontEnum|Function       0
  toUTCString           dateProtoFuncToUTCString             DontEnum|Function       0
  toDateString          dateProtoFuncToDateString            DontEnum|Function       0
  toTimeString          dateProtoFuncToTimeString            DontEnum|Function       0
  toLocaleString        dateProtoFuncToLocaleString          DontEnum|Function       0
  toLocaleDateString    dateProtoFuncToLocaleDateString      DontEnum|Function       0
  toLocaleTimeString    dateProtoFuncToLocaleTimeString      DontEnum|Function       0
  valueOf               dateProtoFuncGetTime                 DontEnum|Function       0
  getTime               dateProtoFuncGetTime                 DontEnum|Function       0
  getFullYear           dateProtoFuncGetFullYear             DontEnum|Function       0
  getUTCFullYear        dateProtoFuncGetUTCFullYear          DontEnum|Function       0
  toGMTString           dateProtoFuncToGMTString             DontEnum|Function       0
  getMonth              dateProtoFuncGetMonth                DontEnum|Function       0
  getUTCMonth           dateProtoFuncGetUTCMonth             DontEnum|Function       0
  getDate               dateProtoFuncGetDate                 DontEnum|Function       0
  getUTCDate            dateProtoFuncGetUTCDate              DontEnum|Function       0
  getDay                dateProtoFuncGetDay                  DontEnum|Function       0
  getUTCDay             dateProtoFuncGetUTCDay               DontEnum|Function       0
  getHours              dateProtoFuncGetHours                DontEnum|Function       0
  getUTCHours           dateProtoFuncGetUTCHours             DontEnum|Function       0
  getMinutes            dateProtoFuncGetMinutes              DontEnum|Function       0
  getUTCMinutes         dateProtoFuncGetUTCMinutes           DontEnum|Function       0
  getSeconds            dateProtoFuncGetSeconds              DontEnum|Function       0
  getUTCSeconds         dateProtoFuncGetUTCSeconds           DontEnum|Function       0
  getMilliseconds       dateProtoFuncGetMilliSeconds         DontEnum|Function       0
  getUTCMilliseconds    dateProtoFuncGetUTCMilliseconds      DontEnum|Function       0
  getTimezoneOffset     dateProtoFuncGetTimezoneOffset       DontEnum|Function       0
  setTime               dateProtoFuncSetTime                 DontEnum|Function       1
  setMilliseconds       dateProtoFuncSetMilliSeconds         DontEnum|Function       1
  setUTCMilliseconds    dateProtoFuncSetUTCMilliseconds      DontEnum|Function       1
  setSeconds            dateProtoFuncSetSeconds              DontEnum|Function       2
  setUTCSeconds         dateProtoFuncSetUTCSeconds           DontEnum|Function       2
  setMinutes            dateProtoFuncSetMinutes              DontEnum|Function       3
  setUTCMinutes         dateProtoFuncSetUTCMinutes           DontEnum|Function       3
  setHours              dateProtoFuncSetHours                DontEnum|Function       4
  setUTCHours           dateProtoFuncSetUTCHours             DontEnum|Function       4
  setDate               dateProtoFuncSetDate                 DontEnum|Function       1
  setUTCDate            dateProtoFuncSetUTCDate              DontEnum|Function       1
  setMonth              dateProtoFuncSetMonth                DontEnum|Function       2
  setUTCMonth           dateProtoFuncSetUTCMonth             DontEnum|Function       2
  setFullYear           dateProtoFuncSetFullYear             DontEnum|Function       3
  setUTCFullYear        dateProtoFuncSetUTCFullYear          DontEnum|Function       3
  setYear               dateProtoFuncSetYear                 DontEnum|Function       1
  getYear               dateProtoFuncGetYear                 DontEnum|Function       0
  toJSON                dateProtoFuncToJSON                  DontEnum|Function       1
@end
*/

// ECMA 15.9.4

DatePrototype::DatePrototype(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure)
    : DateInstance(exec, structure)
{
    ASSERT(inherits(&s_info));

    // The constructor will be added later, after DateConstructor has been built.
    putAnonymousValue(exec->globalData(), 0, globalObject);
}

bool DatePrototype::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<TiObject>(exec, TiExcState::dateTable(exec), this, propertyName, slot);
}


bool DatePrototype::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<TiObject>(exec, TiExcState::dateTable(exec), this, propertyName, descriptor);
}

// Functions

EncodedTiValue JSC_HOST_CALL dateProtoFuncToString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNontrivialString(exec, "Invalid Date"));
    DateConversionBuffer date;
    DateConversionBuffer time;
    formatDate(*gregorianDateTime, date);
    formatTime(*gregorianDateTime, time);
    return TiValue::encode(jsMakeNontrivialString(exec, date, " ", time));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToUTCString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNontrivialString(exec, "Invalid Date"));
    DateConversionBuffer date;
    DateConversionBuffer time;
    formatDateUTCVariant(*gregorianDateTime, date);
    formatTimeUTC(*gregorianDateTime, time);
    return TiValue::encode(jsMakeNontrivialString(exec, date, " ", time));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToISOString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);
    
    DateInstance* thisDateObj = asDateInstance(thisValue); 
    
    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNontrivialString(exec, "Invalid Date"));
    // Maximum amount of space we need in buffer: 6 (max. digits in year) + 2 * 5 (2 characters each for month, day, hour, minute, second) + 4 (. + 3 digits for milliseconds)
    // 6 for formatting and one for null termination = 27.  We add one extra character to allow us to force null termination.
    char buffer[28];
    snprintf(buffer, sizeof(buffer) - 1, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", 1900 + gregorianDateTime->year, gregorianDateTime->month + 1, gregorianDateTime->monthDay, gregorianDateTime->hour, gregorianDateTime->minute, gregorianDateTime->second, static_cast<int>(fmod(thisDateObj->internalNumber(), 1000)));
    buffer[sizeof(buffer) - 1] = 0;
    return TiValue::encode(jsNontrivialString(exec, buffer));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToDateString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNontrivialString(exec, "Invalid Date"));
    DateConversionBuffer date;
    formatDate(*gregorianDateTime, date);
    return TiValue::encode(jsNontrivialString(exec, date));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToTimeString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNontrivialString(exec, "Invalid Date"));
    DateConversionBuffer time;
    formatTime(*gregorianDateTime, time);
    return TiValue::encode(jsNontrivialString(exec, time));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToLocaleString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return TiValue::encode(formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleDateAndTime));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToLocaleDateString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return TiValue::encode(formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleDate));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToLocaleTimeString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return TiValue::encode(formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleTime));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetTime(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    return TiValue::encode(asDateInstance(thisValue)->internalValue());
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetFullYear(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(1900 + gregorianDateTime->year));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCFullYear(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(1900 + gregorianDateTime->year));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToGMTString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNontrivialString(exec, "Invalid Date"));
    DateConversionBuffer date;
    DateConversionBuffer time;
    formatDateUTCVariant(*gregorianDateTime, date);
    formatTimeUTC(*gregorianDateTime, time);
    return TiValue::encode(jsMakeNontrivialString(exec, date, " ", time));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetMonth(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->month));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCMonth(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->month));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetDate(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->monthDay));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCDate(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->monthDay));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetDay(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->weekDay));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCDay(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->weekDay));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetHours(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->hour));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCHours(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->hour));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetMinutes(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->minute));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCMinutes(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->minute));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetSeconds(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->second));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCSeconds(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(gregorianDateTime->second));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetMilliSeconds(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    double milli = thisDateObj->internalNumber();
    if (isnan(milli))
        return TiValue::encode(jsNaN());

    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;
    return TiValue::encode(jsNumber(ms));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetUTCMilliseconds(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    double milli = thisDateObj->internalNumber();
    if (isnan(milli))
        return TiValue::encode(jsNaN());

    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;
    return TiValue::encode(jsNumber(ms));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetTimezoneOffset(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());
    return TiValue::encode(jsNumber(-gregorianDateTime->utcOffset / minutesPerHour));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetTime(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    double milli = timeClip(exec->argument(0).toNumber(exec));
    TiValue result = jsNumber(milli);
    thisDateObj->setInternalValue(exec->globalData(), result);
    return TiValue::encode(result);
}

static EncodedTiValue setNewValueFromTimeArgs(TiExcState* exec, int numArgsToUse, bool inputIsUTC)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    double milli = thisDateObj->internalNumber();
    
    if (!exec->argumentCount() || isnan(milli)) {
        TiValue result = jsNaN();
        thisDateObj->setInternalValue(exec->globalData(), result);
        return TiValue::encode(result);
    }
     
    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;

    const GregorianDateTime* other = inputIsUTC 
        ? thisDateObj->gregorianDateTimeUTC(exec)
        : thisDateObj->gregorianDateTime(exec);
    if (!other)
        return TiValue::encode(jsNaN());

    GregorianDateTime gregorianDateTime;
    gregorianDateTime.copyFrom(*other);
    if (!fillStructuresUsingTimeArgs(exec, numArgsToUse, &ms, &gregorianDateTime)) {
        TiValue result = jsNaN();
        thisDateObj->setInternalValue(exec->globalData(), result);
        return TiValue::encode(result);
    } 
    
    TiValue result = jsNumber(gregorianDateTimeToMS(exec, gregorianDateTime, ms, inputIsUTC));
    thisDateObj->setInternalValue(exec->globalData(), result);
    return TiValue::encode(result);
}

static EncodedTiValue setNewValueFromDateArgs(TiExcState* exec, int numArgsToUse, bool inputIsUTC)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    if (!exec->argumentCount()) {
        TiValue result = jsNaN();
        thisDateObj->setInternalValue(exec->globalData(), result);
        return TiValue::encode(result);
    }      
    
    double milli = thisDateObj->internalNumber();
    double ms = 0; 

    GregorianDateTime gregorianDateTime; 
    if (numArgsToUse == 3 && isnan(milli)) 
        msToGregorianDateTime(exec, 0, true, gregorianDateTime); 
    else { 
        ms = milli - floor(milli / msPerSecond) * msPerSecond; 
        const GregorianDateTime* other = inputIsUTC 
            ? thisDateObj->gregorianDateTimeUTC(exec)
            : thisDateObj->gregorianDateTime(exec);
        if (!other)
            return TiValue::encode(jsNaN());
        gregorianDateTime.copyFrom(*other);
    }
    
    if (!fillStructuresUsingDateArgs(exec, numArgsToUse, &ms, &gregorianDateTime)) {
        TiValue result = jsNaN();
        thisDateObj->setInternalValue(exec->globalData(), result);
        return TiValue::encode(result);
    } 
           
    TiValue result = jsNumber(gregorianDateTimeToMS(exec, gregorianDateTime, ms, inputIsUTC));
    thisDateObj->setInternalValue(exec->globalData(), result);
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetMilliSeconds(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 1, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCMilliseconds(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 1, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetSeconds(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 2, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCSeconds(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 2, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetMinutes(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 3, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCMinutes(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 3, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetHours(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 4, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCHours(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 4, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetDate(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, 1, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCDate(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, 1, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetMonth(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, 2, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCMonth(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, 2, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetFullYear(TiExcState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, 3, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetUTCFullYear(TiExcState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, 3, inputIsUTC);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncSetYear(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);     
    if (!exec->argumentCount()) { 
        TiValue result = jsNaN();
        thisDateObj->setInternalValue(exec->globalData(), result);
        return TiValue::encode(result);
    }
    
    double milli = thisDateObj->internalNumber();
    double ms = 0;

    GregorianDateTime gregorianDateTime;
    if (isnan(milli))
        // Based on ECMA 262 B.2.5 (setYear)
        // the time must be reset to +0 if it is NaN. 
        msToGregorianDateTime(exec, 0, true, gregorianDateTime);
    else {   
        double secs = floor(milli / msPerSecond);
        ms = milli - secs * msPerSecond;
        if (const GregorianDateTime* other = thisDateObj->gregorianDateTime(exec))
            gregorianDateTime.copyFrom(*other);
    }
    
    double year = exec->argument(0).toIntegerPreserveNaN(exec);
    if (!isfinite(year)) {
        TiValue result = jsNaN();
        thisDateObj->setInternalValue(exec->globalData(), result);
        return TiValue::encode(result);
    }
            
    gregorianDateTime.year = toInt32((year > 99 || year < 0) ? year - 1900 : year);
    TiValue result = jsNumber(gregorianDateTimeToMS(exec, gregorianDateTime, ms, false));
    thisDateObj->setInternalValue(exec->globalData(), result);
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncGetYear(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return TiValue::encode(jsNaN());

    // NOTE: IE returns the full year even in getYear.
    return TiValue::encode(jsNumber(gregorianDateTime->year));
}

EncodedTiValue JSC_HOST_CALL dateProtoFuncToJSON(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    TiObject* object = thisValue.toThisObject(exec);
    if (exec->hadException())
        return TiValue::encode(jsNull());
    
    TiValue toISOValue = object->get(exec, exec->globalData().propertyNames->toISOString);
    if (exec->hadException())
        return TiValue::encode(jsNull());

    CallData callData;
    CallType callType = getCallData(toISOValue, callData);
    if (callType == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, "toISOString is not a function"));

    TiValue result = call(exec, asObject(toISOValue), callType, callData, object, exec->emptyList());
    if (exec->hadException())
        return TiValue::encode(jsNull());
    if (result.isObject())
        return throwVMError(exec, createTypeError(exec, "toISOString did not return a primitive value"));
    return TiValue::encode(result);
}

} // namespace TI
