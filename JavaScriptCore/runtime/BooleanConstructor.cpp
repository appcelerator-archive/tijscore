/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
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
#include "BooleanConstructor.h"

#include "BooleanPrototype.h"
#include "JSGlobalObject.h"
#include "Operations.h"

namespace TI {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(BooleanConstructor);

const ClassInfo BooleanConstructor::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(BooleanConstructor) };

BooleanConstructor::BooleanConstructor(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
}

void BooleanConstructor::finishCreation(VM& vm, BooleanPrototype* booleanPrototype)
{
    Base::finishCreation(vm, booleanPrototype->classInfo()->className);
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, booleanPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(1), ReadOnly | DontDelete | DontEnum);
}

// ECMA 15.6.2
JSObject* constructBoolean(ExecState* exec, const ArgList& args)
{
    BooleanObject* obj = BooleanObject::create(exec->vm(), asInternalFunction(exec->callee())->globalObject()->booleanObjectStructure());
    obj->setInternalValue(exec->vm(), jsBoolean(args.at(0).toBoolean(exec)));
    return obj;
}

static EncodedTiValue JSC_HOST_CALL constructWithBooleanConstructor(ExecState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructBoolean(exec, args));
}

ConstructType BooleanConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithBooleanConstructor;
    return ConstructTypeHost;
}

// ECMA 15.6.1
static EncodedTiValue JSC_HOST_CALL callBooleanConstructor(ExecState* exec)
{
    return TiValue::encode(jsBoolean(exec->argument(0).toBoolean(exec)));
}

CallType BooleanConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callBooleanConstructor;
    return CallTypeHost;
}

JSObject* constructBooleanFromImmediateBoolean(ExecState* exec, JSGlobalObject* globalObject, TiValue immediateBooleanValue)
{
    BooleanObject* obj = BooleanObject::create(exec->vm(), globalObject->booleanObjectStructure());
    obj->setInternalValue(exec->vm(), immediateBooleanValue);
    return obj;
}

} // namespace TI
