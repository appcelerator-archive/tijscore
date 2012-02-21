/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
#include "ErrorPrototype.h"

#include "TiFunction.h"
#include "TiString.h"
#include "TiStringBuilder.h"
#include "ObjectPrototype.h"
#include "StringRecursionChecker.h"
#include "UString.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(ErrorPrototype);

static EncodedTiValue JSC_HOST_CALL errorProtoFuncToString(TiExcState*);

}

#include "ErrorPrototype.lut.h"

namespace TI {

const ClassInfo ErrorPrototype::s_info = { "Error", &ErrorInstance::s_info, 0, TiExcState::errorPrototypeTable };

/* Source for ErrorPrototype.lut.h
@begin errorPrototypeTable
  toString          errorProtoFuncToString         DontEnum|Function 0
@end
*/

ASSERT_CLASS_FITS_IN_CELL(ErrorPrototype);

ErrorPrototype::ErrorPrototype(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure)
    : ErrorInstance(&exec->globalData(), structure)
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().name, jsNontrivialString(exec, "Error"), DontEnum);

    ASSERT(inherits(&s_info));
    putAnonymousValue(globalObject->globalData(), 0, globalObject);
}

bool ErrorPrototype::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<ErrorInstance>(exec, TiExcState::errorPrototypeTable(exec), this, propertyName, slot);
}

bool ErrorPrototype::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<ErrorInstance>(exec, TiExcState::errorPrototypeTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

EncodedTiValue JSC_HOST_CALL errorProtoFuncToString(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);

    StringRecursionChecker checker(exec, thisObj);
    if (EncodedTiValue earlyReturnValue = checker.earlyReturnValue())
        return earlyReturnValue;

    TiValue name = thisObj->get(exec, exec->propertyNames().name);
    TiValue message = thisObj->get(exec, exec->propertyNames().message);

    // Mozilla-compatible format.

    if (!name.isUndefined()) {
        if (!message.isUndefined())
            return TiValue::encode(jsMakeNontrivialString(exec, name.toString(exec), ": ", message.toString(exec)));
        return TiValue::encode(jsNontrivialString(exec, name.toString(exec)));
    }
    if (!message.isUndefined())
        return TiValue::encode(jsMakeNontrivialString(exec, "Error: ", message.toString(exec)));
    return TiValue::encode(jsNontrivialString(exec, "Error"));
}

} // namespace TI
