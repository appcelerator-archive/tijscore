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
#include "WeakMapPrototype.h"

#include "JSCTiValueInlines.h"
#include "JSWeakMap.h"
#include "WeakMapData.h"

namespace TI {

const ClassInfo WeakMapPrototype::s_info = { "WeakMap", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(WeakMapPrototype) };

static EncodedTiValue JSC_HOST_CALL protoFuncWeakMapClear(ExecState*);
static EncodedTiValue JSC_HOST_CALL protoFuncWeakMapDelete(ExecState*);
static EncodedTiValue JSC_HOST_CALL protoFuncWeakMapGet(ExecState*);
static EncodedTiValue JSC_HOST_CALL protoFuncWeakMapHas(ExecState*);
static EncodedTiValue JSC_HOST_CALL protoFuncWeakMapSet(ExecState*);

void WeakMapPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);

    JSC_NATIVE_FUNCTION(vm.propertyNames->clear, protoFuncWeakMapClear, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->deleteKeyword, protoFuncWeakMapDelete, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->get, protoFuncWeakMapGet, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->has, protoFuncWeakMapHas, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->set, protoFuncWeakMapSet, DontEnum, 2);
}

static WeakMapData* getWeakMapData(CallFrame* callFrame, TiValue value)
{
    if (!value.isObject()) {
        throwTypeError(callFrame, WTI::ASCIILiteral("Called WeakMap function on non-object"));
        return 0;
    }

    if (JSWeakMap* weakMap = jsDynamicCast<JSWeakMap*>(value))
        return weakMap->weakMapData();

    throwTypeError(callFrame, WTI::ASCIILiteral("Called WeakMap function on a non-WeakMap object"));
    return 0;
}

EncodedTiValue JSC_HOST_CALL protoFuncWeakMapClear(CallFrame* callFrame)
{
    WeakMapData* map = getWeakMapData(callFrame, callFrame->thisValue());
    if (!map)
        return TiValue::encode(jsUndefined());
    map->clear();
    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL protoFuncWeakMapDelete(CallFrame* callFrame)
{
    WeakMapData* map = getWeakMapData(callFrame, callFrame->thisValue());
    if (!map)
        return TiValue::encode(jsUndefined());
    TiValue key = callFrame->argument(0);
    if (!key.isObject())
        return TiValue::encode(throwTypeError(callFrame, WTI::ASCIILiteral("A WeakMap cannot have a non-object key")));
    return TiValue::encode(jsBoolean(map->remove(asObject(key))));
}

EncodedTiValue JSC_HOST_CALL protoFuncWeakMapGet(CallFrame* callFrame)
{
    WeakMapData* map = getWeakMapData(callFrame, callFrame->thisValue());
    if (!map)
        return TiValue::encode(jsUndefined());
    TiValue key = callFrame->argument(0);
    if (!key.isObject())
        return TiValue::encode(throwTypeError(callFrame, WTI::ASCIILiteral("A WeakMap cannot have a non-object key")));
    return TiValue::encode(map->get(asObject(key)));
}

EncodedTiValue JSC_HOST_CALL protoFuncWeakMapHas(CallFrame* callFrame)
{
    WeakMapData* map = getWeakMapData(callFrame, callFrame->thisValue());
    if (!map)
        return TiValue::encode(jsUndefined());
    TiValue key = callFrame->argument(0);
    if (!key.isObject())
        return TiValue::encode(throwTypeError(callFrame, WTI::ASCIILiteral("A WeakMap cannot have a non-object key")));
    return TiValue::encode(jsBoolean(map->contains(asObject(key))));
}

EncodedTiValue JSC_HOST_CALL protoFuncWeakMapSet(CallFrame* callFrame)
{
    WeakMapData* map = getWeakMapData(callFrame, callFrame->thisValue());
    if (!map)
        return TiValue::encode(jsUndefined());
    TiValue key = callFrame->argument(0);
    if (!key.isObject())
        return TiValue::encode(throwTypeError(callFrame, WTI::ASCIILiteral("Attempted to set a non-object key in a WeakMap")));
    map->set(callFrame->vm(), asObject(key), callFrame->argument(1));
    return TiValue::encode(callFrame->thisValue());
}

}
