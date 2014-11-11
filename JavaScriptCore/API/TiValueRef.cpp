/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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

#include "config.h"
#include "TiValueRef.h"

#include "APICast.h"
#include "APIShims.h"
#include "JSAPIWrapperObject.h"
#include "JSCTiValue.h"
#include "JSCallbackObject.h"
#include "JSGlobalObject.h"
#include "JSONObject.h"
#include "JSString.h"
#include "LiteralParser.h"
#include "Operations.h"
#include "Protect.h"
#include <runtime/DateInstance.h>

#include <wtf/Assertions.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#include <algorithm> // for std::min

#if PLATFORM(MAC)
#include <mach-o/dyld.h>
#endif

using namespace TI;

#if PLATFORM(MAC)
static bool evernoteHackNeeded()
{
    static const int32_t webkitLastVersionWithEvernoteHack = 35133959;
    static bool hackNeeded = CFEqual(CFBundleGetIdentifier(CFBundleGetMainBundle()), CFSTR("com.evernote.Evernote"))
        && NSVersionOfLinkTimeLibrary("JavaScriptCore") <= webkitLastVersionWithEvernoteHack;

    return hackNeeded;
}
#endif

::JSType TiValueGetType(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return kTITypeUndefined;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);

    if (jsValue.isUndefined())
        return kTITypeUndefined;
    if (jsValue.isNull())
        return kTITypeNull;
    if (jsValue.isBoolean())
        return kTITypeBoolean;
    if (jsValue.isNumber())
        return kTITypeNumber;
    if (jsValue.isString())
        return kTITypeString;
    ASSERT(jsValue.isObject());
    return kTITypeObject;
}

bool TiValueIsUndefined(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isUndefined();
}

bool TiValueIsNull(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isNull();
}

bool TiValueIsBoolean(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isBoolean();
}

bool TiValueIsNumber(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isNumber();
}

bool TiValueIsString(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isString();
}

bool TiValueIsArray(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.inherits(JSArray::info());
}

bool TiValueIsDate(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    TiValue jsValue = toJS(exec, value);
    return jsValue.inherits(DateInstance::info());
}

bool TiValueIsObject(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isObject();
}

bool TiValueIsObjectOfClass(TiContextRef ctx, TiValueRef value, TiClassRef jsClass)
{
    if (!ctx || !jsClass) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    
    if (JSObject* o = jsValue.getObject()) {
        if (o->inherits(JSCallbackObject<JSGlobalObject>::info()))
            return jsCast<JSCallbackObject<JSGlobalObject>*>(o)->inherits(jsClass);
        if (o->inherits(JSCallbackObject<JSDestructibleObject>::info()))
            return jsCast<JSCallbackObject<JSDestructibleObject>*>(o)->inherits(jsClass);
#if JSC_OBJC_API_ENABLED
        if (o->inherits(JSCallbackObject<JSAPIWrapperObject>::info()))
            return jsCast<JSCallbackObject<JSAPIWrapperObject>*>(o)->inherits(jsClass);
#endif
    }
    return false;
}

bool TiValueIsEqual(TiContextRef ctx, TiValueRef a, TiValueRef b, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsA = toJS(exec, a);
    TiValue jsB = toJS(exec, b);

    bool result = TiValue::equal(exec, jsA, jsB); // false if an exception is thrown
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

bool TiValueIsStrictEqual(TiContextRef ctx, TiValueRef a, TiValueRef b)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsA = toJS(exec, a);
    TiValue jsB = toJS(exec, b);

    return TiValue::strictEqual(exec, jsA, jsB);
}

bool TiValueIsInstanceOfConstructor(TiContextRef ctx, TiValueRef value, TiObjectRef constructor, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);

    JSObject* jsConstructor = toJS(constructor);
    if (!jsConstructor->structure()->typeInfo().implementsHasInstance())
        return false;
    bool result = jsConstructor->hasInstance(exec, jsValue); // false if an exception is thrown
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

TiValueRef TiValueMakeUndefined(TiContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsUndefined());
}

TiValueRef TiValueMakeNull(TiContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsNull());
}

TiValueRef TiValueMakeBoolean(TiContextRef ctx, bool value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsBoolean(value));
}

TiValueRef TiValueMakeNumber(TiContextRef ctx, double value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    // Our TiValue representation relies on a standard bit pattern for NaN. NaNs
    // generated internally to JavaScriptCore naturally have that representation,
    // but an external NaN might not.
    if (std::isnan(value))
        value = QNaN;

    return toRef(exec, jsNumber(value));
}

TiValueRef TiValueMakeString(TiContextRef ctx, TiStringRef string)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsString(exec, string->string()));
}

TiValueRef TiValueMakeFromJSONString(TiContextRef ctx, TiStringRef string)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    String str = string->string();
    unsigned length = str.length();
    if (length && str.is8Bit()) {
        LiteralParser<LChar> parser(exec, str.characters8(), length, StrictJSON);
        return toRef(exec, parser.tryLiteralParse());
    }
    LiteralParser<UChar> parser(exec, str.characters(), length, StrictJSON);
    return toRef(exec, parser.tryLiteralParse());
}

TiStringRef TiValueCreateJSONString(TiContextRef ctx, TiValueRef apiValue, unsigned indent, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    TiValue value = toJS(exec, apiValue);
    String result = JSONStringify(exec, value, indent);
    if (exception)
        *exception = 0;
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    return OpaqueTiString::create(result).leakRef();
}

bool TiValueToBoolean(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return false;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.toBoolean(exec);
}

double TiValueToNumber(TiContextRef ctx, TiValueRef value, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return QNaN;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);

    double number = jsValue.toNumber(exec);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        number = QNaN;
    }
    return number;
}

TiStringRef TiValueToStringCopy(TiContextRef ctx, TiValueRef value, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    
    RefPtr<OpaqueTiString> stringRef(OpaqueTiString::create(jsValue.toString(exec)->value(exec)));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        stringRef.clear();
    }
    return stringRef.release().leakRef();
}

TiObjectRef TiValueToObject(TiContextRef ctx, TiValueRef value, TiValueRef* exception)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    
    TiObjectRef objectRef = toRef(jsValue.toObject(exec));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        objectRef = 0;
    }
    return objectRef;
}    

void TiValueProtect(TiContextRef ctx, TiValueRef value)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJSForGC(exec, value);
    gcProtect(jsValue);
}

void TiValueUnprotect(TiContextRef ctx, TiValueRef value)
{
#if PLATFORM(MAC)
    if ((!value || !ctx) && evernoteHackNeeded())
        return;
#endif

    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJSForGC(exec, value);
    gcUnprotect(jsValue);
}
