/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "MapPrototype.h"

#include "CachedCall.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "JSCTiValueInlines.h"
#include "JSFunctionInlines.h"
#include "JSMap.h"
#include "JSMapIterator.h"
#include "MapData.h"

namespace TI {

const ClassInfo MapPrototype::s_info = { "Map", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(MapPrototype) };

static EncodedTiValue JSC_HOST_CALL mapProtoFuncClear(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncDelete(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncForEach(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncGet(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncHas(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncSet(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncKeys(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncValues(ExecState*);
static EncodedTiValue JSC_HOST_CALL mapProtoFuncEntries(ExecState*);

static EncodedTiValue JSC_HOST_CALL mapProtoFuncSize(ExecState*);
    
void MapPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);

    JSC_NATIVE_FUNCTION(vm.propertyNames->clear, mapProtoFuncClear, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->deleteKeyword, mapProtoFuncDelete, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->forEach, mapProtoFuncForEach, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->get, mapProtoFuncGet, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->has, mapProtoFuncHas, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->set, mapProtoFuncSet, DontEnum, 2);
    JSC_NATIVE_FUNCTION(vm.propertyNames->keys, mapProtoFuncKeys, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->values, mapProtoFuncValues, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->entries, mapProtoFuncEntries, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->iteratorPrivateName, mapProtoFuncEntries, DontEnum, 0);

    GetterSetter* accessor = GetterSetter::create(vm);
    JSFunction* function = JSFunction::create(vm, globalObject, 0, vm.propertyNames->size.string(), mapProtoFuncSize);
    accessor->setGetter(vm, function);
    putDirectNonIndexAccessor(vm, vm.propertyNames->size, accessor, DontEnum | Accessor);
}

ALWAYS_INLINE static MapData* getMapData(CallFrame* callFrame, TiValue thisValue)
{
    if (!thisValue.isObject()) {
        throwVMError(callFrame, createNotAnObjectError(callFrame, thisValue));
        return 0;
    }
    JSMap* map = jsDynamicCast<JSMap*>(thisValue);
    if (!map) {
        throwTypeError(callFrame, ASCIILiteral("Map operation called on non-Map object"));
        return 0;
    }
    return map->mapData();
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncClear(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    data->clear();
    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncDelete(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    return TiValue::encode(jsBoolean(data->remove(callFrame, callFrame->argument(0))));
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncForEach(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    TiValue callBack = callFrame->argument(0);
    CallData callData;
    CallType callType = getCallData(callBack, callData);
    if (callType == CallTypeNone)
        return TiValue::encode(throwTypeError(callFrame, WTI::ASCIILiteral("Map.prototype.forEach called without callback")));
    TiValue thisValue = callFrame->argument(1);
    VM* vm = &callFrame->vm();
    if (callType == CallTypeJS) {
        JSFunction* function = jsCast<JSFunction*>(callBack);
        CachedCall cachedCall(callFrame, function, 2);
        for (auto ptr = data->begin(), end = data->end(); ptr != end && !vm->exception(); ++ptr) {
            cachedCall.setThis(thisValue);
            cachedCall.setArgument(0, ptr.value());
            cachedCall.setArgument(1, ptr.key());
            cachedCall.call();
        }
    } else {
        for (auto ptr = data->begin(), end = data->end(); ptr != end && !vm->exception(); ++ptr) {
            MarkedArgumentBuffer args;
            args.append(ptr.value());
            args.append(ptr.key());
            TI::call(callFrame, callBack, callType, callData, thisValue, args);
        }
    }
    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncGet(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    TiValue result = data->get(callFrame, callFrame->argument(0));
    if (!result)
        result = jsUndefined();
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncHas(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    return TiValue::encode(jsBoolean(data->contains(callFrame, callFrame->argument(0))));
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncSet(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    data->set(callFrame, callFrame->argument(0), callFrame->argument(1));
    return TiValue::encode(callFrame->thisValue());
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncSize(CallFrame* callFrame)
{
    MapData* data = getMapData(callFrame, callFrame->thisValue());
    if (!data)
        return TiValue::encode(jsUndefined());
    return TiValue::encode(jsNumber(data->size(callFrame)));
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncValues(CallFrame* callFrame)
{
    JSMap* thisObj = jsDynamicCast<JSMap*>(callFrame->thisValue());
    if (!thisObj)
        return TiValue::encode(throwTypeError(callFrame, ASCIILiteral("Cannot create a Map value iterator for a non-Map object.")));
    return TiValue::encode(JSMapIterator::create(callFrame->vm(), callFrame->callee()->globalObject()->mapIteratorStructure(), thisObj, MapIterateValue));
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncEntries(CallFrame* callFrame)
{
    JSMap* thisObj = jsDynamicCast<JSMap*>(callFrame->thisValue());
    if (!thisObj)
        return TiValue::encode(throwTypeError(callFrame, ASCIILiteral("Cannot create a Map key iterator for a non-Map object.")));
    return TiValue::encode(JSMapIterator::create(callFrame->vm(), callFrame->callee()->globalObject()->mapIteratorStructure(), thisObj, MapIterateKeyValue));
}

EncodedTiValue JSC_HOST_CALL mapProtoFuncKeys(CallFrame* callFrame)
{
    JSMap* thisObj = jsDynamicCast<JSMap*>(callFrame->thisValue());
    if (!thisObj)
        return TiValue::encode(throwTypeError(callFrame, ASCIILiteral("Cannot create a Map entry iterator for a non-Map object.")));
    return TiValue::encode(JSMapIterator::create(callFrame->vm(), callFrame->callee()->globalObject()->mapIteratorStructure(), thisObj, MapIterateKey));
}

}
