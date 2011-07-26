/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "ObjectPrototype.h"

#include "Error.h"
#include "TiFunction.h"
#include "TiString.h"
#include "TiStringBuilder.h"
#include "PrototypeFunction.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(ObjectPrototype);

static TiValue JSC_HOST_CALL objectProtoFuncValueOf(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncDefineGetter(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncDefineSetter(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncLookupGetter(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncLookupSetter(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL objectProtoFuncToLocaleString(TiExcState*, TiObject*, TiValue, const ArgList&);

ObjectPrototype::ObjectPrototype(TiExcState* exec, NonNullPassRefPtr<Structure> stucture, Structure* prototypeFunctionStructure)
    : TiObject(stucture)
    , m_hasNoPropertiesWithUInt32Names(true)
{
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().toString, objectProtoFuncToString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().toLocaleString, objectProtoFuncToLocaleString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().valueOf, objectProtoFuncValueOf), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().hasOwnProperty, objectProtoFuncHasOwnProperty), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().propertyIsEnumerable, objectProtoFuncPropertyIsEnumerable), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().isPrototypeOf, objectProtoFuncIsPrototypeOf), DontEnum);

    // Mozilla extensions
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 2, exec->propertyNames().__defineGetter__, objectProtoFuncDefineGetter), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 2, exec->propertyNames().__defineSetter__, objectProtoFuncDefineSetter), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().__lookupGetter__, objectProtoFuncLookupGetter), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 1, exec->propertyNames().__lookupSetter__, objectProtoFuncLookupSetter), DontEnum);
}

void ObjectPrototype::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    TiObject::put(exec, propertyName, value, slot);

    if (m_hasNoPropertiesWithUInt32Names) {
        bool isUInt32;
        propertyName.toStrictUInt32(&isUInt32);
        m_hasNoPropertiesWithUInt32Names = !isUInt32;
    }
}

bool ObjectPrototype::getOwnPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (m_hasNoPropertiesWithUInt32Names)
        return false;
    return TiObject::getOwnPropertySlot(exec, propertyName, slot);
}

// ------------------------------ Functions --------------------------------

// ECMA 15.2.4.2, 15.2.4.4, 15.2.4.5, 15.2.4.7

TiValue JSC_HOST_CALL objectProtoFuncValueOf(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    return thisValue.toThisObject(exec);
}

TiValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    return jsBoolean(thisValue.toThisObject(exec)->hasOwnProperty(exec, Identifier(exec, args.at(0).toString(exec))));
}

TiValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    TiObject* thisObj = thisValue.toThisObject(exec);

    if (!args.at(0).isObject())
        return jsBoolean(false);

    TiValue v = asObject(args.at(0))->prototype();

    while (true) {
        if (!v.isObject())
            return jsBoolean(false);
        if (v == thisObj)
            return jsBoolean(true);
        v = asObject(v)->prototype();
    }
}

TiValue JSC_HOST_CALL objectProtoFuncDefineGetter(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    CallData callData;
    if (args.at(1).getCallData(callData) == CallTypeNone)
        return throwError(exec, SyntaxError, "invalid getter usage");
    thisValue.toThisObject(exec)->defineGetter(exec, Identifier(exec, args.at(0).toString(exec)), asObject(args.at(1)));
    return jsUndefined();
}

TiValue JSC_HOST_CALL objectProtoFuncDefineSetter(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    CallData callData;
    if (args.at(1).getCallData(callData) == CallTypeNone)
        return throwError(exec, SyntaxError, "invalid setter usage");
    thisValue.toThisObject(exec)->defineSetter(exec, Identifier(exec, args.at(0).toString(exec)), asObject(args.at(1)));
    return jsUndefined();
}

TiValue JSC_HOST_CALL objectProtoFuncLookupGetter(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    return thisValue.toThisObject(exec)->lookupGetter(exec, Identifier(exec, args.at(0).toString(exec)));
}

TiValue JSC_HOST_CALL objectProtoFuncLookupSetter(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    return thisValue.toThisObject(exec)->lookupSetter(exec, Identifier(exec, args.at(0).toString(exec)));
}

TiValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList& args)
{
    return jsBoolean(thisValue.toThisObject(exec)->propertyIsEnumerable(exec, Identifier(exec, args.at(0).toString(exec))));
}

TiValue JSC_HOST_CALL objectProtoFuncToLocaleString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    return thisValue.toThisTiString(exec);
}

TiValue JSC_HOST_CALL objectProtoFuncToString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    return jsMakeNontrivialString(exec, "[object ", thisValue.toThisObject(exec)->className(), "]");
}

} // namespace TI
