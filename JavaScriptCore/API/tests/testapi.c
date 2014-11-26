/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TiCore.h"
#include "TiBasePrivate.h"
#include "TiContextRefPrivate.h"
#include "TiObjectRefPrivate.h"
#include "JSScriptRefPrivate.h"
#include "TiStringRefPrivate.h"
#include <math.h>
#define ASSERT_DISABLED 0
#include <wtf/Assertions.h>

#if PLATFORM(MAC) || PLATFORM(IOS)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#endif

#if JSC_OBJC_API_ENABLED
void testObjectiveCAPI(void);
#endif

extern void JSSynchronousGarbageCollectForDebugging(TiContextRef);

static TiGlobalContextRef context;
int failed;
static void assertEqualsAsBoolean(TiValueRef value, bool expectedValue)
{
    if (TiValueToBoolean(context, value) != expectedValue) {
        fprintf(stderr, "assertEqualsAsBoolean failed: %p, %d\n", value, expectedValue);
        failed = 1;
    }
}

static void assertEqualsAsNumber(TiValueRef value, double expectedValue)
{
    double number = TiValueToNumber(context, value, NULL);

    // FIXME <rdar://4668451> - On i386 the isnan(double) macro tries to map to the isnan(float) function,
    // causing a build break with -Wshorten-64-to-32 enabled.  The issue is known by the appropriate team.
    // After that's resolved, we can remove these casts
    if (number != expectedValue && !(isnan((float)number) && isnan((float)expectedValue))) {
        fprintf(stderr, "assertEqualsAsNumber failed: %p, %lf\n", value, expectedValue);
        failed = 1;
    }
}

static void assertEqualsAsUTF8String(TiValueRef value, const char* expectedValue)
{
    TiStringRef valueAsString = TiValueToStringCopy(context, value, NULL);

    size_t jsSize = TiStringGetMaximumUTF8CStringSize(valueAsString);
    char* jsBuffer = (char*)malloc(jsSize);
    TiStringGetUTF8CString(valueAsString, jsBuffer, jsSize);
    
    unsigned i;
    for (i = 0; jsBuffer[i]; i++) {
        if (jsBuffer[i] != expectedValue[i]) {
            fprintf(stderr, "assertEqualsAsUTF8String failed at character %d: %c(%d) != %c(%d)\n", i, jsBuffer[i], jsBuffer[i], expectedValue[i], expectedValue[i]);
            failed = 1;
        }
    }

    if (jsSize < strlen(jsBuffer) + 1) {
        fprintf(stderr, "assertEqualsAsUTF8String failed: jsSize was too small\n");
        failed = 1;
    }

    free(jsBuffer);
    TiStringRelease(valueAsString);
}

static void assertEqualsAsCharactersPtr(TiValueRef value, const char* expectedValue)
{
    TiStringRef valueAsString = TiValueToStringCopy(context, value, NULL);

    size_t jsLength = TiStringGetLength(valueAsString);
    const JSChar* jsBuffer = TiStringGetCharactersPtr(valueAsString);

    CFStringRef expectedValueAsCFString = CFStringCreateWithCString(kCFAllocatorDefault, 
                                                                    expectedValue,
                                                                    kCFStringEncodingUTF8);    
    CFIndex cfLength = CFStringGetLength(expectedValueAsCFString);
    UniChar* cfBuffer = (UniChar*)malloc(cfLength * sizeof(UniChar));
    CFStringGetCharacters(expectedValueAsCFString, CFRangeMake(0, cfLength), cfBuffer);
    CFRelease(expectedValueAsCFString);

    if (memcmp(jsBuffer, cfBuffer, cfLength * sizeof(UniChar)) != 0) {
        fprintf(stderr, "assertEqualsAsCharactersPtr failed: jsBuffer != cfBuffer\n");
        failed = 1;
    }
    
    if (jsLength != (size_t)cfLength) {
        fprintf(stderr, "assertEqualsAsCharactersPtr failed: jsLength(%ld) != cfLength(%ld)\n", jsLength, cfLength);
        failed = 1;
    }

    free(cfBuffer);
    TiStringRelease(valueAsString);
}

static bool timeZoneIsPST()
{
    char timeZoneName[70];
    struct tm gtm;
    memset(&gtm, 0, sizeof(gtm));
    strftime(timeZoneName, sizeof(timeZoneName), "%Z", &gtm);

    return 0 == strcmp("PST", timeZoneName);
}

static TiValueRef jsGlobalValue; // non-stack value for testing TiValueProtect()

/* MyObject pseudo-class */

static bool MyObject_hasProperty(TiContextRef context, TiObjectRef object, TiStringRef propertyName)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);

    if (TiStringIsEqualToUTF8CString(propertyName, "alwaysOne")
        || TiStringIsEqualToUTF8CString(propertyName, "cantFind")
        || TiStringIsEqualToUTF8CString(propertyName, "throwOnGet")
        || TiStringIsEqualToUTF8CString(propertyName, "myPropertyName")
        || TiStringIsEqualToUTF8CString(propertyName, "hasPropertyLie")
        || TiStringIsEqualToUTF8CString(propertyName, "0")) {
        return true;
    }
    
    return false;
}

static TiValueRef MyObject_getProperty(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    
    if (TiStringIsEqualToUTF8CString(propertyName, "alwaysOne")) {
        return TiValueMakeNumber(context, 1);
    }
    
    if (TiStringIsEqualToUTF8CString(propertyName, "myPropertyName")) {
        return TiValueMakeNumber(context, 1);
    }

    if (TiStringIsEqualToUTF8CString(propertyName, "cantFind")) {
        return TiValueMakeUndefined(context);
    }
    
    if (TiStringIsEqualToUTF8CString(propertyName, "hasPropertyLie")) {
        return 0;
    }

    if (TiStringIsEqualToUTF8CString(propertyName, "throwOnGet")) {
        return TiEvalScript(context, TiStringCreateWithUTF8CString("throw 'an exception'"), object, TiStringCreateWithUTF8CString("test script"), 1, exception);
    }

    if (TiStringIsEqualToUTF8CString(propertyName, "0")) {
        *exception = TiValueMakeNumber(context, 1);
        return TiValueMakeNumber(context, 1);
    }
    
    return TiValueMakeNull(context);
}

static bool MyObject_setProperty(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(value);
    UNUSED_PARAM(exception);

    if (TiStringIsEqualToUTF8CString(propertyName, "cantSet"))
        return true; // pretend we set the property in order to swallow it
    
    if (TiStringIsEqualToUTF8CString(propertyName, "throwOnSet")) {
        TiEvalScript(context, TiStringCreateWithUTF8CString("throw 'an exception'"), object, TiStringCreateWithUTF8CString("test script"), 1, exception);
    }
    
    return false;
}

static bool MyObject_deleteProperty(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    
    if (TiStringIsEqualToUTF8CString(propertyName, "cantDelete"))
        return true;
    
    if (TiStringIsEqualToUTF8CString(propertyName, "throwOnDelete")) {
        TiEvalScript(context, TiStringCreateWithUTF8CString("throw 'an exception'"), object, TiStringCreateWithUTF8CString("test script"), 1, exception);
        return false;
    }

    return false;
}

static void MyObject_getPropertyNames(TiContextRef context, TiObjectRef object, TiPropertyNameAccumulatorRef propertyNames)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    
    TiStringRef propertyName;
    
    propertyName = TiStringCreateWithUTF8CString("alwaysOne");
    TiPropertyNameAccumulatorAddName(propertyNames, propertyName);
    TiStringRelease(propertyName);
    
    propertyName = TiStringCreateWithUTF8CString("myPropertyName");
    TiPropertyNameAccumulatorAddName(propertyNames, propertyName);
    TiStringRelease(propertyName);
}

static TiValueRef MyObject_callAsFunction(TiContextRef context, TiObjectRef object, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(exception);

    if (argumentCount > 0 && TiValueIsString(context, arguments[0]) && TiStringIsEqualToUTF8CString(TiValueToStringCopy(context, arguments[0], 0), "throwOnCall")) {
        TiEvalScript(context, TiStringCreateWithUTF8CString("throw 'an exception'"), object, TiStringCreateWithUTF8CString("test script"), 1, exception);
        return TiValueMakeUndefined(context);
    }

    if (argumentCount > 0 && TiValueIsStrictEqual(context, arguments[0], TiValueMakeNumber(context, 0)))
        return TiValueMakeNumber(context, 1);
    
    return TiValueMakeUndefined(context);
}

static TiObjectRef MyObject_callAsConstructor(TiContextRef context, TiObjectRef object, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);

    if (argumentCount > 0 && TiValueIsString(context, arguments[0]) && TiStringIsEqualToUTF8CString(TiValueToStringCopy(context, arguments[0], 0), "throwOnConstruct")) {
        TiEvalScript(context, TiStringCreateWithUTF8CString("throw 'an exception'"), object, TiStringCreateWithUTF8CString("test script"), 1, exception);
        return object;
    }

    if (argumentCount > 0 && TiValueIsStrictEqual(context, arguments[0], TiValueMakeNumber(context, 0)))
        return TiValueToObject(context, TiValueMakeNumber(context, 1), exception);
    
    return TiValueToObject(context, TiValueMakeNumber(context, 0), exception);
}

static bool MyObject_hasInstance(TiContextRef context, TiObjectRef constructor, TiValueRef possibleValue, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(constructor);

    if (TiValueIsString(context, possibleValue) && TiStringIsEqualToUTF8CString(TiValueToStringCopy(context, possibleValue, 0), "throwOnHasInstance")) {
        TiEvalScript(context, TiStringCreateWithUTF8CString("throw 'an exception'"), constructor, TiStringCreateWithUTF8CString("test script"), 1, exception);
        return false;
    }

    TiStringRef numberString = TiStringCreateWithUTF8CString("Number");
    TiObjectRef numberConstructor = TiValueToObject(context, TiObjectGetProperty(context, TiContextGetGlobalObject(context), numberString, exception), exception);
    TiStringRelease(numberString);

    return TiValueIsInstanceOfConstructor(context, possibleValue, numberConstructor, exception);
}

static TiValueRef MyObject_convertToType(TiContextRef context, TiObjectRef object, TiType type, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(exception);
    
    switch (type) {
    case kTITypeNumber:
        return TiValueMakeNumber(context, 1);
    case kTITypeString:
        {
            TiStringRef string = TiStringCreateWithUTF8CString("MyObjectAsString");
            TiValueRef result = TiValueMakeString(context, string);
            TiStringRelease(string);
            return result;
        }
    default:
        break;
    }

    // string conversion -- forward to default object class
    return TiValueMakeNull(context);
}

static TiValueRef MyObject_convertToTypeWrapper(TiContextRef context, TiObjectRef object, TiType type, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(type);
    UNUSED_PARAM(exception);
    // Forward to default object class
    return 0;
}

static bool MyObject_set_nullGetForwardSet(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiValueRef* exception)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);
    UNUSED_PARAM(exception);
    return false; // Forward to parent class.
}

static TiStaticValue evilStaticValues[] = {
    { "nullGetSet", 0, 0, kTiPropertyAttributeNone },
    { "nullGetForwardSet", 0, MyObject_set_nullGetForwardSet, kTiPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static TiStaticFunction evilStaticFunctions[] = {
    { "nullCall", 0, kTiPropertyAttributeNone },
    { 0, 0, 0 }
};

TiClassDefinition MyObject_definition = {
    0,
    kTiClassAttributeNone,
    
    "MyObject",
    NULL,
    
    evilStaticValues,
    evilStaticFunctions,
    
    NULL,
    NULL,
    MyObject_hasProperty,
    MyObject_getProperty,
    MyObject_setProperty,
    MyObject_deleteProperty,
    MyObject_getPropertyNames,
    MyObject_callAsFunction,
    MyObject_callAsConstructor,
    MyObject_hasInstance,
    MyObject_convertToType,
};

TiClassDefinition MyObject_convertToTypeWrapperDefinition = {
    0,
    kTiClassAttributeNone,
    
    "MyObject",
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    MyObject_convertToTypeWrapper,
};

TiClassDefinition MyObject_nullWrapperDefinition = {
    0,
    kTiClassAttributeNone,
    
    "MyObject",
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static TiClassRef MyObject_class(TiContextRef context)
{
    UNUSED_PARAM(context);

    static TiClassRef jsClass;
    if (!jsClass) {
        TiClassRef baseClass = TiClassCreate(&MyObject_definition);
        MyObject_convertToTypeWrapperDefinition.parentClass = baseClass;
        TiClassRef wrapperClass = TiClassCreate(&MyObject_convertToTypeWrapperDefinition);
        MyObject_nullWrapperDefinition.parentClass = wrapperClass;
        jsClass = TiClassCreate(&MyObject_nullWrapperDefinition);
    }

    return jsClass;
}

static TiValueRef PropertyCatchalls_getProperty(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    if (TiStringIsEqualToUTF8CString(propertyName, "x")) {
        static size_t count;
        if (count++ < 5)
            return NULL;

        // Swallow all .x gets after 5, returning null.
        return TiValueMakeNull(context);
    }

    if (TiStringIsEqualToUTF8CString(propertyName, "y")) {
        static size_t count;
        if (count++ < 5)
            return NULL;

        // Swallow all .y gets after 5, returning null.
        return TiValueMakeNull(context);
    }
    
    if (TiStringIsEqualToUTF8CString(propertyName, "z")) {
        static size_t count;
        if (count++ < 5)
            return NULL;

        // Swallow all .y gets after 5, returning null.
        return TiValueMakeNull(context);
    }

    return NULL;
}

static bool PropertyCatchalls_setProperty(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);
    UNUSED_PARAM(exception);

    if (TiStringIsEqualToUTF8CString(propertyName, "x")) {
        static size_t count;
        if (count++ < 5)
            return false;

        // Swallow all .x sets after 4.
        return true;
    }

    if (TiStringIsEqualToUTF8CString(propertyName, "make_throw") || TiStringIsEqualToUTF8CString(propertyName, "0")) {
        *exception = TiValueMakeNumber(context, 5);
        return true;
    }

    return false;
}

static void PropertyCatchalls_getPropertyNames(TiContextRef context, TiObjectRef object, TiPropertyNameAccumulatorRef propertyNames)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(object);

    static size_t count;
    static const char* numbers[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    
    // Provide a property of a different name every time.
    TiStringRef propertyName = TiStringCreateWithUTF8CString(numbers[count++ % 10]);
    TiPropertyNameAccumulatorAddName(propertyNames, propertyName);
    TiStringRelease(propertyName);
}

TiClassDefinition PropertyCatchalls_definition = {
    0,
    kTiClassAttributeNone,
    
    "PropertyCatchalls",
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    PropertyCatchalls_getProperty,
    PropertyCatchalls_setProperty,
    NULL,
    PropertyCatchalls_getPropertyNames,
    NULL,
    NULL,
    NULL,
    NULL,
};

static TiClassRef PropertyCatchalls_class(TiContextRef context)
{
    UNUSED_PARAM(context);

    static TiClassRef jsClass;
    if (!jsClass)
        jsClass = TiClassCreate(&PropertyCatchalls_definition);
    
    return jsClass;
}

static bool EvilExceptionObject_hasInstance(TiContextRef context, TiObjectRef constructor, TiValueRef possibleValue, TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(constructor);
    
    TiStringRef hasInstanceName = TiStringCreateWithUTF8CString("hasInstance");
    TiValueRef hasInstance = TiObjectGetProperty(context, constructor, hasInstanceName, exception);
    TiStringRelease(hasInstanceName);
    if (!hasInstance)
        return false;
    TiObjectRef function = TiValueToObject(context, hasInstance, exception);
    TiValueRef result = TiObjectCallAsFunction(context, function, constructor, 1, &possibleValue, exception);
    return result && TiValueToBoolean(context, result);
}

static TiValueRef EvilExceptionObject_convertToType(TiContextRef context, TiObjectRef object, TiType type, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(exception);
    TiStringRef funcName;
    switch (type) {
    case kTITypeNumber:
        funcName = TiStringCreateWithUTF8CString("toNumber");
        break;
    case kTITypeString:
        funcName = TiStringCreateWithUTF8CString("toStringExplicit");
        break;
    default:
        return TiValueMakeNull(context);
        break;
    }
    
    TiValueRef func = TiObjectGetProperty(context, object, funcName, exception);
    TiStringRelease(funcName);    
    TiObjectRef function = TiValueToObject(context, func, exception);
    if (!function)
        return TiValueMakeNull(context);
    TiValueRef value = TiObjectCallAsFunction(context, function, object, 0, NULL, exception);
    if (!value) {
        TiStringRef errorString = TiStringCreateWithUTF8CString("convertToType failed"); 
        TiValueRef errorStringRef = TiValueMakeString(context, errorString);
        TiStringRelease(errorString);
        return errorStringRef;
    }
    return value;
}

TiClassDefinition EvilExceptionObject_definition = {
    0,
    kTiClassAttributeNone,

    "EvilExceptionObject",
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    EvilExceptionObject_hasInstance,
    EvilExceptionObject_convertToType,
};

static TiClassRef EvilExceptionObject_class(TiContextRef context)
{
    UNUSED_PARAM(context);
    
    static TiClassRef jsClass;
    if (!jsClass)
        jsClass = TiClassCreate(&EvilExceptionObject_definition);
    
    return jsClass;
}

TiClassDefinition EmptyObject_definition = {
    0,
    kTiClassAttributeNone,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static TiClassRef EmptyObject_class(TiContextRef context)
{
    UNUSED_PARAM(context);
    
    static TiClassRef jsClass;
    if (!jsClass)
        jsClass = TiClassCreate(&EmptyObject_definition);
    
    return jsClass;
}


static TiValueRef Base_get(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    return TiValueMakeNumber(ctx, 1); // distinguish base get form derived get
}

static bool Base_set(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);

    *exception = TiValueMakeNumber(ctx, 1); // distinguish base set from derived set
    return true;
}

static TiValueRef Base_callAsFunction(TiContextRef ctx, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return TiValueMakeNumber(ctx, 1); // distinguish base call from derived call
}

static TiValueRef Base_returnHardNull(TiContextRef ctx, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return 0; // should convert to undefined!
}

static TiStaticFunction Base_staticFunctions[] = {
    { "baseProtoDup", NULL, kTiPropertyAttributeNone },
    { "baseProto", Base_callAsFunction, kTiPropertyAttributeNone },
    { "baseHardNull", Base_returnHardNull, kTiPropertyAttributeNone },
    { 0, 0, 0 }
};

static TiStaticValue Base_staticValues[] = {
    { "baseDup", Base_get, Base_set, kTiPropertyAttributeNone },
    { "baseOnly", Base_get, Base_set, kTiPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static bool TestInitializeFinalize;
static void Base_initialize(TiContextRef context, TiObjectRef object)
{
    UNUSED_PARAM(context);

    if (TestInitializeFinalize) {
        ASSERT((void*)1 == TiObjectGetPrivate(object));
        TiObjectSetPrivate(object, (void*)2);
    }
}

static unsigned Base_didFinalize;
static void Base_finalize(TiObjectRef object)
{
    UNUSED_PARAM(object);
    if (TestInitializeFinalize) {
        ASSERT((void*)4 == TiObjectGetPrivate(object));
        Base_didFinalize = true;
    }
}

static TiClassRef Base_class(TiContextRef context)
{
    UNUSED_PARAM(context);

    static TiClassRef jsClass;
    if (!jsClass) {
        TiClassDefinition definition = kTiClassDefinitionEmpty;
        definition.staticValues = Base_staticValues;
        definition.staticFunctions = Base_staticFunctions;
        definition.initialize = Base_initialize;
        definition.finalize = Base_finalize;
        jsClass = TiClassCreate(&definition);
    }
    return jsClass;
}

static TiValueRef Derived_get(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    return TiValueMakeNumber(ctx, 2); // distinguish base get form derived get
}

static bool Derived_set(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiValueRef* exception)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);

    *exception = TiValueMakeNumber(ctx, 2); // distinguish base set from derived set
    return true;
}

static TiValueRef Derived_callAsFunction(TiContextRef ctx, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return TiValueMakeNumber(ctx, 2); // distinguish base call from derived call
}

static TiStaticFunction Derived_staticFunctions[] = {
    { "protoOnly", Derived_callAsFunction, kTiPropertyAttributeNone },
    { "protoDup", NULL, kTiPropertyAttributeNone },
    { "baseProtoDup", Derived_callAsFunction, kTiPropertyAttributeNone },
    { 0, 0, 0 }
};

static TiStaticValue Derived_staticValues[] = {
    { "derivedOnly", Derived_get, Derived_set, kTiPropertyAttributeNone },
    { "protoDup", Derived_get, Derived_set, kTiPropertyAttributeNone },
    { "baseDup", Derived_get, Derived_set, kTiPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static void Derived_initialize(TiContextRef context, TiObjectRef object)
{
    UNUSED_PARAM(context);

    if (TestInitializeFinalize) {
        ASSERT((void*)2 == TiObjectGetPrivate(object));
        TiObjectSetPrivate(object, (void*)3);
    }
}

static void Derived_finalize(TiObjectRef object)
{
    if (TestInitializeFinalize) {
        ASSERT((void*)3 == TiObjectGetPrivate(object));
        TiObjectSetPrivate(object, (void*)4);
    }
}

static TiClassRef Derived_class(TiContextRef context)
{
    static TiClassRef jsClass;
    if (!jsClass) {
        TiClassDefinition definition = kTiClassDefinitionEmpty;
        definition.parentClass = Base_class(context);
        definition.staticValues = Derived_staticValues;
        definition.staticFunctions = Derived_staticFunctions;
        definition.initialize = Derived_initialize;
        definition.finalize = Derived_finalize;
        jsClass = TiClassCreate(&definition);
    }
    return jsClass;
}

static TiClassRef Derived2_class(TiContextRef context)
{
    static TiClassRef jsClass;
    if (!jsClass) {
        TiClassDefinition definition = kTiClassDefinitionEmpty;
        definition.parentClass = Derived_class(context);
        jsClass = TiClassCreate(&definition);
    }
    return jsClass;
}

static TiValueRef print_callAsFunction(TiContextRef ctx, TiObjectRef functionObject, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(functionObject);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(exception);

    ASSERT(TiContextGetGlobalContext(ctx) == context);
    
    if (argumentCount > 0) {
        TiStringRef string = TiValueToStringCopy(ctx, arguments[0], NULL);
        size_t sizeUTF8 = TiStringGetMaximumUTF8CStringSize(string);
        char* stringUTF8 = (char*)malloc(sizeUTF8);
        TiStringGetUTF8CString(string, stringUTF8, sizeUTF8);
        printf("%s\n", stringUTF8);
        free(stringUTF8);
        TiStringRelease(string);
    }
    
    return TiValueMakeUndefined(ctx);
}

static TiObjectRef myConstructor_callAsConstructor(TiContextRef context, TiObjectRef constructorObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(constructorObject);
    UNUSED_PARAM(exception);
    
    TiObjectRef result = TiObjectMake(context, NULL, NULL);
    if (argumentCount > 0) {
        TiStringRef value = TiStringCreateWithUTF8CString("value");
        TiObjectSetProperty(context, result, value, arguments[0], kTiPropertyAttributeNone, NULL);
        TiStringRelease(value);
    }
    
    return result;
}

static TiObjectRef myBadConstructor_callAsConstructor(TiContextRef context, TiObjectRef constructorObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(constructorObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    
    return 0;
}


static void globalObject_initialize(TiContextRef context, TiObjectRef object)
{
    UNUSED_PARAM(object);
    // Ensure that an execution context is passed in
    ASSERT(context);

    // Ensure that the global object is set to the object that we were passed
    TiObjectRef globalObject = TiContextGetGlobalObject(context);
    ASSERT(globalObject);
    ASSERT(object == globalObject);

    // Ensure that the standard global properties have been set on the global object
    TiStringRef array = TiStringCreateWithUTF8CString("Array");
    TiObjectRef arrayConstructor = TiValueToObject(context, TiObjectGetProperty(context, globalObject, array, NULL), NULL);
    TiStringRelease(array);

    UNUSED_PARAM(arrayConstructor);
    ASSERT(arrayConstructor);
}

static TiValueRef globalObject_get(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    return TiValueMakeNumber(ctx, 3);
}

static bool globalObject_set(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(value);

    *exception = TiValueMakeNumber(ctx, 3);
    return true;
}

static TiValueRef globalObject_call(TiContextRef ctx, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);

    return TiValueMakeNumber(ctx, 3);
}

static TiValueRef functionGC(TiContextRef context, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);
    TiGarbageCollect(context);
    return TiValueMakeUndefined(context);
}

static TiStaticValue globalObject_staticValues[] = {
    { "globalStaticValue", globalObject_get, globalObject_set, kTiPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static TiStaticFunction globalObject_staticFunctions[] = {
    { "globalStaticFunction", globalObject_call, kTiPropertyAttributeNone },
    { "gc", functionGC, kTiPropertyAttributeNone },
    { 0, 0, 0 }
};

static char* createStringWithContentsOfFile(const char* fileName);

static void testInitializeFinalize()
{
    TiObjectRef o = TiObjectMake(context, Derived_class(context), (void*)1);
    UNUSED_PARAM(o);
    ASSERT(TiObjectGetPrivate(o) == (void*)3);
}

static TiValueRef jsNumberValue =  NULL;

static TiObjectRef aHeapRef = NULL;

static void makeGlobalNumberValue(TiContextRef context) {
    TiValueRef v = TiValueMakeNumber(context, 420);
    TiValueProtect(context, v);
    jsNumberValue = v;
    v = NULL;
}

static bool assertTrue(bool value, const char* message)
{
    if (!value) {
        if (message)
            fprintf(stderr, "assertTrue failed: '%s'\n", message);
        else
            fprintf(stderr, "assertTrue failed.\n");
        failed = 1;
    }
    return value;
}

static bool checkForCycleInPrototypeChain()
{
    bool result = true;
    TiGlobalContextRef context = TiGlobalContextCreate(0);
    TiObjectRef object1 = TiObjectMake(context, /* jsClass */ 0, /* data */ 0);
    TiObjectRef object2 = TiObjectMake(context, /* jsClass */ 0, /* data */ 0);
    TiObjectRef object3 = TiObjectMake(context, /* jsClass */ 0, /* data */ 0);

    TiObjectSetPrototype(context, object1, TiValueMakeNull(context));
    ASSERT(TiValueIsNull(context, TiObjectGetPrototype(context, object1)));

    // object1 -> object1
    TiObjectSetPrototype(context, object1, object1);
    result &= assertTrue(TiValueIsNull(context, TiObjectGetPrototype(context, object1)), "It is possible to assign self as a prototype");

    // object1 -> object2 -> object1
    TiObjectSetPrototype(context, object2, object1);
    ASSERT(TiValueIsStrictEqual(context, TiObjectGetPrototype(context, object2), object1));
    TiObjectSetPrototype(context, object1, object2);
    result &= assertTrue(TiValueIsNull(context, TiObjectGetPrototype(context, object1)), "It is possible to close a prototype chain cycle");

    // object1 -> object2 -> object3 -> object1
    TiObjectSetPrototype(context, object2, object3);
    ASSERT(TiValueIsStrictEqual(context, TiObjectGetPrototype(context, object2), object3));
    TiObjectSetPrototype(context, object1, object2);
    ASSERT(TiValueIsStrictEqual(context, TiObjectGetPrototype(context, object1), object2));
    TiObjectSetPrototype(context, object3, object1);
    result &= assertTrue(!TiValueIsStrictEqual(context, TiObjectGetPrototype(context, object3), object1), "It is possible to close a prototype chain cycle");

    TiValueRef exception;
    TiStringRef code = TiStringCreateWithUTF8CString("o = { }; p = { }; o.__proto__ = p; p.__proto__ = o");
    TiStringRef file = TiStringCreateWithUTF8CString("");
    result &= assertTrue(!TiEvalScript(context, code, /* thisObject*/ 0, file, 1, &exception)
                         , "An exception should be thrown");

    TiStringRelease(code);
    TiStringRelease(file);
    TiGlobalContextRelease(context);
    return result;
}

static TiValueRef valueToObjectExceptionCallAsFunction(TiContextRef ctx, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    TiValueRef jsUndefined = TiValueMakeUndefined(TiContextGetGlobalContext(ctx));
    TiValueToObject(TiContextGetGlobalContext(ctx), jsUndefined, exception);
    
    return TiValueMakeUndefined(ctx);
}
static bool valueToObjectExceptionTest()
{
    TiGlobalContextRef testContext;
    TiClassDefinition globalObjectClassDefinition = kTiClassDefinitionEmpty;
    globalObjectClassDefinition.initialize = globalObject_initialize;
    globalObjectClassDefinition.staticValues = globalObject_staticValues;
    globalObjectClassDefinition.staticFunctions = globalObject_staticFunctions;
    globalObjectClassDefinition.attributes = kTiClassAttributeNoAutomaticPrototype;
    TiClassRef globalObjectClass = TiClassCreate(&globalObjectClassDefinition);
    testContext = TiGlobalContextCreateInGroup(NULL, globalObjectClass);
    TiObjectRef globalObject = TiContextGetGlobalObject(testContext);

    TiStringRef valueToObject = TiStringCreateWithUTF8CString("valueToObject");
    TiObjectRef valueToObjectFunction = TiObjectMakeFunctionWithCallback(testContext, valueToObject, valueToObjectExceptionCallAsFunction);
    TiObjectSetProperty(testContext, globalObject, valueToObject, valueToObjectFunction, kTiPropertyAttributeNone, NULL);
    TiStringRelease(valueToObject);

    TiStringRef test = TiStringCreateWithUTF8CString("valueToObject();");
    TiEvalScript(testContext, test, NULL, NULL, 1, NULL);
    
    TiStringRelease(test);
    TiClassRelease(globalObjectClass);
    TiGlobalContextRelease(testContext);
    
    return true;
}

static bool globalContextNameTest()
{
    bool result = true;
    TiGlobalContextRef context = TiGlobalContextCreate(0);

    TiStringRef str = TiGlobalContextCopyName(context);
    result &= assertTrue(!str, "Default context name is NULL");

    TiStringRef name1 = TiStringCreateWithUTF8CString("name1");
    TiStringRef name2 = TiStringCreateWithUTF8CString("name2");

    TiGlobalContextSetName(context, name1);
    TiStringRef fetchName1 = TiGlobalContextCopyName(context);
    TiGlobalContextSetName(context, name2);
    TiStringRef fetchName2 = TiGlobalContextCopyName(context);

    result &= assertTrue(TiStringIsEqual(name1, fetchName1), "Unexpected Context name");
    result &= assertTrue(TiStringIsEqual(name2, fetchName2), "Unexpected Context name");
    result &= assertTrue(!TiStringIsEqual(fetchName1, fetchName2), "Unexpected Context name");

    TiStringRelease(name1);
    TiStringRelease(name2);
    TiStringRelease(fetchName1);
    TiStringRelease(fetchName2);

    return result;
}

static void checkConstnessInJSObjectNames()
{
    TiStaticFunction fun;
    fun.name = "something";
    TiStaticValue val;
    val.name = "something";
}

#if PLATFORM(MAC) || PLATFORM(IOS)
static double currentCPUTime()
{
    mach_msg_type_number_t infoCount = THREAD_BASIC_INFO_COUNT;
    thread_basic_info_data_t info;

    /* Get thread information */
    mach_port_t threadPort = mach_thread_self();
    thread_info(threadPort, THREAD_BASIC_INFO, (thread_info_t)(&info), &infoCount);
    mach_port_deallocate(mach_task_self(), threadPort);
    
    double time = info.user_time.seconds + info.user_time.microseconds / 1000000.;
    time += info.system_time.seconds + info.system_time.microseconds / 1000000.;
    
    return time;
}

static TiValueRef currentCPUTime_callAsFunction(TiContextRef ctx, TiObjectRef functionObject, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(functionObject);
    UNUSED_PARAM(thisObject);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);

    ASSERT(TiContextGetGlobalContext(ctx) == context);
    return TiValueMakeNumber(ctx, currentCPUTime());
}

bool shouldTerminateCallbackWasCalled = false;
static bool shouldTerminateCallback(TiContextRef ctx, void* context)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(context);
    shouldTerminateCallbackWasCalled = true;
    return true;
}

bool cancelTerminateCallbackWasCalled = false;
static bool cancelTerminateCallback(TiContextRef ctx, void* context)
{
    UNUSED_PARAM(ctx);
    UNUSED_PARAM(context);
    cancelTerminateCallbackWasCalled = true;
    return false;
}

int extendTerminateCallbackCalled = 0;
static bool extendTerminateCallback(TiContextRef ctx, void* context)
{
    UNUSED_PARAM(context);
    extendTerminateCallbackCalled++;
    if (extendTerminateCallbackCalled == 1) {
        TiContextGroupRef contextGroup = TiContextGetGroup(ctx);
        TiContextGroupSetExecutionTimeLimit(contextGroup, .200f, extendTerminateCallback, 0);
        return false;
    }
    return true;
}
#endif /* PLATFORM(MAC) || PLATFORM(IOS) */


int main(int argc, char* argv[])
{
#if OS(WINDOWS)
    // Cygwin calls ::SetErrorMode(SEM_FAILCRITICALERRORS), which we will inherit. This is bad for
    // testing/debugging, as it causes the post-mortem debugger not to be invoked. We reset the
    // error mode here to work around Cygwin's behavior. See <http://webkit.org/b/55222>.
    ::SetErrorMode(0);
#endif

#if JSC_OBJC_API_ENABLED
    testObjectiveCAPI();
#endif

    const char *scriptPath = "testapi.js";
    if (argc > 1) {
        scriptPath = argv[1];
    }
    
    // Test garbage collection with a fresh context
    context = TiGlobalContextCreateInGroup(NULL, NULL);
    TestInitializeFinalize = true;
    testInitializeFinalize();
    TiGlobalContextRelease(context);
    TestInitializeFinalize = false;

    ASSERT(Base_didFinalize);

    TiClassDefinition globalObjectClassDefinition = kTiClassDefinitionEmpty;
    globalObjectClassDefinition.initialize = globalObject_initialize;
    globalObjectClassDefinition.staticValues = globalObject_staticValues;
    globalObjectClassDefinition.staticFunctions = globalObject_staticFunctions;
    globalObjectClassDefinition.attributes = kTiClassAttributeNoAutomaticPrototype;
    TiClassRef globalObjectClass = TiClassCreate(&globalObjectClassDefinition);
    context = TiGlobalContextCreateInGroup(NULL, globalObjectClass);

    TiContextGroupRef contextGroup = TiContextGetGroup(context);
    
    TiGlobalContextRetain(context);
    TiGlobalContextRelease(context);
    ASSERT(TiContextGetGlobalContext(context) == context);
    
    JSReportExtraMemoryCost(context, 0);
    JSReportExtraMemoryCost(context, 1);
    JSReportExtraMemoryCost(context, 1024);

    TiObjectRef globalObject = TiContextGetGlobalObject(context);
    ASSERT(TiValueIsObject(context, globalObject));
    
    TiValueRef jsUndefined = TiValueMakeUndefined(context);
    TiValueRef jsNull = TiValueMakeNull(context);
    TiValueRef jsTrue = TiValueMakeBoolean(context, true);
    TiValueRef jsFalse = TiValueMakeBoolean(context, false);
    TiValueRef jsZero = TiValueMakeNumber(context, 0);
    TiValueRef jsOne = TiValueMakeNumber(context, 1);
    TiValueRef jsOneThird = TiValueMakeNumber(context, 1.0 / 3.0);
    TiObjectRef jsObjectNoProto = TiObjectMake(context, NULL, NULL);
    TiObjectSetPrototype(context, jsObjectNoProto, TiValueMakeNull(context));

    // FIXME: test funny utf8 characters
    TiStringRef jsEmptyIString = TiStringCreateWithUTF8CString("");
    TiValueRef jsEmptyString = TiValueMakeString(context, jsEmptyIString);
    
    TiStringRef jsOneIString = TiStringCreateWithUTF8CString("1");
    TiValueRef jsOneString = TiValueMakeString(context, jsOneIString);

    UniChar singleUniChar = 65; // Capital A
    CFMutableStringRef cfString = 
        CFStringCreateMutableWithExternalCharactersNoCopy(kCFAllocatorDefault,
                                                          &singleUniChar,
                                                          1,
                                                          1,
                                                          kCFAllocatorNull);

    TiStringRef jsCFIString = TiStringCreateWithCFString(cfString);
    TiValueRef jsCFString = TiValueMakeString(context, jsCFIString);
    
    CFStringRef cfEmptyString = CFStringCreateWithCString(kCFAllocatorDefault, "", kCFStringEncodingUTF8);
    
    TiStringRef jsCFEmptyIString = TiStringCreateWithCFString(cfEmptyString);
    TiValueRef jsCFEmptyString = TiValueMakeString(context, jsCFEmptyIString);

    CFIndex cfStringLength = CFStringGetLength(cfString);
    UniChar* buffer = (UniChar*)malloc(cfStringLength * sizeof(UniChar));
    CFStringGetCharacters(cfString, 
                          CFRangeMake(0, cfStringLength), 
                          buffer);
    TiStringRef jsCFIStringWithCharacters = TiStringCreateWithCharacters((JSChar*)buffer, cfStringLength);
    TiValueRef jsCFStringWithCharacters = TiValueMakeString(context, jsCFIStringWithCharacters);
    
    TiStringRef jsCFEmptyIStringWithCharacters = TiStringCreateWithCharacters((JSChar*)buffer, CFStringGetLength(cfEmptyString));
    free(buffer);
    TiValueRef jsCFEmptyStringWithCharacters = TiValueMakeString(context, jsCFEmptyIStringWithCharacters);

    JSChar constantString[] = { 'H', 'e', 'l', 'l', 'o', };
    TiStringRef constantStringRef = TiStringCreateWithCharactersNoCopy(constantString, sizeof(constantString) / sizeof(constantString[0]));
    ASSERT(TiStringGetCharactersPtr(constantStringRef) == constantString);
    TiStringRelease(constantStringRef);

    ASSERT(TiValueGetType(context, NULL) == kTITypeNull);
    ASSERT(TiValueGetType(context, jsUndefined) == kTITypeUndefined);
    ASSERT(TiValueGetType(context, jsNull) == kTITypeNull);
    ASSERT(TiValueGetType(context, jsTrue) == kTITypeBoolean);
    ASSERT(TiValueGetType(context, jsFalse) == kTITypeBoolean);
    ASSERT(TiValueGetType(context, jsZero) == kTITypeNumber);
    ASSERT(TiValueGetType(context, jsOne) == kTITypeNumber);
    ASSERT(TiValueGetType(context, jsOneThird) == kTITypeNumber);
    ASSERT(TiValueGetType(context, jsEmptyString) == kTITypeString);
    ASSERT(TiValueGetType(context, jsOneString) == kTITypeString);
    ASSERT(TiValueGetType(context, jsCFString) == kTITypeString);
    ASSERT(TiValueGetType(context, jsCFStringWithCharacters) == kTITypeString);
    ASSERT(TiValueGetType(context, jsCFEmptyString) == kTITypeString);
    ASSERT(TiValueGetType(context, jsCFEmptyStringWithCharacters) == kTITypeString);

    ASSERT(!TiValueIsBoolean(context, NULL));
    ASSERT(!TiValueIsObject(context, NULL));
    ASSERT(!TiValueIsString(context, NULL));
    ASSERT(!TiValueIsNumber(context, NULL));
    ASSERT(!TiValueIsUndefined(context, NULL));
    ASSERT(TiValueIsNull(context, NULL));
    ASSERT(!TiObjectCallAsFunction(context, NULL, NULL, 0, NULL, NULL));
    ASSERT(!TiObjectCallAsConstructor(context, NULL, 0, NULL, NULL));
    ASSERT(!TiObjectIsConstructor(context, NULL));
    ASSERT(!TiObjectIsFunction(context, NULL));

    TiStringRef nullString = TiStringCreateWithUTF8CString(0);
    const JSChar* characters = TiStringGetCharactersPtr(nullString);
    if (characters) {
        printf("FAIL: Didn't return null when accessing character pointer of a null String.\n");
        failed = 1;
    } else
        printf("PASS: returned null when accessing character pointer of a null String.\n");

    TiStringRef emptyString = TiStringCreateWithCFString(CFSTR(""));
    characters = TiStringGetCharactersPtr(emptyString);
    if (!characters) {
        printf("FAIL: Returned null when accessing character pointer of an empty String.\n");
        failed = 1;
    } else
        printf("PASS: returned empty when accessing character pointer of an empty String.\n");

    size_t length = TiStringGetLength(nullString);
    if (length) {
        printf("FAIL: Didn't return 0 length for null String.\n");
        failed = 1;
    } else
        printf("PASS: returned 0 length for null String.\n");
    TiStringRelease(nullString);

    length = TiStringGetLength(emptyString);
    if (length) {
        printf("FAIL: Didn't return 0 length for empty String.\n");
        failed = 1;
    } else
        printf("PASS: returned 0 length for empty String.\n");
    TiStringRelease(emptyString);

    TiObjectRef propertyCatchalls = TiObjectMake(context, PropertyCatchalls_class(context), NULL);
    TiStringRef propertyCatchallsString = TiStringCreateWithUTF8CString("PropertyCatchalls");
    TiObjectSetProperty(context, globalObject, propertyCatchallsString, propertyCatchalls, kTiPropertyAttributeNone, NULL);
    TiStringRelease(propertyCatchallsString);

    TiObjectRef myObject = TiObjectMake(context, MyObject_class(context), NULL);
    TiStringRef myObjectIString = TiStringCreateWithUTF8CString("MyObject");
    TiObjectSetProperty(context, globalObject, myObjectIString, myObject, kTiPropertyAttributeNone, NULL);
    TiStringRelease(myObjectIString);
    
    TiObjectRef EvilExceptionObject = TiObjectMake(context, EvilExceptionObject_class(context), NULL);
    TiStringRef EvilExceptionObjectIString = TiStringCreateWithUTF8CString("EvilExceptionObject");
    TiObjectSetProperty(context, globalObject, EvilExceptionObjectIString, EvilExceptionObject, kTiPropertyAttributeNone, NULL);
    TiStringRelease(EvilExceptionObjectIString);
    
    TiObjectRef EmptyObject = TiObjectMake(context, EmptyObject_class(context), NULL);
    TiStringRef EmptyObjectIString = TiStringCreateWithUTF8CString("EmptyObject");
    TiObjectSetProperty(context, globalObject, EmptyObjectIString, EmptyObject, kTiPropertyAttributeNone, NULL);
    TiStringRelease(EmptyObjectIString);
    
    TiStringRef lengthStr = TiStringCreateWithUTF8CString("length");
    TiObjectRef aStackRef = TiObjectMakeArray(context, 0, 0, 0);
    aHeapRef = aStackRef;
    TiObjectSetProperty(context, aHeapRef, lengthStr, TiValueMakeNumber(context, 10), 0, 0);
    TiStringRef privatePropertyName = TiStringCreateWithUTF8CString("privateProperty");
    if (!TiObjectSetPrivateProperty(context, myObject, privatePropertyName, aHeapRef)) {
        printf("FAIL: Could not set private property.\n");
        failed = 1;
    } else
        printf("PASS: Set private property.\n");
    aStackRef = 0;
    if (TiObjectSetPrivateProperty(context, aHeapRef, privatePropertyName, aHeapRef)) {
        printf("FAIL: TiObjectSetPrivateProperty should fail on non-API objects.\n");
        failed = 1;
    } else
        printf("PASS: Did not allow TiObjectSetPrivateProperty on a non-API object.\n");
    if (TiObjectGetPrivateProperty(context, myObject, privatePropertyName) != aHeapRef) {
        printf("FAIL: Could not retrieve private property.\n");
        failed = 1;
    } else
        printf("PASS: Retrieved private property.\n");
    if (TiObjectGetPrivateProperty(context, aHeapRef, privatePropertyName)) {
        printf("FAIL: TiObjectGetPrivateProperty should return NULL when called on a non-API object.\n");
        failed = 1;
    } else
        printf("PASS: TiObjectGetPrivateProperty return NULL.\n");

    if (TiObjectGetProperty(context, myObject, privatePropertyName, 0) == aHeapRef) {
        printf("FAIL: Accessed private property through ordinary property lookup.\n");
        failed = 1;
    } else
        printf("PASS: Cannot access private property through ordinary property lookup.\n");

    TiGarbageCollect(context);

    for (int i = 0; i < 10000; i++)
        TiObjectMake(context, 0, 0);

    aHeapRef = TiValueToObject(context, TiObjectGetPrivateProperty(context, myObject, privatePropertyName), 0);
    if (TiValueToNumber(context, TiObjectGetProperty(context, aHeapRef, lengthStr, 0), 0) != 10) {
        printf("FAIL: Private property has been collected.\n");
        failed = 1;
    } else
        printf("PASS: Private property does not appear to have been collected.\n");
    TiStringRelease(lengthStr);

    if (!TiObjectSetPrivateProperty(context, myObject, privatePropertyName, 0)) {
        printf("FAIL: Could not set private property to NULL.\n");
        failed = 1;
    } else
        printf("PASS: Set private property to NULL.\n");
    if (TiObjectGetPrivateProperty(context, myObject, privatePropertyName)) {
        printf("FAIL: Could not retrieve private property.\n");
        failed = 1;
    } else
        printf("PASS: Retrieved private property.\n");

    TiStringRef nullJSON = TiStringCreateWithUTF8CString(0);
    TiValueRef nullJSONObject = TiValueMakeFromJSONString(context, nullJSON);
    if (nullJSONObject) {
        printf("FAIL: Did not parse null String as JSON correctly\n");
        failed = 1;
    } else
        printf("PASS: Parsed null String as JSON correctly.\n");
    TiStringRelease(nullJSON);

    TiStringRef validJSON = TiStringCreateWithUTF8CString("{\"aProperty\":true}");
    TiValueRef jsonObject = TiValueMakeFromJSONString(context, validJSON);
    TiStringRelease(validJSON);
    if (!TiValueIsObject(context, jsonObject)) {
        printf("FAIL: Did not parse valid JSON correctly\n");
        failed = 1;
    } else
        printf("PASS: Parsed valid JSON string.\n");
    TiStringRef propertyName = TiStringCreateWithUTF8CString("aProperty");
    assertEqualsAsBoolean(TiObjectGetProperty(context, TiValueToObject(context, jsonObject, 0), propertyName, 0), true);
    TiStringRelease(propertyName);
    TiStringRef invalidJSON = TiStringCreateWithUTF8CString("fail!");
    if (TiValueMakeFromJSONString(context, invalidJSON)) {
        printf("FAIL: Should return null for invalid JSON data\n");
        failed = 1;
    } else
        printf("PASS: Correctly returned null for invalid JSON data.\n");
    TiValueRef exception;
    TiStringRef str = TiValueCreateJSONString(context, jsonObject, 0, 0);
    if (!TiStringIsEqualToUTF8CString(str, "{\"aProperty\":true}")) {
        printf("FAIL: Did not correctly serialise with indent of 0.\n");
        failed = 1;
    } else
        printf("PASS: Correctly serialised with indent of 0.\n");
    TiStringRelease(str);

    str = TiValueCreateJSONString(context, jsonObject, 4, 0);
    if (!TiStringIsEqualToUTF8CString(str, "{\n    \"aProperty\": true\n}")) {
        printf("FAIL: Did not correctly serialise with indent of 4.\n");
        failed = 1;
    } else
        printf("PASS: Correctly serialised with indent of 4.\n");
    TiStringRelease(str);
    TiStringRef src = TiStringCreateWithUTF8CString("({get a(){ throw '';}})");
    TiValueRef unstringifiableObj = TiEvalScript(context, src, NULL, NULL, 1, NULL);
    
    str = TiValueCreateJSONString(context, unstringifiableObj, 4, 0);
    if (str) {
        printf("FAIL: Didn't return null when attempting to serialize unserializable value.\n");
        TiStringRelease(str);
        failed = 1;
    } else
        printf("PASS: returned null when attempting to serialize unserializable value.\n");
    
    str = TiValueCreateJSONString(context, unstringifiableObj, 4, &exception);
    if (str) {
        printf("FAIL: Didn't return null when attempting to serialize unserializable value.\n");
        TiStringRelease(str);
        failed = 1;
    } else
        printf("PASS: returned null when attempting to serialize unserializable value.\n");
    if (!exception) {
        printf("FAIL: Did not set exception on serialisation error\n");
        failed = 1;
    } else
        printf("PASS: set exception on serialisation error\n");
    // Conversions that throw exceptions
    exception = NULL;
    ASSERT(NULL == TiValueToObject(context, jsNull, &exception));
    ASSERT(exception);
    
    exception = NULL;
    // FIXME <rdar://4668451> - On i386 the isnan(double) macro tries to map to the isnan(float) function,
    // causing a build break with -Wshorten-64-to-32 enabled.  The issue is known by the appropriate team.
    // After that's resolved, we can remove these casts
    ASSERT(isnan((float)TiValueToNumber(context, jsObjectNoProto, &exception)));
    ASSERT(exception);

    exception = NULL;
    ASSERT(!TiValueToStringCopy(context, jsObjectNoProto, &exception));
    ASSERT(exception);
    
    ASSERT(TiValueToBoolean(context, myObject));
    
    exception = NULL;
    ASSERT(!TiValueIsEqual(context, jsObjectNoProto, TiValueMakeNumber(context, 1), &exception));
    ASSERT(exception);
    
    exception = NULL;
    TiObjectGetPropertyAtIndex(context, myObject, 0, &exception);
    ASSERT(1 == TiValueToNumber(context, exception, NULL));

    assertEqualsAsBoolean(jsUndefined, false);
    assertEqualsAsBoolean(jsNull, false);
    assertEqualsAsBoolean(jsTrue, true);
    assertEqualsAsBoolean(jsFalse, false);
    assertEqualsAsBoolean(jsZero, false);
    assertEqualsAsBoolean(jsOne, true);
    assertEqualsAsBoolean(jsOneThird, true);
    assertEqualsAsBoolean(jsEmptyString, false);
    assertEqualsAsBoolean(jsOneString, true);
    assertEqualsAsBoolean(jsCFString, true);
    assertEqualsAsBoolean(jsCFStringWithCharacters, true);
    assertEqualsAsBoolean(jsCFEmptyString, false);
    assertEqualsAsBoolean(jsCFEmptyStringWithCharacters, false);
    
    assertEqualsAsNumber(jsUndefined, nan(""));
    assertEqualsAsNumber(jsNull, 0);
    assertEqualsAsNumber(jsTrue, 1);
    assertEqualsAsNumber(jsFalse, 0);
    assertEqualsAsNumber(jsZero, 0);
    assertEqualsAsNumber(jsOne, 1);
    assertEqualsAsNumber(jsOneThird, 1.0 / 3.0);
    assertEqualsAsNumber(jsEmptyString, 0);
    assertEqualsAsNumber(jsOneString, 1);
    assertEqualsAsNumber(jsCFString, nan(""));
    assertEqualsAsNumber(jsCFStringWithCharacters, nan(""));
    assertEqualsAsNumber(jsCFEmptyString, 0);
    assertEqualsAsNumber(jsCFEmptyStringWithCharacters, 0);
    ASSERT(sizeof(JSChar) == sizeof(UniChar));
    
    assertEqualsAsCharactersPtr(jsUndefined, "undefined");
    assertEqualsAsCharactersPtr(jsNull, "null");
    assertEqualsAsCharactersPtr(jsTrue, "true");
    assertEqualsAsCharactersPtr(jsFalse, "false");
    assertEqualsAsCharactersPtr(jsZero, "0");
    assertEqualsAsCharactersPtr(jsOne, "1");
    assertEqualsAsCharactersPtr(jsOneThird, "0.3333333333333333");
    assertEqualsAsCharactersPtr(jsEmptyString, "");
    assertEqualsAsCharactersPtr(jsOneString, "1");
    assertEqualsAsCharactersPtr(jsCFString, "A");
    assertEqualsAsCharactersPtr(jsCFStringWithCharacters, "A");
    assertEqualsAsCharactersPtr(jsCFEmptyString, "");
    assertEqualsAsCharactersPtr(jsCFEmptyStringWithCharacters, "");
    
    assertEqualsAsUTF8String(jsUndefined, "undefined");
    assertEqualsAsUTF8String(jsNull, "null");
    assertEqualsAsUTF8String(jsTrue, "true");
    assertEqualsAsUTF8String(jsFalse, "false");
    assertEqualsAsUTF8String(jsZero, "0");
    assertEqualsAsUTF8String(jsOne, "1");
    assertEqualsAsUTF8String(jsOneThird, "0.3333333333333333");
    assertEqualsAsUTF8String(jsEmptyString, "");
    assertEqualsAsUTF8String(jsOneString, "1");
    assertEqualsAsUTF8String(jsCFString, "A");
    assertEqualsAsUTF8String(jsCFStringWithCharacters, "A");
    assertEqualsAsUTF8String(jsCFEmptyString, "");
    assertEqualsAsUTF8String(jsCFEmptyStringWithCharacters, "");
    
    checkConstnessInJSObjectNames();
    
    ASSERT(TiValueIsStrictEqual(context, jsTrue, jsTrue));
    ASSERT(!TiValueIsStrictEqual(context, jsOne, jsOneString));

    ASSERT(TiValueIsEqual(context, jsOne, jsOneString, NULL));
    ASSERT(!TiValueIsEqual(context, jsTrue, jsFalse, NULL));
    
    CFStringRef cfJSString = TiStringCopyCFString(kCFAllocatorDefault, jsCFIString);
    CFStringRef cfJSEmptyString = TiStringCopyCFString(kCFAllocatorDefault, jsCFEmptyIString);
    ASSERT(CFEqual(cfJSString, cfString));
    ASSERT(CFEqual(cfJSEmptyString, cfEmptyString));
    CFRelease(cfJSString);
    CFRelease(cfJSEmptyString);

    CFRelease(cfString);
    CFRelease(cfEmptyString);
    
    jsGlobalValue = TiObjectMake(context, NULL, NULL);
    makeGlobalNumberValue(context);
    TiValueProtect(context, jsGlobalValue);
    TiGarbageCollect(context);
    ASSERT(TiValueIsObject(context, jsGlobalValue));
    TiValueUnprotect(context, jsGlobalValue);
    TiValueUnprotect(context, jsNumberValue);

    TiStringRef goodSyntax = TiStringCreateWithUTF8CString("x = 1;");
    const char* badSyntaxConstant = "x := 1;";
    TiStringRef badSyntax = TiStringCreateWithUTF8CString(badSyntaxConstant);
    ASSERT(TiCheckScriptSyntax(context, goodSyntax, NULL, 0, NULL));
    ASSERT(!TiCheckScriptSyntax(context, badSyntax, NULL, 0, NULL));
    ASSERT(!JSScriptCreateFromString(contextGroup, 0, 0, badSyntax, 0, 0));
    ASSERT(!JSScriptCreateReferencingImmortalASCIIText(contextGroup, 0, 0, badSyntaxConstant, strlen(badSyntaxConstant), 0, 0));

    TiValueRef result;
    TiValueRef v;
    TiObjectRef o;
    TiStringRef string;

    result = TiEvalScript(context, goodSyntax, NULL, NULL, 1, NULL);
    ASSERT(result);
    ASSERT(TiValueIsEqual(context, result, jsOne, NULL));

    exception = NULL;
    result = TiEvalScript(context, badSyntax, NULL, NULL, 1, &exception);
    ASSERT(!result);
    ASSERT(TiValueIsObject(context, exception));
    
    TiStringRef array = TiStringCreateWithUTF8CString("Array");
    TiObjectRef arrayConstructor = TiValueToObject(context, TiObjectGetProperty(context, globalObject, array, NULL), NULL);
    TiStringRelease(array);
    result = TiObjectCallAsConstructor(context, arrayConstructor, 0, NULL, NULL);
    ASSERT(result);
    ASSERT(TiValueIsObject(context, result));
    ASSERT(TiValueIsInstanceOfConstructor(context, result, arrayConstructor, NULL));
    ASSERT(!TiValueIsInstanceOfConstructor(context, TiValueMakeNull(context), arrayConstructor, NULL));

    o = TiValueToObject(context, result, NULL);
    exception = NULL;
    ASSERT(TiValueIsUndefined(context, TiObjectGetPropertyAtIndex(context, o, 0, &exception)));
    ASSERT(!exception);
    
    TiObjectSetPropertyAtIndex(context, o, 0, TiValueMakeNumber(context, 1), &exception);
    ASSERT(!exception);
    
    exception = NULL;
    ASSERT(1 == TiValueToNumber(context, TiObjectGetPropertyAtIndex(context, o, 0, &exception), &exception));
    ASSERT(!exception);

    TiStringRef functionBody;
    TiObjectRef function;
    
    exception = NULL;
    functionBody = TiStringCreateWithUTF8CString("rreturn Array;");
    TiStringRef line = TiStringCreateWithUTF8CString("line");
    ASSERT(!TiObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, &exception));
    ASSERT(TiValueIsObject(context, exception));
    v = TiObjectGetProperty(context, TiValueToObject(context, exception, NULL), line, NULL);
    assertEqualsAsNumber(v, 1);
    TiStringRelease(functionBody);
    TiStringRelease(line);

    exception = NULL;
    functionBody = TiStringCreateWithUTF8CString("return Array;");
    function = TiObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, &exception);
    TiStringRelease(functionBody);
    ASSERT(!exception);
    ASSERT(TiObjectIsFunction(context, function));
    v = TiObjectCallAsFunction(context, function, NULL, 0, NULL, NULL);
    ASSERT(v);
    ASSERT(TiValueIsEqual(context, v, arrayConstructor, NULL));
    
    exception = NULL;
    function = TiObjectMakeFunction(context, NULL, 0, NULL, jsEmptyIString, NULL, 0, &exception);
    ASSERT(!exception);
    v = TiObjectCallAsFunction(context, function, NULL, 0, NULL, &exception);
    ASSERT(v && !exception);
    ASSERT(TiValueIsUndefined(context, v));
    
    exception = NULL;
    v = NULL;
    TiStringRef foo = TiStringCreateWithUTF8CString("foo");
    TiStringRef argumentNames[] = { foo };
    functionBody = TiStringCreateWithUTF8CString("return foo;");
    function = TiObjectMakeFunction(context, foo, 1, argumentNames, functionBody, NULL, 1, &exception);
    ASSERT(function && !exception);
    TiValueRef arguments[] = { TiValueMakeNumber(context, 2) };
    TiObjectCallAsFunction(context, function, NULL, 1, arguments, &exception);
    TiStringRelease(foo);
    TiStringRelease(functionBody);
    
    string = TiValueToStringCopy(context, function, NULL);
    assertEqualsAsUTF8String(TiValueMakeString(context, string), "function foo(foo) { return foo;\n}");
    TiStringRelease(string);

    TiStringRef print = TiStringCreateWithUTF8CString("print");
    TiObjectRef printFunction = TiObjectMakeFunctionWithCallback(context, print, print_callAsFunction);
    TiObjectSetProperty(context, globalObject, print, printFunction, kTiPropertyAttributeNone, NULL); 
    TiStringRelease(print);
    
    ASSERT(!TiObjectSetPrivate(printFunction, (void*)1));
    ASSERT(!TiObjectGetPrivate(printFunction));

    TiStringRef myConstructorIString = TiStringCreateWithUTF8CString("MyConstructor");
    TiObjectRef myConstructor = TiObjectMakeConstructor(context, NULL, myConstructor_callAsConstructor);
    TiObjectSetProperty(context, globalObject, myConstructorIString, myConstructor, kTiPropertyAttributeNone, NULL);
    TiStringRelease(myConstructorIString);
    
    TiStringRef myBadConstructorIString = TiStringCreateWithUTF8CString("MyBadConstructor");
    TiObjectRef myBadConstructor = TiObjectMakeConstructor(context, NULL, myBadConstructor_callAsConstructor);
    TiObjectSetProperty(context, globalObject, myBadConstructorIString, myBadConstructor, kTiPropertyAttributeNone, NULL);
    TiStringRelease(myBadConstructorIString);
    
    ASSERT(!TiObjectSetPrivate(myConstructor, (void*)1));
    ASSERT(!TiObjectGetPrivate(myConstructor));
    
    string = TiStringCreateWithUTF8CString("Base");
    TiObjectRef baseConstructor = TiObjectMakeConstructor(context, Base_class(context), NULL);
    TiObjectSetProperty(context, globalObject, string, baseConstructor, kTiPropertyAttributeNone, NULL);
    TiStringRelease(string);
    
    string = TiStringCreateWithUTF8CString("Derived");
    TiObjectRef derivedConstructor = TiObjectMakeConstructor(context, Derived_class(context), NULL);
    TiObjectSetProperty(context, globalObject, string, derivedConstructor, kTiPropertyAttributeNone, NULL);
    TiStringRelease(string);
    
    string = TiStringCreateWithUTF8CString("Derived2");
    TiObjectRef derived2Constructor = TiObjectMakeConstructor(context, Derived2_class(context), NULL);
    TiObjectSetProperty(context, globalObject, string, derived2Constructor, kTiPropertyAttributeNone, NULL);
    TiStringRelease(string);

    o = TiObjectMake(context, NULL, NULL);
    TiObjectSetProperty(context, o, jsOneIString, TiValueMakeNumber(context, 1), kTiPropertyAttributeNone, NULL);
    TiObjectSetProperty(context, o, jsCFIString,  TiValueMakeNumber(context, 1), kTiPropertyAttributeDontEnum, NULL);
    TiPropertyNameArrayRef nameArray = TiObjectCopyPropertyNames(context, o);
    size_t expectedCount = TiPropertyNameArrayGetCount(nameArray);
    size_t count;
    for (count = 0; count < expectedCount; ++count)
        TiPropertyNameArrayGetNameAtIndex(nameArray, count);
    TiPropertyNameArrayRelease(nameArray);
    ASSERT(count == 1); // jsCFString should not be enumerated

    TiValueRef argumentsArrayValues[] = { TiValueMakeNumber(context, 10), TiValueMakeNumber(context, 20) };
    o = TiObjectMakeArray(context, sizeof(argumentsArrayValues) / sizeof(TiValueRef), argumentsArrayValues, NULL);
    string = TiStringCreateWithUTF8CString("length");
    v = TiObjectGetProperty(context, o, string, NULL);
    assertEqualsAsNumber(v, 2);
    v = TiObjectGetPropertyAtIndex(context, o, 0, NULL);
    assertEqualsAsNumber(v, 10);
    v = TiObjectGetPropertyAtIndex(context, o, 1, NULL);
    assertEqualsAsNumber(v, 20);

    o = TiObjectMakeArray(context, 0, NULL, NULL);
    v = TiObjectGetProperty(context, o, string, NULL);
    assertEqualsAsNumber(v, 0);
    TiStringRelease(string);

    TiValueRef argumentsDateValues[] = { TiValueMakeNumber(context, 0) };
    o = TiObjectMakeDate(context, 1, argumentsDateValues, NULL);
    if (timeZoneIsPST())
        assertEqualsAsUTF8String(o, "Wed Dec 31 1969 16:00:00 GMT-0800 (PST)");

    string = TiStringCreateWithUTF8CString("an error message");
    TiValueRef argumentsErrorValues[] = { TiValueMakeString(context, string) };
    o = TiObjectMakeError(context, 1, argumentsErrorValues, NULL);
    assertEqualsAsUTF8String(o, "Error: an error message");
    TiStringRelease(string);

    string = TiStringCreateWithUTF8CString("foo");
    TiStringRef string2 = TiStringCreateWithUTF8CString("gi");
    TiValueRef argumentsRegExpValues[] = { TiValueMakeString(context, string), TiValueMakeString(context, string2) };
    o = TiObjectMakeRegExp(context, 2, argumentsRegExpValues, NULL);
    assertEqualsAsUTF8String(o, "/foo/gi");
    TiStringRelease(string);
    TiStringRelease(string2);

    TiClassDefinition nullDefinition = kTiClassDefinitionEmpty;
    nullDefinition.attributes = kTiClassAttributeNoAutomaticPrototype;
    TiClassRef nullClass = TiClassCreate(&nullDefinition);
    TiClassRelease(nullClass);
    
    nullDefinition = kTiClassDefinitionEmpty;
    nullClass = TiClassCreate(&nullDefinition);
    TiClassRelease(nullClass);

    functionBody = TiStringCreateWithUTF8CString("return this;");
    function = TiObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, NULL);
    TiStringRelease(functionBody);
    v = TiObjectCallAsFunction(context, function, NULL, 0, NULL, NULL);
    ASSERT(TiValueIsEqual(context, v, globalObject, NULL));
    v = TiObjectCallAsFunction(context, function, o, 0, NULL, NULL);
    ASSERT(TiValueIsEqual(context, v, o, NULL));

    functionBody = TiStringCreateWithUTF8CString("return eval(\"this\");");
    function = TiObjectMakeFunction(context, NULL, 0, NULL, functionBody, NULL, 1, NULL);
    TiStringRelease(functionBody);
    v = TiObjectCallAsFunction(context, function, NULL, 0, NULL, NULL);
    ASSERT(TiValueIsEqual(context, v, globalObject, NULL));
    v = TiObjectCallAsFunction(context, function, o, 0, NULL, NULL);
    ASSERT(TiValueIsEqual(context, v, o, NULL));

    const char* thisScript = "this;";
    TiStringRef script = TiStringCreateWithUTF8CString(thisScript);
    v = TiEvalScript(context, script, NULL, NULL, 1, NULL);
    ASSERT(TiValueIsEqual(context, v, globalObject, NULL));
    v = TiEvalScript(context, script, o, NULL, 1, NULL);
    ASSERT(TiValueIsEqual(context, v, o, NULL));
    TiStringRelease(script);

    JSScriptRef scriptObject = JSScriptCreateReferencingImmortalASCIIText(contextGroup, 0, 0, thisScript, strlen(thisScript), 0, 0);
    v = JSScriptEvaluate(context, scriptObject, NULL, NULL);
    ASSERT(TiValueIsEqual(context, v, globalObject, NULL));
    v = JSScriptEvaluate(context, scriptObject, o, NULL);
    ASSERT(TiValueIsEqual(context, v, o, NULL));
    JSScriptRelease(scriptObject);

    script = TiStringCreateWithUTF8CString("eval(this);");
    v = TiEvalScript(context, script, NULL, NULL, 1, NULL);
    ASSERT(TiValueIsEqual(context, v, globalObject, NULL));
    v = TiEvalScript(context, script, o, NULL, 1, NULL);
    ASSERT(TiValueIsEqual(context, v, o, NULL));
    TiStringRelease(script);

    // Verify that creating a constructor for a class with no static functions does not trigger
    // an assert inside putDirect or lead to a crash during GC. <https://bugs.webkit.org/show_bug.cgi?id=25785>
    nullDefinition = kTiClassDefinitionEmpty;
    nullClass = TiClassCreate(&nullDefinition);
    TiObjectMakeConstructor(context, nullClass, 0);
    TiClassRelease(nullClass);

    char* scriptUTF8 = createStringWithContentsOfFile(scriptPath);
    if (!scriptUTF8) {
        printf("FAIL: Test script could not be loaded.\n");
        failed = 1;
    } else {
        TiStringRef url = TiStringCreateWithUTF8CString(scriptPath);
        TiStringRef script = TiStringCreateWithUTF8CString(scriptUTF8);
        TiStringRef errorMessage = 0;
        int errorLine = 0;
        JSScriptRef scriptObject = JSScriptCreateFromString(contextGroup, url, 1, script, &errorMessage, &errorLine);
        ASSERT((!scriptObject) != (!errorMessage));
        if (!scriptObject) {
            printf("FAIL: Test script did not parse\n\t%s:%d\n\t", scriptPath, errorLine);
            CFStringRef errorCF = TiStringCopyCFString(kCFAllocatorDefault, errorMessage);
            CFShow(errorCF);
            CFRelease(errorCF);
            TiStringRelease(errorMessage);
            failed = 1;
        }

        TiStringRelease(script);
        result = scriptObject ? JSScriptEvaluate(context, scriptObject, 0, &exception) : 0;
        if (result && TiValueIsUndefined(context, result))
            printf("PASS: Test script executed successfully.\n");
        else {
            printf("FAIL: Test script returned unexpected value:\n");
            TiStringRef exceptionIString = TiValueToStringCopy(context, exception, NULL);
            CFStringRef exceptionCF = TiStringCopyCFString(kCFAllocatorDefault, exceptionIString);
            CFShow(exceptionCF);
            CFRelease(exceptionCF);
            TiStringRelease(exceptionIString);
            failed = 1;
        }
        JSScriptRelease(scriptObject);
        free(scriptUTF8);
    }

#if PLATFORM(MAC) || PLATFORM(IOS)
    TiStringRef currentCPUTimeStr = TiStringCreateWithUTF8CString("currentCPUTime");
    TiObjectRef currentCPUTimeFunction = TiObjectMakeFunctionWithCallback(context, currentCPUTimeStr, currentCPUTime_callAsFunction);
    TiObjectSetProperty(context, globalObject, currentCPUTimeStr, currentCPUTimeFunction, kTiPropertyAttributeNone, NULL); 
    TiStringRelease(currentCPUTimeStr);

    /* Test script timeout: */
    TiContextGroupSetExecutionTimeLimit(contextGroup, .10f, shouldTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .150) break; } ";
        TiStringRef script = TiStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        shouldTerminateCallbackWasCalled = false;
        startTime = currentCPUTime();
        v = TiEvalScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) < .150f) && shouldTerminateCallbackWasCalled)
            printf("PASS: script timed out as expected.\n");
        else {
            if (!((endTime - startTime) < .150f))
                printf("FAIL: script did not timed out as expected.\n");
            if (!shouldTerminateCallbackWasCalled)
                printf("FAIL: script timeout callback was not called.\n");
            failed = true;
        }

        if (!exception) {
            printf("FAIL: TerminatedExecutionException was not thrown.\n");
            failed = true;
        }
    }

    /* Test the script timeout's TerminatedExecutionException should NOT be catchable: */
    TiContextGroupSetExecutionTimeLimit(contextGroup, 0.10f, shouldTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); try { while (true) { if (currentCPUTime() - startTime > .150) break; } } catch(e) { }";
        TiStringRef script = TiStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        shouldTerminateCallbackWasCalled = false;
        startTime = currentCPUTime();
        v = TiEvalScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) >= .150f) || !shouldTerminateCallbackWasCalled) {
            if (!((endTime - startTime) < .150f))
                printf("FAIL: script did not timed out as expected.\n");
            if (!shouldTerminateCallbackWasCalled)
                printf("FAIL: script timeout callback was not called.\n");
            failed = true;
        }

        if (exception)
            printf("PASS: TerminatedExecutionException was not catchable as expected.\n");
        else {
            printf("FAIL: TerminatedExecutionException was caught.\n");
            failed = true;
        }
    }

    /* Test script timeout with no callback: */
    TiContextGroupSetExecutionTimeLimit(contextGroup, .10f, 0, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .150) break; } ";
        TiStringRef script = TiStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        startTime = currentCPUTime();
        v = TiEvalScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) < .150f) && shouldTerminateCallbackWasCalled)
            printf("PASS: script timed out as expected when no callback is specified.\n");
        else {
            if (!((endTime - startTime) < .150f))
                printf("FAIL: script did not timed out as expected when no callback is specified.\n");
            failed = true;
        }

        if (!exception) {
            printf("FAIL: TerminatedExecutionException was not thrown.\n");
            failed = true;
        }
    }

    /* Test script timeout cancellation: */
    TiContextGroupSetExecutionTimeLimit(contextGroup, 0.10f, cancelTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .150) break; } ";
        TiStringRef script = TiStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        exception = NULL;
        startTime = currentCPUTime();
        v = TiEvalScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();

        if (((endTime - startTime) >= .150f) && cancelTerminateCallbackWasCalled && !exception)
            printf("PASS: script timeout was cancelled as expected.\n");
        else {
            if (((endTime - startTime) < .150) || exception)
                printf("FAIL: script timeout was not cancelled.\n");
            if (!cancelTerminateCallbackWasCalled)
                printf("FAIL: script timeout callback was not called.\n");
            failed = true;
        }

        if (exception) {
            printf("FAIL: Unexpected TerminatedExecutionException thrown.\n");
            failed = true;
        }
    }

    /* Test script timeout extension: */
    TiContextGroupSetExecutionTimeLimit(contextGroup, 0.100f, extendTerminateCallback, 0);
    {
        const char* loopForeverScript = "var startTime = currentCPUTime(); while (true) { if (currentCPUTime() - startTime > .500) break; } ";
        TiStringRef script = TiStringCreateWithUTF8CString(loopForeverScript);
        double startTime;
        double endTime;
        double deltaTime;
        exception = NULL;
        startTime = currentCPUTime();
        v = TiEvalScript(context, script, NULL, NULL, 1, &exception);
        endTime = currentCPUTime();
        deltaTime = endTime - startTime;

        if ((deltaTime >= .300f) && (deltaTime < .500f) && (extendTerminateCallbackCalled == 2) && exception)
            printf("PASS: script timeout was extended as expected.\n");
        else {
            if (deltaTime < .200f)
                printf("FAIL: script timeout was not extended as expected.\n");
            else if (deltaTime >= .500f)
                printf("FAIL: script did not timeout.\n");

            if (extendTerminateCallbackCalled < 1)
                printf("FAIL: script timeout callback was not called.\n");
            if (extendTerminateCallbackCalled < 2)
                printf("FAIL: script timeout callback was not called after timeout extension.\n");

            if (!exception)
                printf("FAIL: TerminatedExecutionException was not thrown during timeout extension test.\n");

            failed = true;
        }
    }
#endif /* PLATFORM(MAC) || PLATFORM(IOS) */

    // Clear out local variables pointing at TiObjectRefs to allow their values to be collected
    function = NULL;
    v = NULL;
    o = NULL;
    globalObject = NULL;
    myConstructor = NULL;

    TiStringRelease(jsEmptyIString);
    TiStringRelease(jsOneIString);
    TiStringRelease(jsCFIString);
    TiStringRelease(jsCFEmptyIString);
    TiStringRelease(jsCFIStringWithCharacters);
    TiStringRelease(jsCFEmptyIStringWithCharacters);
    TiStringRelease(goodSyntax);
    TiStringRelease(badSyntax);

    TiGlobalContextRelease(context);
    TiClassRelease(globalObjectClass);

    // Test for an infinite prototype chain that used to be created. This test
    // passes if the call to TiObjectHasProperty() does not hang.

    TiClassDefinition prototypeLoopClassDefinition = kTiClassDefinitionEmpty;
    prototypeLoopClassDefinition.staticFunctions = globalObject_staticFunctions;
    TiClassRef prototypeLoopClass = TiClassCreate(&prototypeLoopClassDefinition);
    TiGlobalContextRef prototypeLoopContext = TiGlobalContextCreateInGroup(NULL, prototypeLoopClass);

    TiStringRef nameProperty = TiStringCreateWithUTF8CString("name");
    TiObjectHasProperty(prototypeLoopContext, TiContextGetGlobalObject(prototypeLoopContext), nameProperty);

    TiGlobalContextRelease(prototypeLoopContext);
    TiClassRelease(prototypeLoopClass);

    printf("PASS: Infinite prototype chain does not occur.\n");

    if (checkForCycleInPrototypeChain())
        printf("PASS: A cycle in a prototype chain can't be created.\n");
    else {
        printf("FAIL: A cycle in a prototype chain can be created.\n");
        failed = true;
    }
    if (valueToObjectExceptionTest())
        printf("PASS: throwException did not crash when handling an error with appendMessageToError set and no codeBlock available.\n");

    if (globalContextNameTest())
        printf("PASS: global context name behaves as expected.\n");

    if (failed) {
        printf("FAIL: Some tests failed.\n");
        return 1;
    }

    printf("PASS: Program exited normally.\n");
    return 0;
}

static char* createStringWithContentsOfFile(const char* fileName)
{
    char* buffer;
    
    size_t buffer_size = 0;
    size_t buffer_capacity = 1024;
    buffer = (char*)malloc(buffer_capacity);
    
    FILE* f = fopen(fileName, "r");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", fileName);
        free(buffer);
        return 0;
    }
    
    while (!feof(f) && !ferror(f)) {
        buffer_size += fread(buffer + buffer_size, 1, buffer_capacity - buffer_size, f);
        if (buffer_size == buffer_capacity) { // guarantees space for trailing '\0'
            buffer_capacity *= 2;
            buffer = (char*)realloc(buffer, buffer_capacity);
            ASSERT(buffer);
        }
        
        ASSERT(buffer_size < buffer_capacity);
    }
    fclose(f);
    buffer[buffer_size] = '\0';
    
    return buffer;
}
