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
#include "ErrorConstructor.h"

#include "ErrorPrototype.h"
#include "TiGlobalObject.h"
#include "TiString.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(ErrorConstructor);

ErrorConstructor::ErrorConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, ErrorPrototype* errorPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, errorPrototype->classInfo()->className))
{
    // ECMA 15.11.3.1 Error.prototype
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, errorPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), DontDelete | ReadOnly | DontEnum);
}

// ECMA 15.9.3

static EncodedTiValue JSC_HOST_CALL constructWithErrorConstructor(TiExcState* exec)
{
    TiValue message = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    Structure* errorStructure = asInternalFunction(exec->callee())->globalObject()->errorStructure();
    return TiValue::encode(ErrorInstance::create(exec, errorStructure, message));
}

ConstructType ErrorConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithErrorConstructor;
    return ConstructTypeHost;
}

static EncodedTiValue JSC_HOST_CALL callErrorConstructor(TiExcState* exec)
{
    TiValue message = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    Structure* errorStructure = asInternalFunction(exec->callee())->globalObject()->errorStructure();
    return TiValue::encode(ErrorInstance::create(exec, errorStructure, message));
}

CallType ErrorConstructor::getCallData(CallData& callData)
{
    callData.native.function = callErrorConstructor;
    return CallTypeHost;
}

} // namespace TI
