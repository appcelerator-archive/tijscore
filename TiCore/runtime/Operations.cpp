/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Operations.h"

#include "Error.h"
#include "TiObject.h"
#include "TiString.h"
#include <math.h>
#include <stdio.h>
#include <wtf/MathExtras.h>

namespace TI {

bool TiValue::equalSlowCase(TiExcState* exec, TiValue v1, TiValue v2)
{
    return equalSlowCaseInline(exec, v1, v2);
}

bool TiValue::strictEqualSlowCase(TiExcState* exec, TiValue v1, TiValue v2)
{
    return strictEqualSlowCaseInline(exec, v1, v2);
}

NEVER_INLINE TiValue jsAddSlowCase(CallFrame* callFrame, TiValue v1, TiValue v2)
{
    // exception for the Date exception in defaultValue()
    TiValue p1 = v1.toPrimitive(callFrame);
    TiValue p2 = v2.toPrimitive(callFrame);

    if (p1.isString()) {
        return p2.isString()
            ? jsString(callFrame, asString(p1), asString(p2))
            : jsString(callFrame, asString(p1), p2.toString(callFrame));
    }
    if (p2.isString())
        return jsString(callFrame, p1.toString(callFrame), asString(p2));

    return jsNumber(callFrame, p1.toNumber(callFrame) + p2.toNumber(callFrame));
}

TiValue jsTypeStringForValue(CallFrame* callFrame, TiValue v)
{
    if (v.isUndefined())
        return jsNontrivialString(callFrame, "undefined");
    if (v.isBoolean())
        return jsNontrivialString(callFrame, "boolean");
    if (v.isNumber())
        return jsNontrivialString(callFrame, "number");
    if (v.isString())
        return jsNontrivialString(callFrame, "string");
    if (v.isObject()) {
        // Return "undefined" for objects that should be treated
        // as null when doing comparisons.
        if (asObject(v)->structure()->typeInfo().masqueradesAsUndefined())
            return jsNontrivialString(callFrame, "undefined");
        CallData callData;
        if (asObject(v)->getCallData(callData) != CallTypeNone)
            return jsNontrivialString(callFrame, "function");
    }
    return jsNontrivialString(callFrame, "object");
}

bool jsIsObjectType(TiValue v)
{
    if (!v.isCell())
        return v.isNull();

    TiType type = asCell(v)->structure()->typeInfo().type();
    if (type == NumberType || type == StringType)
        return false;
    if (type == ObjectType) {
        if (asObject(v)->structure()->typeInfo().masqueradesAsUndefined())
            return false;
        CallData callData;
        if (asObject(v)->getCallData(callData) != CallTypeNone)
            return false;
    }
    return true;
}

bool jsIsFunctionType(TiValue v)
{
    if (v.isObject()) {
        CallData callData;
        if (asObject(v)->getCallData(callData) != CallTypeNone)
            return true;
    }
    return false;
}

} // namespace TI
