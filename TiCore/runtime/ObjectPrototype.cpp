/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008, 2011 Apple Inc. All rights reserved.
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

namespace TI {

static EncodedTiValue JSC_HOST_CALL objectProtoFuncValueOf(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineGetter(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineSetter(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupGetter(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupSetter(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(TiExcState*);
static EncodedTiValue JSC_HOST_CALL objectProtoFuncToLocaleString(TiExcState*);

}

#include "ObjectPrototype.lut.h"

namespace TI {

const ClassInfo ObjectPrototype::s_info = { "Object", &JSNonFinalObject::s_info, 0, TiExcState::objectPrototypeTable };

/* Source for ObjectPrototype.lut.h
@begin objectPrototypeTable
  toString              objectProtoFuncToString                 DontEnum|Function 0
  toLocaleString        objectProtoFuncToLocaleString           DontEnum|Function 0
  valueOf               objectProtoFuncValueOf                  DontEnum|Function 0
  hasOwnProperty        objectProtoFuncHasOwnProperty           DontEnum|Function 1
  propertyIsEnumerable  objectProtoFuncPropertyIsEnumerable     DontEnum|Function 1
  isPrototypeOf         objectProtoFuncIsPrototypeOf            DontEnum|Function 1
  __defineGetter__      objectProtoFuncDefineGetter             DontEnum|Function 2
  __defineSetter__      objectProtoFuncDefineSetter             DontEnum|Function 2
  __lookupGetter__      objectProtoFuncLookupGetter             DontEnum|Function 1
  __lookupSetter__      objectProtoFuncLookupSetter             DontEnum|Function 1
@end
*/

ASSERT_CLASS_FITS_IN_CELL(ObjectPrototype);

ObjectPrototype::ObjectPrototype(TiExcState* exec, TiGlobalObject* globalObject, Structure* stucture)
    : JSNonFinalObject(exec->globalData(), stucture)
    , m_hasNoPropertiesWithUInt32Names(true)
{
    ASSERT(inherits(&s_info));
    putAnonymousValue(globalObject->globalData(), 0, globalObject);
}

void ObjectPrototype::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    JSNonFinalObject::put(exec, propertyName, value, slot);

    if (m_hasNoPropertiesWithUInt32Names) {
        bool isUInt32;
        propertyName.toUInt32(isUInt32);
        m_hasNoPropertiesWithUInt32Names = !isUInt32;
    }
}

bool ObjectPrototype::getOwnPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (m_hasNoPropertiesWithUInt32Names)
        return false;
    return JSNonFinalObject::getOwnPropertySlot(exec, propertyName, slot);
}

bool ObjectPrototype::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<JSNonFinalObject>(exec, TiExcState::objectPrototypeTable(exec), this, propertyName, slot);
}

bool ObjectPrototype::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<JSNonFinalObject>(exec, TiExcState::objectPrototypeTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions --------------------------------

EncodedTiValue JSC_HOST_CALL objectProtoFuncValueOf(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(thisValue.toThisObject(exec));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(jsBoolean(thisValue.toThisObject(exec)->hasOwnProperty(exec, Identifier(exec, exec->argument(0).toString(exec)))));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    TiObject* thisObj = thisValue.toThisObject(exec);

    if (!exec->argument(0).isObject())
        return TiValue::encode(jsBoolean(false));

    TiValue v = asObject(exec->argument(0))->prototype();

    while (true) {
        if (!v.isObject())
            return TiValue::encode(jsBoolean(false));
        if (v == thisObj)
            return TiValue::encode(jsBoolean(true));
        v = asObject(v)->prototype();
    }
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineGetter(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    CallData callData;
    if (getCallData(exec->argument(1), callData) == CallTypeNone)
        return throwVMError(exec, createSyntaxError(exec, "invalid getter usage"));
    thisValue.toThisObject(exec)->defineGetter(exec, Identifier(exec, exec->argument(0).toString(exec)), asObject(exec->argument(1)));
    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncDefineSetter(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    CallData callData;
    if (getCallData(exec->argument(1), callData) == CallTypeNone)
        return throwVMError(exec, createSyntaxError(exec, "invalid setter usage"));
    thisValue.toThisObject(exec)->defineSetter(exec, Identifier(exec, exec->argument(0).toString(exec)), asObject(exec->argument(1)));
    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupGetter(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(thisValue.toThisObject(exec)->lookupGetter(exec, Identifier(exec, exec->argument(0).toString(exec))));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncLookupSetter(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(thisValue.toThisObject(exec)->lookupSetter(exec, Identifier(exec, exec->argument(0).toString(exec))));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(jsBoolean(thisValue.toThisObject(exec)->propertyIsEnumerable(exec, Identifier(exec, exec->argument(0).toString(exec)))));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncToLocaleString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(thisValue.toThisTiString(exec));
}

EncodedTiValue JSC_HOST_CALL objectProtoFuncToString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(jsMakeNontrivialString(exec, "[object ", thisValue.toThisObject(exec)->className(), "]"));
}

} // namespace TI
