/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WeakMapConstructor.h"

#include "JSCTiValueInlines.h"
#include "JSCellInlines.h"
#include "JSGlobalObject.h"
#include "JSWeakMap.h"
#include "WeakMapPrototype.h"
#include "StructureInlines.h"
namespace TI {

const ClassInfo WeakMapConstructor::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(WeakMapConstructor) };

void WeakMapConstructor::finishCreation(VM& vm, WeakMapPrototype* prototype)
{
    Base::finishCreation(vm, prototype->classInfo()->className);
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, prototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontEnum | DontDelete);
}

static EncodedTiValue JSC_HOST_CALL constructWeakMap(ExecState* exec)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();
    Structure* structure = globalObject->weakMapStructure();
    return TiValue::encode(JSWeakMap::create(exec, structure));
}

ConstructType WeakMapConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWeakMap;
    return ConstructTypeHost;
}

CallType WeakMapConstructor::getCallData(JSCell*, CallData&)
{
    return CallTypeNone;
}

}
