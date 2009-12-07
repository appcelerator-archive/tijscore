/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "BooleanPrototype.h"

#include "Error.h"
#include "TiFunction.h"
#include "TiString.h"
#include "ObjectPrototype.h"
#include "PrototypeFunction.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(BooleanPrototype);

// Functions
static TiValue JSC_HOST_CALL booleanProtoFuncToString(TiExcState*, TiObject*, TiValue, const ArgList&);
static TiValue JSC_HOST_CALL booleanProtoFuncValueOf(TiExcState*, TiObject*, TiValue, const ArgList&);

// ECMA 15.6.4

BooleanPrototype::BooleanPrototype(TiExcState* exec, NonNullPassRefPtr<Structure> structure, Structure* prototypeFunctionStructure)
    : BooleanObject(structure)
{
    setInternalValue(jsBoolean(false));

    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().toString, booleanProtoFuncToString), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().valueOf, booleanProtoFuncValueOf), DontEnum);
}


// ------------------------------ Functions --------------------------

// ECMA 15.6.4.2 + 15.6.4.3

TiValue JSC_HOST_CALL booleanProtoFuncToString(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (thisValue == jsBoolean(false))
        return jsNontrivialString(exec, "false");

    if (thisValue == jsBoolean(true))
        return jsNontrivialString(exec, "true");

    if (!thisValue.inherits(&BooleanObject::info))
        return throwError(exec, TypeError);

    if (asBooleanObject(thisValue)->internalValue() == jsBoolean(false))
        return jsNontrivialString(exec, "false");

    ASSERT(asBooleanObject(thisValue)->internalValue() == jsBoolean(true));
    return jsNontrivialString(exec, "true");
}

TiValue JSC_HOST_CALL booleanProtoFuncValueOf(TiExcState* exec, TiObject*, TiValue thisValue, const ArgList&)
{
    if (thisValue.isBoolean())
        return thisValue;

    if (!thisValue.inherits(&BooleanObject::info))
        return throwError(exec, TypeError);

    return asBooleanObject(thisValue)->internalValue();
}

} // namespace TI
