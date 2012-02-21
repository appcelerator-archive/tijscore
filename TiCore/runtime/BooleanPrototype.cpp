/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008, 2011 Apple Inc. All rights reserved.
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
#include "BooleanPrototype.h"

#include "Error.h"
#include "ExceptionHelpers.h"
#include "TiFunction.h"
#include "TiString.h"
#include "ObjectPrototype.h"

namespace TI {

static EncodedTiValue JSC_HOST_CALL booleanProtoFuncToString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL booleanProtoFuncValueOf(TiExcState*);

}

#include "BooleanPrototype.lut.h"

namespace TI {

const ClassInfo BooleanPrototype::s_info = { "Boolean", &BooleanObject::s_info, 0, TiExcState::booleanPrototypeTable };

/* Source for BooleanPrototype.lut.h
@begin booleanPrototypeTable
  toString  booleanProtoFuncToString    DontEnum|Function 0
  valueOf   booleanProtoFuncValueOf     DontEnum|Function 0
@end
*/

ASSERT_CLASS_FITS_IN_CELL(BooleanPrototype);

BooleanPrototype::BooleanPrototype(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure)
    : BooleanObject(exec->globalData(), structure)
{
    setInternalValue(exec->globalData(), jsBoolean(false));

    ASSERT(inherits(&s_info));
    putAnonymousValue(globalObject->globalData(), 0, globalObject);
}

bool BooleanPrototype::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<BooleanObject>(exec, TiExcState::booleanPrototypeTable(exec), this, propertyName, slot);
}

bool BooleanPrototype::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<BooleanObject>(exec, TiExcState::booleanPrototypeTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

EncodedTiValue JSC_HOST_CALL booleanProtoFuncToString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue == jsBoolean(false))
        return TiValue::encode(jsNontrivialString(exec, "false"));

    if (thisValue == jsBoolean(true))
        return TiValue::encode(jsNontrivialString(exec, "true"));

    if (!thisValue.inherits(&BooleanObject::s_info))
        return throwVMTypeError(exec);

    if (asBooleanObject(thisValue)->internalValue() == jsBoolean(false))
        return TiValue::encode(jsNontrivialString(exec, "false"));

    ASSERT(asBooleanObject(thisValue)->internalValue() == jsBoolean(true));
    return TiValue::encode(jsNontrivialString(exec, "true"));
}

EncodedTiValue JSC_HOST_CALL booleanProtoFuncValueOf(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isBoolean())
        return TiValue::encode(thisValue);

    if (!thisValue.inherits(&BooleanObject::s_info))
        return throwVMTypeError(exec);

    return TiValue::encode(asBooleanObject(thisValue)->internalValue());
}

} // namespace TI
