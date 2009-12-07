/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2008, 2009 Torch Mobile, Inc. All rights reserved.
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
#include "Error.h"
#include "TiString.h"
#include "ObjectPrototype.h"
#include "DateInstance.h"
#include <float.h>

#if !PLATFORM(MAC) && HAVE(LANGINFO_H)
#include <langinfo.h>
#endif

#include <limits.h>
#include <locale.h>
#include <math.h>
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

#if PLATFORM(MAC)
#include <CoreFoundation/CoreFoundation.h>
#endif

#if PLATFORM(WINCE) && !PLATFORM(QT)
extern "C" size_t strftime(char * const s, const size_t maxsize, const char * const format, const struct tm * const t); //provided by libce
#endif

using namespace WTI;

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(DatePrototype);

static TiValue JSC_HOST_CALL dateProtoFuncGetDate(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetDay(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetFullYear(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetHours(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetMilliSeconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetMinutes(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetMonth(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetSeconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetTime(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetTimezoneOffset(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCDate(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCDay(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCFullYear(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCHours(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCMilliseconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCMinutes(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCMonth(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetUTCSeconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncGetYear(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetDate(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetFullYear(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetHours(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetMilliSeconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetMinutes(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetMonth(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetSeconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetTime(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCDate(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCFullYear(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCHours(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCMilliseconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCMinutes(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCMonth(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetUTCSeconds(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncSetYear(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToDateString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToGMTString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToLocaleDateString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToLocaleString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToLocaleTimeString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToTimeString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToUTCString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL dateProtoFuncToISOString(TiExcState*, TiObject*, TiValue, const ArgList&);

static TiValue JSC_HOST_CALL dateProtoFuncToJSON(TiExcState*, TiObject*, TiValue, const ArgList&);

}

#include "DatePrototype.lut.h"

namespace TI {

enum LocaleDateTimeFormat { LocaleDateAndTime, LocaleDate, LocaleTime };
 
#if PLATFORM(MAC)

// FIXME: Since this is superior to the strftime-based version, why limit this to PLATFORM(MAC)?
// Instead we should consider using this whenever PLATFORM(CF) is true.

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

static TiCell* formatLocaleDate(TiExcState* exec, DateInstance*, double timeInMilliseconds, LocaleDateTimeFormat format, const ArgList& args)
{
    CFDateFormatterStyle dateStyle = (format != LocaleTime ? kCFDateFormatterLongStyle : kCFDateFormatterNoStyle);
    CFDateFormatterStyle timeStyle = (format != LocaleDate ? kCFDateFormatterLongStyle : kCFDateFormatterNoStyle);

    bool useCustomFormat = false;
    UString customFormatString;

    UString arg0String = args.at(0).toString(exec);
    if (arg0String == "custom" && !args.at(1).isUndefined()) {
        useCustomFormat = true;
        customFormatString = args.at(1).toString(exec);
    } else if (format == LocaleDateAndTime && !args.at(1).isUndefined()) {
        dateStyle = styleFromArgString(arg0String, dateStyle);
        timeStyle = styleFromArgString(args.at(1).toString(exec), timeStyle);
    } else if (format != LocaleTime && !args.at(0).isUndefined())
        dateStyle = styleFromArgString(arg0String, dateStyle);
    else if (format != LocaleDate && !args.at(0).isUndefined())
        timeStyle = styleFromArgString(arg0String, timeStyle);

    CFLocaleRef locale = CFLocaleCopyCurrent();
    CFDateFormatterRef formatter = CFDateFormatterCreate(0, locale, dateStyle, timeStyle);
    CFRelease(locale);

    if (useCustomFormat) {
        CFStringRef customFormatCFString = CFStringCreateWithCharacters(0, customFormatString.data(), customFormatString.size());
        CFDateFormatterSetFormat(formatter, customFormatCFString);
        CFRelease(customFormatCFString);
    }

    CFStringRef string = CFDateFormatterCreateStringWithAbsoluteTime(0, formatter, floor(timeInMilliseconds / msPerSecond) - kCFAbsoluteTimeIntervalSince1970);

    CFRelease(formatter);

    // We truncate the string returned from CFDateFormatter if it's absurdly long (> 200 characters).
    // That's not great error handling, but it just won't happen so it doesn't matter.
    UChar buffer[200];
    const size_t bufferLength = sizeof(buffer) / sizeof(buffer[0]);
    size_t length = CFStringGetLength(string);
    ASSERT(length <= bufferLength);
    if (length > bufferLength)
        length = bufferLength;
    CFStringGetCharacters(string, CFRangeMake(0, length), buffer);

    CFRelease(string);

    return jsNontrivialString(exec, UString(buffer, length));
}

#else // !PLATFORM(MAC)

static TiCell* formatLocaleDate(TiExcState* exec, const GregorianDateTime& gdt, LocaleDateTimeFormat format)
{
#if HAVE(LANGINFO_H)
    static const nl_item formats[] = { D_T_FMT, D_FMT, T_FMT };
#elif (PLATFORM(WINCE) && !PLATFORM(QT)) || PLATFORM(SYMBIAN)
     // strftime() does not support '#' on WinCE or Symbian
    static const char* const formatStrings[] = { "%c", "%x", "%X" };
#else
    static const char* const formatStrings[] = { "%#c", "%#x", "%X" };
#endif
 
    // Offset year if needed
    struct tm localTM = gdt;
    int year = gdt.year + 1900;
    bool yearNeedsOffset = year < 1900 || year > 2038;
    if (yearNeedsOffset)
        localTM.tm_year = equivalentYearForDST(year) - 1900;
 
#if HAVE(LANGINFO_H)
    // We do not allow strftime to generate dates with 2-digits years,
    // both to avoid ambiguity, and a crash in strncpy, for years that
    // need offset.
    char* formatString = strdup(nl_langinfo(formats[format]));
    char* yPos = strchr(formatString, 'y');
    if (yPos)
        *yPos = 'Y';
#endif

    // Do the formatting
    const int bufsize = 128;
    char timebuffer[bufsize];

#if HAVE(LANGINFO_H)
    size_t ret = strftime(timebuffer, bufsize, formatString, &localTM);
    free(formatString);
#else
    size_t ret = strftime(timebuffer, bufsize, formatStrings[format], &localTM);
#endif
 
    if (ret == 0)
        return jsEmptyString(exec);
 
    // Copy original into the buffer
    if (yearNeedsOffset && format != LocaleTime) {
        static const int yearLen = 5;   // FIXME will be a problem in the year 10,000
        char yearString[yearLen];
 
        snprintf(yearString, yearLen, "%d", localTM.tm_year + 1900);
        char* yearLocation = strstr(timebuffer, yearString);
        snprintf(yearString, yearLen, "%d", year);
 
        strncpy(yearLocation, yearString, yearLen - 1);
    }
 
    return jsNontrivialString(exec, timebuffer);
}

static TiCell* formatLocaleDate(TiExcState* exec, DateInstance* dateObject, double, LocaleDateTimeFormat format, const ArgList&)
{
    const GregorianDateTime* gregorianDateTime = dateObject->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    return formatLocaleDate(exec, *gregorianDateTime, format);
}

#endif // !PLATFORM(MAC)

// Converts a list of arguments sent to a Date member function into milliseconds, updating
// ms (representing milliseconds) and t (representing the rest of the date structure) appropriately.
//
// Format of member function: f([hour,] [min,] [sec,] [ms])
static bool fillStructuresUsingTimeArgs(TiExcState* exec, const ArgList& args, int maxArgs, double* ms, GregorianDateTime* t)
{
    double milliseconds = 0;
    bool ok = true;
    int idx = 0;
    int numArgs = args.size();
    
    // JS allows extra trailing arguments -- ignore them
    if (numArgs > maxArgs)
        numArgs = maxArgs;

    // hours
    if (maxArgs >= 4 && idx < numArgs) {
        t->hour = 0;
        milliseconds += args.at(idx++).toInt32(exec, ok) * msPerHour;
    }

    // minutes
    if (maxArgs >= 3 && idx < numArgs && ok) {
        t->minute = 0;
        milliseconds += args.at(idx++).toInt32(exec, ok) * msPerMinute;
    }
    
    // seconds
    if (maxArgs >= 2 && idx < numArgs && ok) {
        t->second = 0;
        milliseconds += args.at(idx++).toInt32(exec, ok) * msPerSecond;
    }
    
    if (!ok)
        return false;
        
    // milliseconds
    if (idx < numArgs) {
        double millis = args.at(idx).toNumber(exec);
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
static bool fillStructuresUsingDateArgs(TiExcState *exec, const ArgList& args, int maxArgs, double *ms, GregorianDateTime *t)
{
    int idx = 0;
    bool ok = true;
    int numArgs = args.size();
  
    // JS allows extra trailing arguments -- ignore them
    if (numArgs > maxArgs)
        numArgs = maxArgs;
  
    // years
    if (maxArgs >= 3 && idx < numArgs)
        t->year = args.at(idx++).toInt32(exec, ok) - 1900;
    
    // months
    if (maxArgs >= 2 && idx < numArgs && ok)   
        t->month = args.at(idx++).toInt32(exec, ok);
    
    // days
    if (idx < numArgs && ok) {   
        t->monthDay = 0;
        *ms += args.at(idx).toInt32(exec, ok) * msPerDay;
    }
    
    return ok;
}

const ClassInfo DatePrototype::info = {"Date", &DateInstance::info, 0, TiExcState::dateTable};

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
  toJSON                dateProtoFuncToJSON                  DontEnum|Function       0
@end
*/

// ECMA 15.9.4

DatePrototype::DatePrototype(TiExcState* exec, NonNullPassRefPtr<Structure> structure)
    : DateInstance(exec, structure)
{
    // The constructor will be added later, after DateConstructor has been built.
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

TiValue JSC_HOST_CALL dateProtoFuncToString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    return jsNontrivialString(exec, formatDate(*gregorianDateTime) + " " + formatTime(*gregorianDateTime));
}

TiValue JSC_HOST_CALL dateProtoFuncToUTCString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    return jsNontrivialString(exec, formatDateUTCVariant(*gregorianDateTime) + " " + formatTimeUTC(*gregorianDateTime));
}

TiValue JSC_HOST_CALL dateProtoFuncToISOString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);
    
    DateInstance* thisDateObj = asDateInstance(thisValue); 
    
    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    // Maximum amount of space we need in buffer: 6 (max. digits in year) + 2 * 5 (2 characters each for month, day, hour, minute, second) + 4 (. + 3 digits for milliseconds)
    // 6 for formatting and one for null termination = 27.  We add one extra character to allow us to force null termination.
    char buffer[28];
    snprintf(buffer, sizeof(buffer) - 1, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", 1900 + gregorianDateTime->year, gregorianDateTime->month + 1, gregorianDateTime->monthDay, gregorianDateTime->hour, gregorianDateTime->minute, gregorianDateTime->second, static_cast<int>(fmod(thisDateObj->internalNumber(), 1000)));
    buffer[sizeof(buffer) - 1] = 0;
    return jsNontrivialString(exec, buffer);
}

TiValue JSC_HOST_CALL dateProtoFuncToDateString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    return jsNontrivialString(exec, formatDate(*gregorianDateTime));
}

TiValue JSC_HOST_CALL dateProtoFuncToTimeString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    return jsNontrivialString(exec, formatTime(*gregorianDateTime));
}

TiValue JSC_HOST_CALL dateProtoFuncToLocaleString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleDateAndTime, args);
}

TiValue JSC_HOST_CALL dateProtoFuncToLocaleDateString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleDate, args);
}

TiValue JSC_HOST_CALL dateProtoFuncToLocaleTimeString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleTime, args);
}

TiValue JSC_HOST_CALL dateProtoFuncGetTime(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    return asDateInstance(thisValue)->internalValue(); 
}

TiValue JSC_HOST_CALL dateProtoFuncGetFullYear(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, 1900 + gregorianDateTime->year);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCFullYear(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, 1900 + gregorianDateTime->year);
}

TiValue JSC_HOST_CALL dateProtoFuncToGMTString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, "Invalid Date");
    return jsNontrivialString(exec, formatDateUTCVariant(*gregorianDateTime) + " " + formatTimeUTC(*gregorianDateTime));
}

TiValue JSC_HOST_CALL dateProtoFuncGetMonth(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->month);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCMonth(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->month);
}

TiValue JSC_HOST_CALL dateProtoFuncGetDate(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->monthDay);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCDate(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->monthDay);
}

TiValue JSC_HOST_CALL dateProtoFuncGetDay(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->weekDay);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCDay(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->weekDay);
}

TiValue JSC_HOST_CALL dateProtoFuncGetHours(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->hour);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCHours(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->hour);
}

TiValue JSC_HOST_CALL dateProtoFuncGetMinutes(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->minute);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCMinutes(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->minute);
}

TiValue JSC_HOST_CALL dateProtoFuncGetSeconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->second);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCSeconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, gregorianDateTime->second);
}

TiValue JSC_HOST_CALL dateProtoFuncGetMilliSeconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    double milli = thisDateObj->internalNumber();
    if (isnan(milli))
        return jsNaN(exec);

    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;
    return jsNumber(exec, ms);
}

TiValue JSC_HOST_CALL dateProtoFuncGetUTCMilliseconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    double milli = thisDateObj->internalNumber();
    if (isnan(milli))
        return jsNaN(exec);

    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;
    return jsNumber(exec, ms);
}

TiValue JSC_HOST_CALL dateProtoFuncGetTimezoneOffset(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);
    return jsNumber(exec, -gregorianDateTime->utcOffset / minutesPerHour);
}

TiValue JSC_HOST_CALL dateProtoFuncSetTime(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    double milli = timeClip(args.at(0).toNumber(exec));
    TiValue result = jsNumber(exec, milli);
    thisDateObj->setInternalValue(result);
    return result;
}

static TiValue setNewValueFromTimeArgs(TiExcState* exec, TiValue thisValue, const ArgList& args, int numArgsToUse, bool inputIsUTC)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    double milli = thisDateObj->internalNumber();
    
    if (args.isEmpty() || isnan(milli)) {
        TiValue result = jsNaN(exec);
        thisDateObj->setInternalValue(result);
        return result;
    }
     
    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;

    const GregorianDateTime* other = inputIsUTC 
        ? thisDateObj->gregorianDateTimeUTC(exec)
        : thisDateObj->gregorianDateTime(exec);
    if (!other)
        return jsNaN(exec);

    GregorianDateTime gregorianDateTime;
    gregorianDateTime.copyFrom(*other);
    if (!fillStructuresUsingTimeArgs(exec, args, numArgsToUse, &ms, &gregorianDateTime)) {
        TiValue result = jsNaN(exec);
        thisDateObj->setInternalValue(result);
        return result;
    } 
    
    TiValue result = jsNumber(exec, gregorianDateTimeToMS(exec, gregorianDateTime, ms, inputIsUTC));
    thisDateObj->setInternalValue(result);
    return result;
}

static TiValue setNewValueFromDateArgs(TiExcState* exec, TiValue thisValue, const ArgList& args, int numArgsToUse, bool inputIsUTC)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    if (args.isEmpty()) {
        TiValue result = jsNaN(exec);
        thisDateObj->setInternalValue(result);
        return result;
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
            return jsNaN(exec);
        gregorianDateTime.copyFrom(*other);
    }
    
    if (!fillStructuresUsingDateArgs(exec, args, numArgsToUse, &ms, &gregorianDateTime)) {
        TiValue result = jsNaN(exec);
        thisDateObj->setInternalValue(result);
        return result;
    } 
           
    TiValue result = jsNumber(exec, gregorianDateTimeToMS(exec, gregorianDateTime, ms, inputIsUTC));
    thisDateObj->setInternalValue(result);
    return result;
}

TiValue JSC_HOST_CALL dateProtoFuncSetMilliSeconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, thisValue, args, 1, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCMilliseconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, thisValue, args, 1, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetSeconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, thisValue, args, 2, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCSeconds(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, thisValue, args, 2, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetMinutes(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, thisValue, args, 3, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCMinutes(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, thisValue, args, 3, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetHours(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, thisValue, args, 4, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCHours(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, thisValue, args, 4, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetDate(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, thisValue, args, 1, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCDate(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, thisValue, args, 1, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetMonth(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, thisValue, args, 2, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCMonth(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, thisValue, args, 2, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetFullYear(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, thisValue, args, 3, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetUTCFullYear(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, thisValue, args, 3, inputIsUTC);
}

TiValue JSC_HOST_CALL dateProtoFuncSetYear(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue);     
    if (args.isEmpty()) { 
        TiValue result = jsNaN(exec);
        thisDateObj->setInternalValue(result);
        return result;
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
    
    bool ok = true;
    int32_t year = args.at(0).toInt32(exec, ok);
    if (!ok) {
        TiValue result = jsNaN(exec);
        thisDateObj->setInternalValue(result);
        return result;
    }
            
    gregorianDateTime.year = (year > 99 || year < 0) ? year - 1900 : year;
    TiValue result = jsNumber(exec, gregorianDateTimeToMS(exec, gregorianDateTime, ms, false));
    thisDateObj->setInternalValue(result);
    return result;
}

TiValue JSC_HOST_CALL dateProtoFuncGetYear(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (!thisValue.inherits(&DateInstance::info))
        return throwError(exec, TypeError);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNaN(exec);

    // NOTE: IE returns the full year even in getYear.
    return jsNumber(exec, gregorianDateTime->year);
}

TiValue JSC_HOST_CALL dateProtoFuncToJSON(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    TiObject* object = thisValue.toThisObject(exec);
    if (exec->hadException())
        return jsNull();
    
    TiValue toISOValue = object->get(exec, exec->globalData().propertyNames->toISOString);
    if (exec->hadException())
        return jsNull();

    CallData callData;
    CallType callType = toISOValue.getCallData(callData);
    if (callType == CallTypeNone)
        return throwError(exec, TypeError, "toISOString is not a function");

    TiValue result = call(exec, asObject(toISOValue), callType, callData, object, exec->emptyList());
    if (exec->hadException())
        return jsNull();
    if (result.isObject())
        return throwError(exec, TypeError, "toISOString did not return a primitive value");
    return result;
}

} // namespace TI
