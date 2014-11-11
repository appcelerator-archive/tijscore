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
#if ENABLE(JIT)
#include "JITOperations.h"

#include "Arguments.h"
#include "ArrayConstructor.h"
#include "CallFrameInlines.h"
#include "CommonSlowPaths.h"
#include "DFGCompilationMode.h"
#include "DFGDriver.h"
#include "DFGOSREntry.h"
#include "DFGWorklist.h"
#include "Error.h"
#include "GetterSetter.h"
#include "HostCallReturnValue.h"
#include "JIT.h"
#include "JITOperationWrappers.h"
#include "JITToDFGDeferredCompilationCallback.h"
#include "JSGlobalObjectFunctions.h"
#include "JSNameScope.h"
#include "JSPropertyNameIterator.h"
#include "JSStackInlines.h"
#include "JSWithScope.h"
#include "ObjectConstructor.h"
#include "Operations.h"
#include "Repatch.h"
#include "RepatchBuffer.h"

namespace TI {

extern "C" {

#if COMPILER(MSVC)
void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

#define OUR_RETURN_ADDRESS _ReturnAddress()
#else
#define OUR_RETURN_ADDRESS __builtin_return_address(0)
#endif

#if ENABLE(OPCODE_SAMPLING)
#define CTI_SAMPLER vm->interpreter->sampler()
#else
#define CTI_SAMPLER 0
#endif


void JIT_OPERATION operationStackCheck(ExecState* exec, CodeBlock* codeBlock)
{
    // We pass in our own code block, because the callframe hasn't been populated.
    VM* vm = codeBlock->vm();
    CallFrame* callerFrame = exec->callerFrameSkippingVMEntrySentinel();
    if (!callerFrame)
        callerFrame = exec;

    NativeCallFrameTracer tracer(vm, callerFrame);

    JSStack& stack = vm->interpreter->stack();

    if (UNLIKELY(!stack.grow(&exec->registers()[virtualRegisterForLocal(codeBlock->frameRegisterCount()).offset()])))
        vm->throwException(callerFrame, createStackOverflowError(callerFrame));
}

int32_t JIT_OPERATION operationCallArityCheck(ExecState* exec)
{
    VM* vm = &exec->vm();
    CallFrame* callerFrame = exec->callerFrameSkippingVMEntrySentinel();
    NativeCallFrameTracer tracer(vm, callerFrame);

    JSStack& stack = vm->interpreter->stack();

    int32_t missingArgCount = CommonSlowPaths::arityCheckFor(exec, &stack, CodeForCall);
    if (missingArgCount < 0)
        vm->throwException(callerFrame, createStackOverflowError(callerFrame));

    return missingArgCount;
}

int32_t JIT_OPERATION operationConstructArityCheck(ExecState* exec)
{
    VM* vm = &exec->vm();
    CallFrame* callerFrame = exec->callerFrameSkippingVMEntrySentinel();
    NativeCallFrameTracer tracer(vm, callerFrame);

    JSStack& stack = vm->interpreter->stack();

    int32_t missingArgCount = CommonSlowPaths::arityCheckFor(exec, &stack, CodeForConstruct);
    if (missingArgCount < 0)
        vm->throwException(callerFrame, createStackOverflowError(callerFrame));

    return missingArgCount;
}

EncodedTiValue JIT_OPERATION operationGetById(ExecState* exec, StructureStubInfo*, EncodedTiValue base, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue baseValue = TiValue::decode(base);
    PropertySlot slot(baseValue);
    Identifier ident(vm, uid);
    return TiValue::encode(baseValue.get(exec, ident, slot));
}

EncodedTiValue JIT_OPERATION operationGetByIdBuildList(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue base, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue baseValue = TiValue::decode(base);
    PropertySlot slot(baseValue);
    TiValue result = baseValue.get(exec, ident, slot);

    if (accessType == static_cast<AccessType>(stubInfo->accessType))
        buildGetByIDList(exec, baseValue, ident, slot, *stubInfo);

    return TiValue::encode(result);
}

EncodedTiValue JIT_OPERATION operationGetByIdOptimize(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue base, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    Identifier ident = uid->isEmptyUnique() ? Identifier::from(PrivateName(uid)) : Identifier(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue baseValue = TiValue::decode(base);
    PropertySlot slot(baseValue);
    TiValue result = baseValue.get(exec, ident, slot);
    
    if (accessType == static_cast<AccessType>(stubInfo->accessType)) {
        if (stubInfo->seen)
            repatchGetByID(exec, baseValue, ident, slot, *stubInfo);
        else
            stubInfo->seen = true;
    }

    return TiValue::encode(result);
}

EncodedTiValue JIT_OPERATION operationInOptimize(ExecState* exec, StructureStubInfo* stubInfo, JSCell* base, StringImpl* key)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    if (!base->isObject()) {
        vm->throwException(exec, createInvalidParameterError(exec, "in", base));
        return TiValue::encode(jsUndefined());
    }
    
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    Identifier ident(vm, key);
    PropertySlot slot(base);
    bool result = asObject(base)->getPropertySlot(exec, ident, slot);
    
    RELEASE_ASSERT(accessType == stubInfo->accessType);
    
    if (stubInfo->seen)
        repatchIn(exec, base, ident, result, slot, *stubInfo);
    else
        stubInfo->seen = true;
    
    return TiValue::encode(jsBoolean(result));
}

EncodedTiValue JIT_OPERATION operationIn(ExecState* exec, StructureStubInfo*, JSCell* base, StringImpl* key)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    if (!base->isObject()) {
        vm->throwException(exec, createInvalidParameterError(exec, "in", base));
        return TiValue::encode(jsUndefined());
    }

    Identifier ident(vm, key);
    return TiValue::encode(jsBoolean(asObject(base)->hasProperty(exec, ident)));
}

EncodedTiValue JIT_OPERATION operationGenericIn(ExecState* exec, JSCell* base, EncodedTiValue key)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return TiValue::encode(jsBoolean(CommonSlowPaths::opIn(exec, TiValue::decode(key), base)));
}

EncodedTiValue JIT_OPERATION operationCallCustomGetter(ExecState* exec, JSCell* base, PropertySlot::GetValueFunc function, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    
    return function(exec, TiValue::encode(base), TiValue::encode(base), ident);
}

EncodedTiValue JIT_OPERATION operationCallGetter(ExecState* exec, JSCell* base, JSCell* getterSetter)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return TiValue::encode(callGetter(exec, base, getterSetter));
}

void JIT_OPERATION operationPutByIdStrict(ExecState* exec, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    PutPropertySlot slot(TiValue::decode(encodedBase), true, exec->codeBlock()->putByIdContext());
    TiValue::decode(encodedBase).put(exec, ident, TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutByIdNonStrict(ExecState* exec, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    PutPropertySlot slot(TiValue::decode(encodedBase), false, exec->codeBlock()->putByIdContext());
    TiValue::decode(encodedBase).put(exec, ident, TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutByIdDirectStrict(ExecState* exec, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    PutPropertySlot slot(TiValue::decode(encodedBase), true, exec->codeBlock()->putByIdContext());
    asObject(TiValue::decode(encodedBase))->putDirect(exec->vm(), ident, TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutByIdDirectNonStrict(ExecState* exec, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    PutPropertySlot slot(TiValue::decode(encodedBase), false, exec->codeBlock()->putByIdContext());
    asObject(TiValue::decode(encodedBase))->putDirect(exec->vm(), ident, TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutByIdStrictOptimize(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    TiValue baseValue = TiValue::decode(encodedBase);
    PutPropertySlot slot(baseValue, true, exec->codeBlock()->putByIdContext());
    
    baseValue.put(exec, ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    if (stubInfo->seen)
        repatchPutByID(exec, baseValue, ident, slot, *stubInfo, NotDirect);
    else
        stubInfo->seen = true;
}

void JIT_OPERATION operationPutByIdNonStrictOptimize(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    TiValue baseValue = TiValue::decode(encodedBase);
    PutPropertySlot slot(baseValue, false, exec->codeBlock()->putByIdContext());
    
    baseValue.put(exec, ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    if (stubInfo->seen)
        repatchPutByID(exec, baseValue, ident, slot, *stubInfo, NotDirect);
    else
        stubInfo->seen = true;
}

void JIT_OPERATION operationPutByIdDirectStrictOptimize(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    JSObject* baseObject = asObject(TiValue::decode(encodedBase));
    PutPropertySlot slot(baseObject, true, exec->codeBlock()->putByIdContext());
    
    baseObject->putDirect(exec->vm(), ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    if (stubInfo->seen)
        repatchPutByID(exec, baseObject, ident, slot, *stubInfo, Direct);
    else
        stubInfo->seen = true;
}

void JIT_OPERATION operationPutByIdDirectNonStrictOptimize(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    JSObject* baseObject = asObject(TiValue::decode(encodedBase));
    PutPropertySlot slot(baseObject, false, exec->codeBlock()->putByIdContext());
    
    baseObject->putDirect(exec->vm(), ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    if (stubInfo->seen)
        repatchPutByID(exec, baseObject, ident, slot, *stubInfo, Direct);
    else
        stubInfo->seen = true;
}

void JIT_OPERATION operationPutByIdStrictBuildList(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    TiValue baseValue = TiValue::decode(encodedBase);
    PutPropertySlot slot(baseValue, true, exec->codeBlock()->putByIdContext());
    
    baseValue.put(exec, ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    buildPutByIdList(exec, baseValue, ident, slot, *stubInfo, NotDirect);
}

void JIT_OPERATION operationPutByIdNonStrictBuildList(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    TiValue baseValue = TiValue::decode(encodedBase);
    PutPropertySlot slot(baseValue, false, exec->codeBlock()->putByIdContext());
    
    baseValue.put(exec, ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    buildPutByIdList(exec, baseValue, ident, slot, *stubInfo, NotDirect);
}

void JIT_OPERATION operationPutByIdDirectStrictBuildList(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);
    
    TiValue value = TiValue::decode(encodedValue);
    JSObject* baseObject = asObject(TiValue::decode(encodedBase));
    PutPropertySlot slot(baseObject, true, exec->codeBlock()->putByIdContext());
    
    baseObject->putDirect(exec->vm(), ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    buildPutByIdList(exec, baseObject, ident, slot, *stubInfo, Direct);
}

void JIT_OPERATION operationPutByIdDirectNonStrictBuildList(ExecState* exec, StructureStubInfo* stubInfo, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl* uid)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    Identifier ident(vm, uid);
    AccessType accessType = static_cast<AccessType>(stubInfo->accessType);

    TiValue value = TiValue::decode(encodedValue);
    JSObject* baseObject = asObject(TiValue::decode(encodedBase));
    PutPropertySlot slot(baseObject, false, exec->codeBlock()->putByIdContext());
    
    baseObject ->putDirect(exec->vm(), ident, value, slot);
    
    if (accessType != static_cast<AccessType>(stubInfo->accessType))
        return;
    
    buildPutByIdList(exec, baseObject, ident, slot, *stubInfo, Direct);
}

void JIT_OPERATION operationReallocateStorageAndFinishPut(ExecState* exec, JSObject* base, Structure* structure, PropertyOffset offset, EncodedTiValue value)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    ASSERT(structure->outOfLineCapacity() > base->structure()->outOfLineCapacity());
    ASSERT(!vm.heap.storageAllocator().fastPathShouldSucceed(structure->outOfLineCapacity() * sizeof(TiValue)));
    base->setStructureAndReallocateStorageIfNecessary(vm, structure);
    base->putDirect(vm, offset, TiValue::decode(value));
}

static void putByVal(CallFrame* callFrame, TiValue baseValue, TiValue subscript, TiValue value)
{
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (baseValue.isObject()) {
            JSObject* object = asObject(baseValue);
            if (object->canSetIndexQuickly(i))
                object->setIndexQuickly(callFrame->vm(), i, value);
            else
                object->methodTable()->putByIndex(object, callFrame, i, value, callFrame->codeBlock()->isStrictMode());
        } else
            baseValue.putByIndex(callFrame, i, value, callFrame->codeBlock()->isStrictMode());
    } else if (isName(subscript)) {
        PutPropertySlot slot(baseValue, callFrame->codeBlock()->isStrictMode());
        baseValue.put(callFrame, jsCast<NameInstance*>(subscript.asCell())->privateName(), value, slot);
    } else {
        Identifier property(callFrame, subscript.toString(callFrame)->value(callFrame));
        if (!callFrame->vm().exception()) { // Don't put to an object if toString threw an exception.
            PutPropertySlot slot(baseValue, callFrame->codeBlock()->isStrictMode());
            baseValue.put(callFrame, property, value, slot);
        }
    }
}

static void directPutByVal(CallFrame* callFrame, JSObject* baseObject, TiValue subscript, TiValue value)
{
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        baseObject->putDirectIndex(callFrame, i, value);
    } else if (isName(subscript)) {
        PutPropertySlot slot(baseObject, callFrame->codeBlock()->isStrictMode());
        baseObject->putDirect(callFrame->vm(), jsCast<NameInstance*>(subscript.asCell())->privateName(), value, slot);
    } else {
        Identifier property(callFrame, subscript.toString(callFrame)->value(callFrame));
        if (!callFrame->vm().exception()) { // Don't put to an object if toString threw an exception.
            PutPropertySlot slot(baseObject, callFrame->codeBlock()->isStrictMode());
            baseObject->putDirect(callFrame->vm(), property, value, slot);
        }
    }
}
void JIT_OPERATION operationPutByVal(ExecState* exec, EncodedTiValue encodedBaseValue, EncodedTiValue encodedSubscript, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    TiValue baseValue = TiValue::decode(encodedBaseValue);
    TiValue subscript = TiValue::decode(encodedSubscript);
    TiValue value = TiValue::decode(encodedValue);

    if (baseValue.isObject() && subscript.isInt32()) {
        // See if it's worth optimizing at all.
        JSObject* object = asObject(baseValue);
        bool didOptimize = false;

        unsigned bytecodeOffset = exec->locationAsBytecodeOffset();
        ASSERT(bytecodeOffset);
        ByValInfo& byValInfo = exec->codeBlock()->getByValInfo(bytecodeOffset - 1);
        ASSERT(!byValInfo.stubRoutine);

        if (hasOptimizableIndexing(object->structure())) {
            // Attempt to optimize.
            JITArrayMode arrayMode = jitArrayModeForStructure(object->structure());
            if (arrayMode != byValInfo.arrayMode) {
                JIT::compilePutByVal(&vm, exec->codeBlock(), &byValInfo, ReturnAddressPtr(OUR_RETURN_ADDRESS), arrayMode);
                didOptimize = true;
            }
        }

        if (!didOptimize) {
            // If we take slow path more than 10 times without patching then make sure we
            // never make that mistake again. Or, if we failed to patch and we have some object
            // that intercepts indexed get, then don't even wait until 10 times. For cases
            // where we see non-index-intercepting objects, this gives 10 iterations worth of
            // opportunity for us to observe that the get_by_val may be polymorphic.
            if (++byValInfo.slowPathCount >= 10
                || object->structure()->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero()) {
                // Don't ever try to optimize.
                RepatchBuffer repatchBuffer(exec->codeBlock());
                repatchBuffer.relinkCallerToFunction(ReturnAddressPtr(OUR_RETURN_ADDRESS), FunctionPtr(operationPutByValGeneric));
            }
        }
    }

    putByVal(exec, baseValue, subscript, value);
}

void JIT_OPERATION operationDirectPutByVal(ExecState* callFrame, EncodedTiValue encodedBaseValue, EncodedTiValue encodedSubscript, EncodedTiValue encodedValue)
{
    VM& vm = callFrame->vm();
    NativeCallFrameTracer tracer(&vm, callFrame);
    
    TiValue baseValue = TiValue::decode(encodedBaseValue);
    TiValue subscript = TiValue::decode(encodedSubscript);
    TiValue value = TiValue::decode(encodedValue);
    RELEASE_ASSERT(baseValue.isObject());
    JSObject* object = asObject(baseValue);
    if (subscript.isInt32()) {
        // See if it's worth optimizing at all.
        bool didOptimize = false;
        
        unsigned bytecodeOffset = callFrame->locationAsBytecodeOffset();
        ASSERT(bytecodeOffset);
        ByValInfo& byValInfo = callFrame->codeBlock()->getByValInfo(bytecodeOffset - 1);
        ASSERT(!byValInfo.stubRoutine);
        
        if (hasOptimizableIndexing(object->structure())) {
            // Attempt to optimize.
            JITArrayMode arrayMode = jitArrayModeForStructure(object->structure());
            if (arrayMode != byValInfo.arrayMode) {
                JIT::compileDirectPutByVal(&vm, callFrame->codeBlock(), &byValInfo, ReturnAddressPtr(OUR_RETURN_ADDRESS), arrayMode);
                didOptimize = true;
            }
        }
        
        if (!didOptimize) {
            // If we take slow path more than 10 times without patching then make sure we
            // never make that mistake again. Or, if we failed to patch and we have some object
            // that intercepts indexed get, then don't even wait until 10 times. For cases
            // where we see non-index-intercepting objects, this gives 10 iterations worth of
            // opportunity for us to observe that the get_by_val may be polymorphic.
            if (++byValInfo.slowPathCount >= 10
                || object->structure()->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero()) {
                // Don't ever try to optimize.
                RepatchBuffer repatchBuffer(callFrame->codeBlock());
                repatchBuffer.relinkCallerToFunction(ReturnAddressPtr(OUR_RETURN_ADDRESS), FunctionPtr(operationDirectPutByValGeneric));
            }
        }
    }
    directPutByVal(callFrame, object, subscript, value);
}

void JIT_OPERATION operationPutByValGeneric(ExecState* exec, EncodedTiValue encodedBaseValue, EncodedTiValue encodedSubscript, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    TiValue baseValue = TiValue::decode(encodedBaseValue);
    TiValue subscript = TiValue::decode(encodedSubscript);
    TiValue value = TiValue::decode(encodedValue);

    putByVal(exec, baseValue, subscript, value);
}


void JIT_OPERATION operationDirectPutByValGeneric(ExecState* exec, EncodedTiValue encodedBaseValue, EncodedTiValue encodedSubscript, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    TiValue baseValue = TiValue::decode(encodedBaseValue);
    TiValue subscript = TiValue::decode(encodedSubscript);
    TiValue value = TiValue::decode(encodedValue);
    RELEASE_ASSERT(baseValue.isObject());
    directPutByVal(exec, asObject(baseValue), subscript, value);
}

EncodedTiValue JIT_OPERATION operationCallEval(ExecState* execCallee)
{
    CallFrame* callerFrame = execCallee->callerFrame();
    ASSERT(execCallee->callerFrame()->codeBlock()->codeType() != FunctionCode
        || !execCallee->callerFrame()->codeBlock()->needsFullScopeChain()
        || execCallee->callerFrame()->uncheckedR(execCallee->callerFrame()->codeBlock()->activationRegister().offset()).jsValue());

    execCallee->setScope(callerFrame->scope());
    execCallee->setReturnPC(static_cast<Instruction*>(OUR_RETURN_ADDRESS));
    execCallee->setCodeBlock(0);

    if (!isHostFunction(execCallee->calleeAsValue(), globalFuncEval))
        return TiValue::encode(TiValue());

    VM* vm = &execCallee->vm();
    TiValue result = eval(execCallee);
    if (vm->exception())
        return EncodedTiValue();
    
    return TiValue::encode(result);
}

static void* handleHostCall(ExecState* execCallee, TiValue callee, CodeSpecializationKind kind)
{
    ExecState* exec = execCallee->callerFrame();
    VM* vm = &exec->vm();

    execCallee->setScope(exec->scope());
    execCallee->setCodeBlock(0);

    if (kind == CodeForCall) {
        CallData callData;
        CallType callType = getCallData(callee, callData);
    
        ASSERT(callType != CallTypeJS);
    
        if (callType == CallTypeHost) {
            NativeCallFrameTracer tracer(vm, execCallee);
            execCallee->setCallee(asObject(callee));
            vm->hostCallReturnValue = TiValue::decode(callData.native.function(execCallee));
            if (vm->exception())
                return vm->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();

            return reinterpret_cast<void*>(getHostCallReturnValue);
        }
    
        ASSERT(callType == CallTypeNone);
        exec->vm().throwException(exec, createNotAFunctionError(exec, callee));
        return vm->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();
    }

    ASSERT(kind == CodeForConstruct);
    
    ConstructData constructData;
    ConstructType constructType = getConstructData(callee, constructData);
    
    ASSERT(constructType != ConstructTypeJS);
    
    if (constructType == ConstructTypeHost) {
        NativeCallFrameTracer tracer(vm, execCallee);
        execCallee->setCallee(asObject(callee));
        vm->hostCallReturnValue = TiValue::decode(constructData.native.function(execCallee));
        if (vm->exception())
            return vm->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();

        return reinterpret_cast<void*>(getHostCallReturnValue);
    }
    
    ASSERT(constructType == ConstructTypeNone);
    exec->vm().throwException(exec, createNotAConstructorError(exec, callee));
    return vm->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress();
}

inline char* linkFor(ExecState* execCallee, CodeSpecializationKind kind)
{
    ExecState* exec = execCallee->callerFrame();
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue calleeAsValue = execCallee->calleeAsValue();
    JSCell* calleeAsFunctionCell = getJSFunction(calleeAsValue);
    if (!calleeAsFunctionCell)
        return reinterpret_cast<char*>(handleHostCall(execCallee, calleeAsValue, kind));

    JSFunction* callee = jsCast<JSFunction*>(calleeAsFunctionCell);
    execCallee->setScope(callee->scopeUnchecked());
    ExecutableBase* executable = callee->executable();

    MacroAssemblerCodePtr codePtr;
    CodeBlock* codeBlock = 0;
    CallLinkInfo& callLinkInfo = exec->codeBlock()->getCallLinkInfo(execCallee->returnPC());
    if (executable->isHostFunction())
        codePtr = executable->generatedJITCodeFor(kind)->addressForCall();
    else {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->prepareForExecution(execCallee, callee->scope(), kind);
        if (error) {
            vm->throwException(exec, createStackOverflowError(exec));
            return reinterpret_cast<char*>(vm->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress());
        }
        codeBlock = functionExecutable->codeBlockFor(kind);
        if (execCallee->argumentCountIncludingThis() < static_cast<size_t>(codeBlock->numParameters()) || callLinkInfo.callType == CallLinkInfo::CallVarargs)
            codePtr = functionExecutable->generatedJITCodeWithArityCheckFor(kind);
        else
            codePtr = functionExecutable->generatedJITCodeFor(kind)->addressForCall();
    }
    if (!callLinkInfo.seenOnce())
        callLinkInfo.setSeen();
    else
        linkFor(execCallee, callLinkInfo, codeBlock, callee, codePtr, kind);
    return reinterpret_cast<char*>(codePtr.executableAddress());
}

char* JIT_OPERATION operationLinkCall(ExecState* execCallee)
{
    return linkFor(execCallee, CodeForCall);
}

char* JIT_OPERATION operationLinkConstruct(ExecState* execCallee)
{
    return linkFor(execCallee, CodeForConstruct);
}

inline char* virtualForWithFunction(ExecState* execCallee, CodeSpecializationKind kind, JSCell*& calleeAsFunctionCell)
{
    ExecState* exec = execCallee->callerFrame();
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue calleeAsValue = execCallee->calleeAsValue();
    calleeAsFunctionCell = getJSFunction(calleeAsValue);
    if (UNLIKELY(!calleeAsFunctionCell))
        return reinterpret_cast<char*>(handleHostCall(execCallee, calleeAsValue, kind));
    
    JSFunction* function = jsCast<JSFunction*>(calleeAsFunctionCell);
    execCallee->setScope(function->scopeUnchecked());
    ExecutableBase* executable = function->executable();
    if (UNLIKELY(!executable->hasJITCodeFor(kind))) {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->prepareForExecution(execCallee, function->scope(), kind);
        if (error) {
            exec->vm().throwException(execCallee, error);
            return reinterpret_cast<char*>(vm->getCTIStub(throwExceptionFromCallSlowPathGenerator).code().executableAddress());
        }
    }
    return reinterpret_cast<char*>(executable->generatedJITCodeWithArityCheckFor(kind).executableAddress());
}

inline char* virtualFor(ExecState* execCallee, CodeSpecializationKind kind)
{
    JSCell* calleeAsFunctionCellIgnored;
    return virtualForWithFunction(execCallee, kind, calleeAsFunctionCellIgnored);
}

static bool attemptToOptimizeClosureCall(ExecState* execCallee, JSCell* calleeAsFunctionCell, CallLinkInfo& callLinkInfo)
{
    if (!calleeAsFunctionCell)
        return false;
    
    JSFunction* callee = jsCast<JSFunction*>(calleeAsFunctionCell);
    JSFunction* oldCallee = callLinkInfo.callee.get();
    
    if (!oldCallee
        || oldCallee->structure() != callee->structure()
        || oldCallee->executable() != callee->executable())
        return false;
    
    ASSERT(callee->executable()->hasJITCodeForCall());
    MacroAssemblerCodePtr codePtr = callee->executable()->generatedJITCodeForCall()->addressForCall();
    
    CodeBlock* codeBlock;
    if (callee->executable()->isHostFunction())
        codeBlock = 0;
    else {
        codeBlock = jsCast<FunctionExecutable*>(callee->executable())->codeBlockForCall();
        if (execCallee->argumentCountIncludingThis() < static_cast<size_t>(codeBlock->numParameters()))
            return false;
    }
    
    linkClosureCall(
        execCallee, callLinkInfo, codeBlock,
        callee->structure(), callee->executable(), codePtr);
    
    return true;
}

char* JIT_OPERATION operationLinkClosureCall(ExecState* execCallee)
{
    JSCell* calleeAsFunctionCell;
    char* result = virtualForWithFunction(execCallee, CodeForCall, calleeAsFunctionCell);
    CallLinkInfo& callLinkInfo = execCallee->callerFrame()->codeBlock()->getCallLinkInfo(execCallee->returnPC());

    if (!attemptToOptimizeClosureCall(execCallee, calleeAsFunctionCell, callLinkInfo))
        linkSlowFor(execCallee, callLinkInfo, CodeForCall);
    
    return result;
}

char* JIT_OPERATION operationVirtualCall(ExecState* execCallee)
{    
    return virtualFor(execCallee, CodeForCall);
}

char* JIT_OPERATION operationVirtualConstruct(ExecState* execCallee)
{
    return virtualFor(execCallee, CodeForConstruct);
}


size_t JIT_OPERATION operationCompareLess(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return jsLess<true>(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

size_t JIT_OPERATION operationCompareLessEq(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return jsLessEq<true>(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

size_t JIT_OPERATION operationCompareGreater(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return jsLess<false>(exec, TiValue::decode(encodedOp2), TiValue::decode(encodedOp1));
}

size_t JIT_OPERATION operationCompareGreaterEq(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return jsLessEq<false>(exec, TiValue::decode(encodedOp2), TiValue::decode(encodedOp1));
}

size_t JIT_OPERATION operationConvertTiValueToBoolean(ExecState* exec, EncodedTiValue encodedOp)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return TiValue::decode(encodedOp).toBoolean(exec);
}

size_t JIT_OPERATION operationCompareEq(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return TiValue::equalSlowCaseInline(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

#if USE(JSVALUE64)
EncodedTiValue JIT_OPERATION operationCompareStringEq(ExecState* exec, JSCell* left, JSCell* right)
#else
size_t JIT_OPERATION operationCompareStringEq(ExecState* exec, JSCell* left, JSCell* right)
#endif
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    bool result = asString(left)->value(exec) == asString(right)->value(exec);
#if USE(JSVALUE64)
    return TiValue::encode(jsBoolean(result));
#else
    return result;
#endif
}

size_t JIT_OPERATION operationHasProperty(ExecState* exec, JSObject* base, JSString* property)
{
    int result = base->hasProperty(exec, Identifier(exec, property->value(exec)));
    return result;
}
    

EncodedTiValue JIT_OPERATION operationNewArrayWithProfile(ExecState* exec, ArrayAllocationProfile* profile, const TiValue* values, int size)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    return TiValue::encode(constructArrayNegativeIndexed(exec, profile, values, size));
}

EncodedTiValue JIT_OPERATION operationNewArrayBufferWithProfile(ExecState* exec, ArrayAllocationProfile* profile, const TiValue* values, int size)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    return TiValue::encode(constructArray(exec, profile, values, size));
}

EncodedTiValue JIT_OPERATION operationNewArrayWithSizeAndProfile(ExecState* exec, ArrayAllocationProfile* profile, EncodedTiValue size)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    TiValue sizeValue = TiValue::decode(size);
    return TiValue::encode(constructArrayWithSizeQuirk(exec, profile, exec->lexicalGlobalObject(), sizeValue));
}

EncodedTiValue JIT_OPERATION operationNewFunction(ExecState* exec, JSCell* functionExecutable)
{
    ASSERT(functionExecutable->inherits(FunctionExecutable::info()));
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    return TiValue::encode(JSFunction::create(vm, static_cast<FunctionExecutable*>(functionExecutable), exec->scope()));
}

JSCell* JIT_OPERATION operationNewObject(ExecState* exec, Structure* structure)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return constructEmptyObject(exec, structure);
}

EncodedTiValue JIT_OPERATION operationNewRegexp(ExecState* exec, void* regexpPtr)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    RegExp* regexp = static_cast<RegExp*>(regexpPtr);
    if (!regexp->isValid()) {
        vm.throwException(exec, createSyntaxError(exec, "Invalid flags supplied to RegExp constructor."));
        return TiValue::encode(jsUndefined());
    }

    return TiValue::encode(RegExpObject::create(vm, exec->lexicalGlobalObject()->regExpStructure(), regexp));
}

void JIT_OPERATION operationHandleWatchdogTimer(ExecState* exec)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    if (UNLIKELY(vm.watchdog.didFire(exec)))
        vm.throwException(exec, createTerminatedExecutionException(&vm));
}

void JIT_OPERATION operationThrowStaticError(ExecState* exec, EncodedTiValue encodedValue, int32_t referenceErrorFlag)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    String message = errorDescriptionForValue(exec, TiValue::decode(encodedValue))->value(exec);
    if (referenceErrorFlag)
        vm.throwException(exec, createReferenceError(exec, message));
    else
        vm.throwException(exec, createTypeError(exec, message));
}

void JIT_OPERATION operationDebug(ExecState* exec, int32_t debugHookID)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    vm.interpreter->debug(exec, static_cast<DebugHookID>(debugHookID));
}

#if ENABLE(DFG_JIT)
char* JIT_OPERATION operationOptimize(ExecState* exec, int32_t bytecodeIndex)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    // Defer GC so that it doesn't run between when we enter into this slow path and
    // when we figure out the state of our code block. This prevents a number of
    // awkward reentrancy scenarios, including:
    //
    // - The optimized version of our code block being jettisoned by GC right after
    //   we concluded that we wanted to use it.
    //
    // - An optimized version of our code block being installed just as we decided
    //   that it wasn't ready yet.
    //
    // This still leaves the following: anytime we return from cti_optimize, we may
    // GC, and the GC may either jettison the optimized version of our code block,
    // or it may install the optimized version of our code block even though we
    // concluded that it wasn't ready yet.
    //
    // Note that jettisoning won't happen if we already initiated OSR, because in
    // that case we would have already planted the optimized code block into the JS
    // stack.
    DeferGC deferGC(vm.heap);
    
    CodeBlock* codeBlock = exec->codeBlock();

    if (bytecodeIndex) {
        // If we're attempting to OSR from a loop, assume that this should be
        // separately optimized.
        codeBlock->m_shouldAlwaysBeInlined = false;
    }

    if (Options::verboseOSR()) {
        dataLog(
            *codeBlock, ": Entered optimize with bytecodeIndex = ", bytecodeIndex,
            ", executeCounter = ", codeBlock->jitExecuteCounter(),
            ", optimizationDelayCounter = ", codeBlock->reoptimizationRetryCounter(),
            ", exitCounter = ");
        if (codeBlock->hasOptimizedReplacement())
            dataLog(codeBlock->replacement()->osrExitCounter());
        else
            dataLog("N/A");
        dataLog("\n");
    }

    if (!codeBlock->checkIfOptimizationThresholdReached()) {
        codeBlock->updateAllPredictions();
        if (Options::verboseOSR())
            dataLog("Choosing not to optimize ", *codeBlock, " yet, because the threshold hasn't been reached.\n");
        return 0;
    }
    
    if (codeBlock->m_shouldAlwaysBeInlined) {
        codeBlock->updateAllPredictions();
        codeBlock->optimizeAfterWarmUp();
        if (Options::verboseOSR())
            dataLog("Choosing not to optimize ", *codeBlock, " yet, because m_shouldAlwaysBeInlined == true.\n");
        return 0;
    }

    // We cannot be in the process of asynchronous compilation and also have an optimized
    // replacement.
    ASSERT(
        !vm.worklist
        || !(vm.worklist->compilationState(DFG::CompilationKey(codeBlock, DFG::DFGMode)) != DFG::Worklist::NotKnown
        && codeBlock->hasOptimizedReplacement()));

    DFG::Worklist::State worklistState;
    if (vm.worklist) {
        // The call to DFG::Worklist::completeAllReadyPlansForVM() will complete all ready
        // (i.e. compiled) code blocks. But if it completes ours, we also need to know
        // what the result was so that we don't plow ahead and attempt OSR or immediate
        // reoptimization. This will have already also set the appropriate JIT execution
        // count threshold depending on what happened, so if the compilation was anything
        // but successful we just want to return early. See the case for worklistState ==
        // DFG::Worklist::Compiled, below.
        
        // Note that we could have alternatively just called Worklist::compilationState()
        // here, and if it returned Compiled, we could have then called
        // completeAndScheduleOSR() below. But that would have meant that it could take
        // longer for code blocks to be completed: they would only complete when *their*
        // execution count trigger fired; but that could take a while since the firing is
        // racy. It could also mean that code blocks that never run again after being
        // compiled would sit on the worklist until next GC. That's fine, but it's
        // probably a waste of memory. Our goal here is to complete code blocks as soon as
        // possible in order to minimize the chances of us executing baseline code after
        // optimized code is already available.
        worklistState = vm.worklist->completeAllReadyPlansForVM(
            vm, DFG::CompilationKey(codeBlock, DFG::DFGMode));
    } else
        worklistState = DFG::Worklist::NotKnown;

    if (worklistState == DFG::Worklist::Compiling) {
        // We cannot be in the process of asynchronous compilation and also have an optimized
        // replacement.
        RELEASE_ASSERT(!codeBlock->hasOptimizedReplacement());
        codeBlock->setOptimizationThresholdBasedOnCompilationResult(CompilationDeferred);
        return 0;
    }

    if (worklistState == DFG::Worklist::Compiled) {
        // If we don't have an optimized replacement but we did just get compiled, then
        // the compilation failed or was invalidated, in which case the execution count
        // thresholds have already been set appropriately by
        // CodeBlock::setOptimizationThresholdBasedOnCompilationResult() and we have
        // nothing left to do.
        if (!codeBlock->hasOptimizedReplacement()) {
            codeBlock->updateAllPredictions();
            if (Options::verboseOSR())
                dataLog("Code block ", *codeBlock, " was compiled but it doesn't have an optimized replacement.\n");
            return 0;
        }
    } else if (codeBlock->hasOptimizedReplacement()) {
        if (Options::verboseOSR())
            dataLog("Considering OSR ", *codeBlock, " -> ", *codeBlock->replacement(), ".\n");
        // If we have an optimized replacement, then it must be the case that we entered
        // cti_optimize from a loop. That's because if there's an optimized replacement,
        // then all calls to this function will be relinked to the replacement and so
        // the prologue OSR will never fire.
        
        // This is an interesting threshold check. Consider that a function OSR exits
        // in the middle of a loop, while having a relatively low exit count. The exit
        // will reset the execution counter to some target threshold, meaning that this
        // code won't be reached until that loop heats up for >=1000 executions. But then
        // we do a second check here, to see if we should either reoptimize, or just
        // attempt OSR entry. Hence it might even be correct for
        // shouldReoptimizeFromLoopNow() to always return true. But we make it do some
        // additional checking anyway, to reduce the amount of recompilation thrashing.
        if (codeBlock->replacement()->shouldReoptimizeFromLoopNow()) {
            if (Options::verboseOSR()) {
                dataLog(
                    "Triggering reoptimization of ", *codeBlock,
                    "(", *codeBlock->replacement(), ") (in loop).\n");
            }
            codeBlock->replacement()->jettison(CountReoptimization);
            return 0;
        }
    } else {
        if (!codeBlock->shouldOptimizeNow()) {
            if (Options::verboseOSR()) {
                dataLog(
                    "Delaying optimization for ", *codeBlock,
                    " because of insufficient profiling.\n");
            }
            return 0;
        }

        if (Options::verboseOSR())
            dataLog("Triggering optimized compilation of ", *codeBlock, "\n");

        unsigned numVarsWithValues;
        if (bytecodeIndex)
            numVarsWithValues = codeBlock->m_numVars;
        else
            numVarsWithValues = 0;
        Operands<TiValue> mustHandleValues(codeBlock->numParameters(), numVarsWithValues);
        for (size_t i = 0; i < mustHandleValues.size(); ++i) {
            int operand = mustHandleValues.operandForIndex(i);
            if (operandIsArgument(operand)
                && !VirtualRegister(operand).toArgument()
                && codeBlock->codeType() == FunctionCode
                && codeBlock->specializationKind() == CodeForConstruct) {
                // Ugh. If we're in a constructor, the 'this' argument may hold garbage. It will
                // also never be used. It doesn't matter what we put into the value for this,
                // but it has to be an actual value that can be grokked by subsequent DFG passes,
                // so we sanitize it here by turning it into Undefined.
                mustHandleValues[i] = jsUndefined();
            } else
                mustHandleValues[i] = exec->uncheckedR(operand).jsValue();
        }

        CompilationResult result = DFG::compile(
            vm, codeBlock->newReplacement().get(), DFG::DFGMode, bytecodeIndex,
            mustHandleValues, JITToDFGDeferredCompilationCallback::create(),
            vm.ensureWorklist());
        
        if (result != CompilationSuccessful)
            return 0;
    }
    
    CodeBlock* optimizedCodeBlock = codeBlock->replacement();
    ASSERT(JITCode::isOptimizingJIT(optimizedCodeBlock->jitType()));
    
    if (void* address = DFG::prepareOSREntry(exec, optimizedCodeBlock, bytecodeIndex)) {
        if (Options::verboseOSR()) {
            dataLog(
                "Performing OSR ", *codeBlock, " -> ", *optimizedCodeBlock, ", address ",
                RawPointer(OUR_RETURN_ADDRESS), " -> ", RawPointer(address), ".\n");
        }

        codeBlock->optimizeSoon();
        return static_cast<char*>(address);
    }

    if (Options::verboseOSR()) {
        dataLog(
            "Optimizing ", *codeBlock, " -> ", *codeBlock->replacement(),
            " succeeded, OSR failed, after a delay of ",
            codeBlock->optimizationDelayCounter(), ".\n");
    }

    // Count the OSR failure as a speculation failure. If this happens a lot, then
    // reoptimize.
    optimizedCodeBlock->countOSRExit();

    // We are a lot more conservative about triggering reoptimization after OSR failure than
    // before it. If we enter the optimize_from_loop trigger with a bucket full of fail
    // already, then we really would like to reoptimize immediately. But this case covers
    // something else: there weren't many (or any) speculation failures before, but we just
    // failed to enter the speculative code because some variable had the wrong value or
    // because the OSR code decided for any spurious reason that it did not want to OSR
    // right now. So, we only trigger reoptimization only upon the more conservative (non-loop)
    // reoptimization trigger.
    if (optimizedCodeBlock->shouldReoptimizeNow()) {
        if (Options::verboseOSR()) {
            dataLog(
                "Triggering reoptimization of ", *codeBlock, " -> ",
                *codeBlock->replacement(), " (after OSR fail).\n");
        }
        optimizedCodeBlock->jettison(CountReoptimization);
        return 0;
    }

    // OSR failed this time, but it might succeed next time! Let the code run a bit
    // longer and then try again.
    codeBlock->optimizeAfterWarmUp();
    
    return 0;
}
#endif

void JIT_OPERATION operationPutByIndex(ExecState* exec, EncodedTiValue encodedArrayValue, int32_t index, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    TiValue arrayValue = TiValue::decode(encodedArrayValue);
    ASSERT(isJSArray(arrayValue));
    asArray(arrayValue)->putDirectIndex(exec, index, TiValue::decode(encodedValue));
}

#if USE(JSVALUE64)
void JIT_OPERATION operationPutGetterSetter(ExecState* exec, EncodedTiValue encodedObjectValue, Identifier* identifier, EncodedTiValue encodedGetterValue, EncodedTiValue encodedSetterValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    ASSERT(TiValue::decode(encodedObjectValue).isObject());
    JSObject* baseObj = asObject(TiValue::decode(encodedObjectValue));

    GetterSetter* accessor = GetterSetter::create(vm);

    TiValue getter = TiValue::decode(encodedGetterValue);
    TiValue setter = TiValue::decode(encodedSetterValue);
    ASSERT(getter.isObject() || getter.isUndefined());
    ASSERT(setter.isObject() || setter.isUndefined());
    ASSERT(getter.isObject() || setter.isObject());

    if (!getter.isUndefined())
        accessor->setGetter(vm, asObject(getter));
    if (!setter.isUndefined())
        accessor->setSetter(vm, asObject(setter));
    baseObj->putDirectAccessor(exec, *identifier, accessor, Accessor);
}
#else
void JIT_OPERATION operationPutGetterSetter(ExecState* exec, JSCell* object, Identifier* identifier, JSCell* getter, JSCell* setter)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    ASSERT(object && object->isObject());
    JSObject* baseObj = object->getObject();

    GetterSetter* accessor = GetterSetter::create(vm);

    ASSERT(!getter || getter->isObject());
    ASSERT(!setter || setter->isObject());
    ASSERT(getter || setter);

    if (getter)
        accessor->setGetter(vm, getter->getObject());
    if (setter)
        accessor->setSetter(vm, setter->getObject());
    baseObj->putDirectAccessor(exec, *identifier, accessor, Accessor);
}
#endif

void JIT_OPERATION operationPushNameScope(ExecState* exec, Identifier* identifier, EncodedTiValue encodedValue, int32_t attibutes)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    JSNameScope* scope = JSNameScope::create(exec, *identifier, TiValue::decode(encodedValue), attibutes);

    exec->setScope(scope);
}

void JIT_OPERATION operationPushWithScope(ExecState* exec, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    JSObject* o = TiValue::decode(encodedValue).toObject(exec);
    if (vm.exception())
        return;

    exec->setScope(JSWithScope::create(exec, o));
}

void JIT_OPERATION operationPopScope(ExecState* exec)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    exec->setScope(exec->scope()->next());
}

void JIT_OPERATION operationProfileDidCall(ExecState* exec, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(exec, TiValue::decode(encodedValue));
}

void JIT_OPERATION operationProfileWillCall(ExecState* exec, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(exec, TiValue::decode(encodedValue));
}

EncodedTiValue JIT_OPERATION operationCheckHasInstance(ExecState* exec, EncodedTiValue encodedValue, EncodedTiValue encodedBaseVal)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue value = TiValue::decode(encodedValue);
    TiValue baseVal = TiValue::decode(encodedBaseVal);

    if (baseVal.isObject()) {
        JSObject* baseObject = asObject(baseVal);
        ASSERT(!baseObject->structure()->typeInfo().implementsDefaultHasInstance());
        if (baseObject->structure()->typeInfo().implementsHasInstance()) {
            bool result = baseObject->methodTable()->customHasInstance(baseObject, exec, value);
            return TiValue::encode(jsBoolean(result));
        }
    }

    vm->throwException(exec, createInvalidParameterError(exec, "instanceof", baseVal));
    return TiValue::encode(TiValue());
}

JSCell* JIT_OPERATION operationCreateActivation(ExecState* exec, int32_t offset)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    JSActivation* activation = JSActivation::create(vm, exec, exec->registers() + offset, exec->codeBlock());
    exec->setScope(activation);
    return activation;
}

JSCell* JIT_OPERATION operationCreateArguments(ExecState* exec)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    // NB: This needs to be exceedingly careful with top call frame tracking, since it
    // may be called from OSR exit, while the state of the call stack is bizarre.
    Arguments* result = Arguments::create(vm, exec);
    ASSERT(!vm.exception());
    return result;
}

EncodedTiValue JIT_OPERATION operationGetArgumentsLength(ExecState* exec, int32_t argumentsRegister)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    // Here we can assume that the argumernts were created. Because otherwise the JIT code would
    // have not made this call.
    Identifier ident(&vm, "length");
    TiValue baseValue = exec->uncheckedR(argumentsRegister).jsValue();
    PropertySlot slot(baseValue);
    return TiValue::encode(baseValue.get(exec, ident, slot));
}

}

static TiValue getByVal(ExecState* exec, TiValue baseValue, TiValue subscript, ReturnAddressPtr returnAddress)
{
    if (LIKELY(baseValue.isCell() && subscript.isString())) {
        if (TiValue result = baseValue.asCell()->fastGetOwnProperty(exec, asString(subscript)->value(exec)))
            return result;
    }

    if (subscript.isUInt32()) {
        uint32_t i = subscript.asUInt32();
        if (isJSString(baseValue) && asString(baseValue)->canGetIndex(i)) {
            ctiPatchCallByReturnAddress(exec->codeBlock(), returnAddress, FunctionPtr(operationGetByValString));
            return asString(baseValue)->getIndex(exec, i);
        }
        return baseValue.get(exec, i);
    }

    if (isName(subscript))
        return baseValue.get(exec, jsCast<NameInstance*>(subscript.asCell())->privateName());

    Identifier property(exec, subscript.toString(exec)->value(exec));
    return baseValue.get(exec, property);
}

extern "C" {
    
EncodedTiValue JIT_OPERATION operationGetByValGeneric(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedSubscript)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue subscript = TiValue::decode(encodedSubscript);

    TiValue result = getByVal(exec, baseValue, subscript, ReturnAddressPtr(OUR_RETURN_ADDRESS));
    return TiValue::encode(result);
}

EncodedTiValue JIT_OPERATION operationGetByValDefault(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedSubscript)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue subscript = TiValue::decode(encodedSubscript);
    
    if (baseValue.isObject() && subscript.isInt32()) {
        // See if it's worth optimizing this at all.
        JSObject* object = asObject(baseValue);
        bool didOptimize = false;

        unsigned bytecodeOffset = exec->locationAsBytecodeOffset();
        ASSERT(bytecodeOffset);
        ByValInfo& byValInfo = exec->codeBlock()->getByValInfo(bytecodeOffset - 1);
        ASSERT(!byValInfo.stubRoutine);
        
        if (hasOptimizableIndexing(object->structure())) {
            // Attempt to optimize.
            JITArrayMode arrayMode = jitArrayModeForStructure(object->structure());
            if (arrayMode != byValInfo.arrayMode) {
                JIT::compileGetByVal(&vm, exec->codeBlock(), &byValInfo, ReturnAddressPtr(OUR_RETURN_ADDRESS), arrayMode);
                didOptimize = true;
            }
        }
        
        if (!didOptimize) {
            // If we take slow path more than 10 times without patching then make sure we
            // never make that mistake again. Or, if we failed to patch and we have some object
            // that intercepts indexed get, then don't even wait until 10 times. For cases
            // where we see non-index-intercepting objects, this gives 10 iterations worth of
            // opportunity for us to observe that the get_by_val may be polymorphic.
            if (++byValInfo.slowPathCount >= 10
                || object->structure()->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero()) {
                // Don't ever try to optimize.
                RepatchBuffer repatchBuffer(exec->codeBlock());
                repatchBuffer.relinkCallerToFunction(ReturnAddressPtr(OUR_RETURN_ADDRESS), FunctionPtr(operationGetByValGeneric));
            }
        }
    }
    
    TiValue result = getByVal(exec, baseValue, subscript, ReturnAddressPtr(OUR_RETURN_ADDRESS));
    return TiValue::encode(result);
}
    
EncodedTiValue JIT_OPERATION operationGetByValString(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedSubscript)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue subscript = TiValue::decode(encodedSubscript);
    
    TiValue result;
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (isJSString(baseValue) && asString(baseValue)->canGetIndex(i))
            result = asString(baseValue)->getIndex(exec, i);
        else {
            result = baseValue.get(exec, i);
            if (!isJSString(baseValue))
                ctiPatchCallByReturnAddress(exec->codeBlock(), ReturnAddressPtr(OUR_RETURN_ADDRESS), FunctionPtr(operationGetByValDefault));
        }
    } else if (isName(subscript))
        result = baseValue.get(exec, jsCast<NameInstance*>(subscript.asCell())->privateName());
    else {
        Identifier property(exec, subscript.toString(exec)->value(exec));
        result = baseValue.get(exec, property);
    }

    return TiValue::encode(result);
}
    
void JIT_OPERATION operationTearOffActivation(ExecState* exec, JSCell* activationCell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    ASSERT(exec->codeBlock()->needsFullScopeChain());
    jsCast<JSActivation*>(activationCell)->tearOff(vm);
}

void JIT_OPERATION operationTearOffArguments(ExecState* exec, JSCell* argumentsCell, JSCell* activationCell)
{
    ASSERT(exec->codeBlock()->usesArguments());
    if (activationCell) {
        jsCast<Arguments*>(argumentsCell)->didTearOffActivation(exec, jsCast<JSActivation*>(activationCell));
        return;
    }
    jsCast<Arguments*>(argumentsCell)->tearOff(exec);
}

EncodedTiValue JIT_OPERATION operationDeleteById(ExecState* exec, EncodedTiValue encodedBase, const Identifier* identifier)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    JSObject* baseObj = TiValue::decode(encodedBase).toObject(exec);
    bool couldDelete = baseObj->methodTable()->deleteProperty(baseObj, exec, *identifier);
    TiValue result = jsBoolean(couldDelete);
    if (!couldDelete && exec->codeBlock()->isStrictMode())
        vm.throwException(exec, createTypeError(exec, "Unable to delete property."));
    return TiValue::encode(result);
}

JSCell* JIT_OPERATION operationGetPNames(ExecState* exec, JSObject* obj)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    Structure* structure = obj->structure();
    JSPropertyNameIterator* jsPropertyNameIterator = structure->enumerationCache();
    if (!jsPropertyNameIterator || jsPropertyNameIterator->cachedPrototypeChain() != structure->prototypeChain(exec))
        jsPropertyNameIterator = JSPropertyNameIterator::create(exec, obj);
    return jsPropertyNameIterator;
}

EncodedTiValue JIT_OPERATION operationInstanceOf(ExecState* exec, EncodedTiValue encodedValue, EncodedTiValue encodedProto)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue value = TiValue::decode(encodedValue);
    TiValue proto = TiValue::decode(encodedProto);
    
    ASSERT(!value.isObject() || !proto.isObject());

    bool result = JSObject::defaultHasInstance(exec, value, proto);
    return TiValue::encode(jsBoolean(result));
}

CallFrame* JIT_OPERATION operationSizeAndAllocFrameForVarargs(ExecState* exec, EncodedTiValue encodedArguments, int32_t firstFreeRegister)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    JSStack* stack = &exec->interpreter()->stack();
    TiValue arguments = TiValue::decode(encodedArguments);
    CallFrame* newCallFrame = sizeAndAllocFrameForVarargs(exec, stack, arguments, firstFreeRegister);
    return newCallFrame;
}

CallFrame* JIT_OPERATION operationLoadVarargs(ExecState* exec, CallFrame* newCallFrame, EncodedTiValue encodedThis, EncodedTiValue encodedArguments)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue thisValue = TiValue::decode(encodedThis);
    TiValue arguments = TiValue::decode(encodedArguments);
    loadVarargs(exec, newCallFrame, thisValue, arguments);
    return newCallFrame;
}

EncodedTiValue JIT_OPERATION operationToObject(ExecState* exec, EncodedTiValue value)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    return TiValue::encode(TiValue::decode(value).toObject(exec));
}

char* JIT_OPERATION operationSwitchCharWithUnknownKeyType(ExecState* exec, EncodedTiValue encodedKey, size_t tableIndex)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue key = TiValue::decode(encodedKey);
    CodeBlock* codeBlock = exec->codeBlock();

    SimpleJumpTable& jumpTable = codeBlock->switchJumpTable(tableIndex);
    void* result = jumpTable.ctiDefault.executableAddress();

    if (key.isString()) {
        StringImpl* value = asString(key)->value(exec).impl();
        if (value->length() == 1)
            result = jumpTable.ctiForValue((*value)[0]).executableAddress();
    }

    return reinterpret_cast<char*>(result);
}

char* JIT_OPERATION operationSwitchImmWithUnknownKeyType(ExecState* exec, EncodedTiValue encodedKey, size_t tableIndex)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue key = TiValue::decode(encodedKey);
    CodeBlock* codeBlock = exec->codeBlock();

    SimpleJumpTable& jumpTable = codeBlock->switchJumpTable(tableIndex);
    void* result;
    if (key.isInt32())
        result = jumpTable.ctiForValue(key.asInt32()).executableAddress();
    else if (key.isDouble() && key.asDouble() == static_cast<int32_t>(key.asDouble()))
        result = jumpTable.ctiForValue(static_cast<int32_t>(key.asDouble())).executableAddress();
    else
        result = jumpTable.ctiDefault.executableAddress();
    return reinterpret_cast<char*>(result);
}

char* JIT_OPERATION operationSwitchStringWithUnknownKeyType(ExecState* exec, EncodedTiValue encodedKey, size_t tableIndex)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    TiValue key = TiValue::decode(encodedKey);
    CodeBlock* codeBlock = exec->codeBlock();

    void* result;
    StringJumpTable& jumpTable = codeBlock->stringSwitchJumpTable(tableIndex);

    if (key.isString()) {
        StringImpl* value = asString(key)->value(exec).impl();
        result = jumpTable.ctiForValue(value).executableAddress();
    } else
        result = jumpTable.ctiDefault.executableAddress();

    return reinterpret_cast<char*>(result);
}

EncodedTiValue JIT_OPERATION operationResolveScope(ExecState* exec, int32_t identifierIndex)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    const Identifier& ident = exec->codeBlock()->identifier(identifierIndex);
    return TiValue::encode(JSScope::resolve(exec, exec->scope(), ident));
}

EncodedTiValue JIT_OPERATION operationGetFromScope(ExecState* exec, Instruction* bytecodePC)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    CodeBlock* codeBlock = exec->codeBlock();
    Instruction* pc = bytecodePC;

    const Identifier& ident = codeBlock->identifier(pc[3].u.operand);
    JSObject* scope = jsCast<JSObject*>(exec->uncheckedR(pc[2].u.operand).jsValue());
    ResolveModeAndType modeAndType(pc[4].u.operand);

    PropertySlot slot(scope);
    if (!scope->getPropertySlot(exec, ident, slot)) {
        if (modeAndType.mode() == ThrowIfNotFound)
            vm.throwException(exec, createUndefinedVariableError(exec, ident));
        return TiValue::encode(jsUndefined());
    }

    // Covers implicit globals. Since they don't exist until they first execute, we didn't know how to cache them at compile time.
    if (slot.isCacheableValue() && slot.slotBase() == scope && scope->structure()->propertyAccessesAreCacheable()) {
        if (modeAndType.type() == GlobalProperty || modeAndType.type() == GlobalPropertyWithVarInjectionChecks) {
            ConcurrentJITLocker locker(codeBlock->m_lock);
            pc[5].u.structure.set(exec->vm(), codeBlock->ownerExecutable(), scope->structure());
            pc[6].u.operand = slot.cachedOffset();
        }
    }

    return TiValue::encode(slot.getValue(exec, ident));
}

void JIT_OPERATION operationPutToScope(ExecState* exec, Instruction* bytecodePC)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    Instruction* pc = bytecodePC;

    CodeBlock* codeBlock = exec->codeBlock();
    const Identifier& ident = codeBlock->identifier(pc[2].u.operand);
    JSObject* scope = jsCast<JSObject*>(exec->uncheckedR(pc[1].u.operand).jsValue());
    TiValue value = exec->r(pc[3].u.operand).jsValue();
    ResolveModeAndType modeAndType = ResolveModeAndType(pc[4].u.operand);

    if (modeAndType.mode() == ThrowIfNotFound && !scope->hasProperty(exec, ident)) {
        exec->vm().throwException(exec, createUndefinedVariableError(exec, ident));
        return;
    }

    PutPropertySlot slot(scope, codeBlock->isStrictMode());
    scope->methodTable()->put(scope, exec, ident, value, slot);
    
    if (exec->vm().exception())
        return;

    // Covers implicit globals. Since they don't exist until they first execute, we didn't know how to cache them at compile time.
    if (modeAndType.type() == GlobalProperty || modeAndType.type() == GlobalPropertyWithVarInjectionChecks) {
        if (slot.isCacheable() && slot.base() == scope && scope->structure()->propertyAccessesAreCacheable()) {
            ConcurrentJITLocker locker(codeBlock->m_lock);
            pc[5].u.structure.set(exec->vm(), codeBlock->ownerExecutable(), scope->structure());
            pc[6].u.operand = slot.cachedOffset();
        }
    }
}

void JIT_OPERATION operationThrow(ExecState* exec, EncodedTiValue encodedExceptionValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue exceptionValue = TiValue::decode(encodedExceptionValue);
    vm->throwException(exec, exceptionValue);

    // Results stored out-of-band in vm.targetMachinePCForThrow & vm.callFrameForThrow
    genericUnwind(vm, exec, exceptionValue);
}

void JIT_OPERATION operationFlushWriteBarrierBuffer(ExecState* exec, JSCell* cell)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    vm->heap.flushWriteBarrierBuffer(cell);
}

void JIT_OPERATION operationOSRWriteBarrier(ExecState* exec, JSCell* cell)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    exec->heap()->writeBarrier(cell);
}

// NB: We don't include the value as part of the barrier because the write barrier elision
// phase in the DFG only tracks whether the object being stored to has been barriered. It 
// would be much more complicated to try to model the value being stored as well.
void JIT_OPERATION operationUnconditionalWriteBarrier(ExecState* exec, JSCell* cell)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    Heap::writeBarrier(cell);
}

void JIT_OPERATION operationInitGlobalConst(ExecState* exec, Instruction* pc)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue value = exec->r(pc[2].u.operand).jsValue();
    pc[1].u.registerPointer->set(*vm, exec->codeBlock()->globalObject(), value);
}

void JIT_OPERATION lookupExceptionHandler(ExecState* exec)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue exceptionValue = exec->exception();
    ASSERT(exceptionValue);
    
    genericUnwind(vm, exec, exceptionValue);
    ASSERT(vm->targetMachinePCForThrow);
}

void JIT_OPERATION operationVMHandleException(ExecState* exec)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    ASSERT(!exec->isVMEntrySentinel());
    genericUnwind(vm, exec, vm->exception());
}

} // extern "C"

// Note: getHostCallReturnValueWithExecState() needs to be placed before the
// definition of getHostCallReturnValue() below because the Windows build
// requires it.
extern "C" EncodedTiValue HOST_CALL_RETURN_VALUE_OPTION getHostCallReturnValueWithExecState(ExecState* exec)
{
    if (!exec)
        return TiValue::encode(TiValue());
    return TiValue::encode(exec->vm().hostCallReturnValue);
}

#if COMPILER(GCC) && CPU(X86_64)
asm (
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "mov 0(%rbp), %rbp\n" // CallerFrameAndPC::callerFrame
    "mov %rbp, %rdi\n"
    "jmp " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);

#elif COMPILER(GCC) && CPU(X86)
asm (
".text" "\n" \
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "mov 0(%ebp), %ebp\n" // CallerFrameAndPC::callerFrame
    "mov %ebp, 4(%esp)\n"
    "jmp " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);

#elif COMPILER(GCC) && CPU(ARM_THUMB2)
asm (
".text" "\n"
".align 2" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
".thumb" "\n"
".thumb_func " THUMB_FUNC_PARAM(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "ldr r7, [r7, #0]" "\n" // CallerFrameAndPC::callerFrame
    "mov r0, r7" "\n"
    "b " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);

#elif COMPILER(GCC) && CPU(ARM_TRADITIONAL)
asm (
".text" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
INLINE_ARM_FUNCTION(getHostCallReturnValue)
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "ldr r11, [r11, #0]" "\n" // CallerFrameAndPC::callerFrame
    "mov r0, r11" "\n"
    "b " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);

#elif CPU(ARM64)
asm (
".text" "\n"
".align 2" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "ldur x29, [x29, #0]" "\n"
     "mov x0, x29" "\n"
     "b " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);

#elif COMPILER(GCC) && CPU(MIPS)
asm (
".text" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    LOAD_FUNCTION_TO_T9(getHostCallReturnValueWithExecState)
    "lw $fp, 0($fp)" "\n" // CallerFrameAndPC::callerFrame
    "move $a0, $fp" "\n"
    "b " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "\n"
);

#elif COMPILER(GCC) && CPU(SH4)
asm (
".text" "\n"
".globl " SYMBOL_STRING(getHostCallReturnValue) "\n"
HIDE_SYMBOL(getHostCallReturnValue) "\n"
SYMBOL_STRING(getHostCallReturnValue) ":" "\n"
    "mov.l @r14, r14" "\n" // CallerFrameAndPC::callerFrame
    "mov r14, r4" "\n"
    "mov.l 2f, " SH4_SCRATCH_REGISTER "\n"
    "braf " SH4_SCRATCH_REGISTER "\n"
    "nop" "\n"
    "1: .balign 4" "\n"
    "2: .long " LOCAL_REFERENCE(getHostCallReturnValueWithExecState) "-1b\n"
);

#elif COMPILER(MSVC) && CPU(X86)
extern "C" {
    __declspec(naked) EncodedTiValue HOST_CALL_RETURN_VALUE_OPTION getHostCallReturnValue()
    {
        __asm mov ebp, [ebp + 0]; // CallerFrameAndPC::callerFrame
        __asm mov [esp + 4], ebp;
        __asm jmp getHostCallReturnValueWithExecState
    }
}
#endif

} // namespace TI

#endif // ENABLE(JIT)
