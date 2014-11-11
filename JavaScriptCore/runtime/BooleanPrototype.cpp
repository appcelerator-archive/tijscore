/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
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
#include "JSFunction.h"
#include "JSString.h"
#include "ObjectPrototype.h"
#include "Operations.h"

namespace TI {

static EncodedTiValue JSC_HOST_CALL booleanProtoFuncToString(ExecState*);
static EncodedTiValue JSC_HOST_CALL booleanProtoFuncValueOf(ExecState*);

}

#include "BooleanPrototype.lut.h"

namespace TI {

const ClassInfo BooleanPrototype::s_info = { "Boolean", &BooleanObject::s_info, 0, ExecState::booleanPrototypeTable, CREATE_METHOD_TABLE(BooleanPrototype) };

/* Source for BooleanPrototype.lut.h
@begin booleanPrototypeTable
  toString  booleanProtoFuncToString    DontEnum|Function 0
  valueOf   booleanProtoFuncValueOf     DontEnum|Function 0
@end
*/

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(BooleanPrototype);

BooleanPrototype::BooleanPrototype(VM& vm, Structure* structure)
    : BooleanObject(vm, structure)
{
}

void BooleanPrototype::finishCreation(VM& vm, JSGlobalObject*)
{
    Base::finishCreation(vm);
    setInternalValue(vm, jsBoolean(false));

    ASSERT(inherits(info()));
}

bool BooleanPrototype::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<BooleanObject>(exec, ExecState::booleanPrototypeTable(exec), jsCast<BooleanPrototype*>(object), propertyName, slot);
}

// ------------------------------ Functions ---------------------------

EncodedTiValue JSC_HOST_CALL booleanProtoFuncToString(ExecState* exec)
{
    VM* vm = &exec->vm();
    TiValue thisValue = exec->hostThisValue();
    if (thisValue == jsBoolean(false))
        return TiValue::encode(vm->smallStrings.falseString());

    if (thisValue == jsBoolean(true))
        return TiValue::encode(vm->smallStrings.trueString());

    if (!thisValue.inherits(BooleanObject::info()))
        return throwVMTypeError(exec);

    if (asBooleanObject(thisValue)->internalValue() == jsBoolean(false))
        return TiValue::encode(vm->smallStrings.falseString());

    ASSERT(asBooleanObject(thisValue)->internalValue() == jsBoolean(true));
    return TiValue::encode(vm->smallStrings.trueString());
}

EncodedTiValue JSC_HOST_CALL booleanProtoFuncValueOf(ExecState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isBoolean())
        return TiValue::encode(thisValue);

    if (!thisValue.inherits(BooleanObject::info()))
        return throwVMTypeError(exec);

    return TiValue::encode(asBooleanObject(thisValue)->internalValue());
}

} // namespace TI
