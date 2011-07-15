/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "TiCallbackObject.h"

#include <runtime/TiGlobalObject.h>
#include <runtime/JSONObject.h>
#include <runtime/TiString.h>
#include <runtime/LiteralParser.h>
#include <runtime/Operations.h>
#include <runtime/Protect.h>
#include <runtime/UString.h>
#include <runtime/TiValue.h>

#include <wtf/Assertions.h>
#include <wtf/text/StringHash.h>

#include <algorithm> // for std::min

using namespace TI;

::TiType TiValueGetType(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isUndefined();
}

bool TiValueIsNull(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isNull();
}

bool TiValueIsBoolean(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isBoolean();
}

bool TiValueIsNumber(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isNumber();
}

bool TiValueIsString(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isString();
}

bool TiValueIsObject(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.isObject();
}

bool TiValueIsObjectOfClass(TiContextRef ctx, TiValueRef value, TiClassRef jsClass)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    
    if (TiObject* o = jsValue.getObject()) {
        if (o->inherits(&TiCallbackObject<TiGlobalObject>::info))
            return static_cast<TiCallbackObject<TiGlobalObject>*>(o)->inherits(jsClass);
        else if (o->inherits(&TiCallbackObject<TiObject>::info))
            return static_cast<TiCallbackObject<TiObject>*>(o)->inherits(jsClass);
    }
    return false;
}

bool TiValueIsEqual(TiContextRef ctx, TiValueRef a, TiValueRef b, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsA = toJS(exec, a);
    TiValue jsB = toJS(exec, b);

    return TiValue::strictEqual(exec, jsA, jsB);
}

bool TiValueIsInstanceOfConstructor(TiContextRef ctx, TiValueRef value, TiObjectRef constructor, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);

    TiObject* jsConstructor = toJS(constructor);
    if (!jsConstructor->structure()->typeInfo().implementsHasInstance())
        return false;
    bool result = jsConstructor->hasInstance(exec, jsValue, jsConstructor->get(exec, exec->propertyNames().prototype)); // false if an exception is thrown
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

TiValueRef TiValueMakeUndefined(TiContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsUndefined());
}

TiValueRef TiValueMakeNull(TiContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsNull());
}

TiValueRef TiValueMakeBoolean(TiContextRef ctx, bool value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsBoolean(value));
}

TiValueRef TiValueMakeNumber(TiContextRef ctx, double value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    // Our TiValue representation relies on a standard bit pattern for NaN. NaNs
    // generated internally to TiCore naturally have that representation,
    // but an external NaN might not.
    if (isnan(value))
        value = NaN;

    return toRef(exec, jsNumber(exec, value));
}

TiValueRef TiValueMakeString(TiContextRef ctx, TiStringRef string)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsString(exec, string->ustring()));
}

TiValueRef TiValueMakeFromJSONString(TiContextRef ctx, TiStringRef string)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    LiteralParser parser(exec, string->ustring(), LiteralParser::StrictJSON);
    return toRef(exec, parser.tryLiteralParse());
}

TiStringRef TiValueCreateJSONString(TiContextRef ctx, TiValueRef apiValue, unsigned indent, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    TiValue value = toJS(exec, apiValue);
    UString result = JSONStringify(exec, value, indent);
    if (exception)
        *exception = 0;
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    return OpaqueTiString::create(result).releaseRef();
}

bool TiValueToBoolean(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    return jsValue.toBoolean(exec);
}

double TiValueToNumber(TiContextRef ctx, TiValueRef value, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);

    double number = jsValue.toNumber(exec);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        number = NaN;
    }
    return number;
}

TiStringRef TiValueToStringCopy(TiContextRef ctx, TiValueRef value, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJS(exec, value);
    
    RefPtr<OpaqueTiString> stringRef(OpaqueTiString::create(jsValue.toString(exec)));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        stringRef.clear();
    }
    return stringRef.release().releaseRef();
}

TiObjectRef TiValueToObject(TiContextRef ctx, TiValueRef value, TiValueRef* exception)
{
    TiExcState* exec = toJS(ctx);
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
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJSForGC(exec, value);
    gcProtect(jsValue);
}

void TiValueUnprotect(TiContextRef ctx, TiValueRef value)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiValue jsValue = toJSForGC(exec, value);
    gcUnprotect(jsValue);
}
