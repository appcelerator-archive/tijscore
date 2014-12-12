/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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
#include "DFGOperations.h"

#include "Arguments.h"
#include "ButterflyInlines.h"
#include "CodeBlock.h"
#include "CommonSlowPaths.h"
#include "CopiedSpaceInlines.h"
#include "DFGDriver.h"
#include "DFGOSRExit.h"
#include "DFGThunks.h"
#include "DFGToFTLDeferredCompilationCallback.h"
#include "DFGToFTLForOSREntryDeferredCompilationCallback.h"
#include "DFGWorklist.h"
#include "FTLForOSREntryJITCode.h"
#include "FTLOSREntry.h"
#include "HostCallReturnValue.h"
#include "GetterSetter.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JITExceptions.h"
#include "JITOperationWrappers.h"
#include "JSActivation.h"
#include "VM.h"
#include "JSNameScope.h"
#include "NameInstance.h"
#include "ObjectConstructor.h"
#include "Operations.h"
#include "Repatch.h"
#include "StringConstructor.h"
#include "TypedArrayInlines.h"
#include <wtf/InlineASM.h>

#if ENABLE(JIT)
#if ENABLE(DFG_JIT)

namespace TI { namespace DFG {

template<bool strict, bool direct>
static inline void putByVal(ExecState* exec, TiValue baseValue, uint32_t index, TiValue value)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    if (direct) {
        RELEASE_ASSERT(baseValue.isObject());
        asObject(baseValue)->putDirectIndex(exec, index, value, 0, strict ? PutDirectIndexShouldThrow : PutDirectIndexShouldNotThrow);
        return;
    }
    if (baseValue.isObject()) {
        JSObject* object = asObject(baseValue);
        if (object->canSetIndexQuickly(index)) {
            object->setIndexQuickly(vm, index, value);
            return;
        }

        object->methodTable()->putByIndex(object, exec, index, value, strict);
        return;
    }

    baseValue.putByIndex(exec, index, value, strict);
}

template<bool strict, bool direct>
ALWAYS_INLINE static void JIT_OPERATION operationPutByValInternal(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue property = TiValue::decode(encodedProperty);
    TiValue value = TiValue::decode(encodedValue);

    if (LIKELY(property.isUInt32())) {
        putByVal<strict, direct>(exec, baseValue, property.asUInt32(), value);
        return;
    }

    if (property.isDouble()) {
        double propertyAsDouble = property.asDouble();
        uint32_t propertyAsUInt32 = static_cast<uint32_t>(propertyAsDouble);
        if (propertyAsDouble == propertyAsUInt32) {
            putByVal<strict, direct>(exec, baseValue, propertyAsUInt32, value);
            return;
        }
    }

    if (isName(property)) {
        PutPropertySlot slot(baseValue, strict);
        if (direct) {
            RELEASE_ASSERT(baseValue.isObject());
            asObject(baseValue)->putDirect(*vm, jsCast<NameInstance*>(property.asCell())->privateName(), value, slot);
        } else
            baseValue.put(exec, jsCast<NameInstance*>(property.asCell())->privateName(), value, slot);
        return;
    }

    // Don't put to an object if toString throws an exception.
    Identifier ident(exec, property.toString(exec)->value(exec));
    if (!vm->exception()) {
        PutPropertySlot slot(baseValue, strict);
        if (direct) {
            RELEASE_ASSERT(baseValue.isObject());
            asObject(baseValue)->putDirect(*vm, jsCast<NameInstance*>(property.asCell())->privateName(), value, slot);
        } else
            baseValue.put(exec, ident, value, slot);
    }
}

template<typename ViewClass>
char* newTypedArrayWithSize(ExecState* exec, Structure* structure, int32_t size)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    if (size < 0) {
        vm.throwException(exec, createRangeError(exec, "Requested length is negative"));
        return 0;
    }
    return bitwise_cast<char*>(ViewClass::create(exec, structure, size));
}

template<typename ViewClass>
char* newTypedArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    TiValue value = TiValue::decode(encodedValue);
    
    if (JSArrayBuffer* jsBuffer = jsDynamicCast<JSArrayBuffer*>(value)) {
        RefPtr<ArrayBuffer> buffer = jsBuffer->impl();
        
        if (buffer->byteLength() % ViewClass::elementSize) {
            vm.throwException(exec, createRangeError(exec, "ArrayBuffer length minus the byteOffset is not a multiple of the element size"));
            return 0;
        }
        return bitwise_cast<char*>(
            ViewClass::create(
                exec, structure, buffer, 0, buffer->byteLength() / ViewClass::elementSize));
    }
    
    if (JSObject* object = jsDynamicCast<JSObject*>(value)) {
        unsigned length = object->get(exec, vm.propertyNames->length).toUInt32(exec);
        if (exec->hadException())
            return 0;
        
        ViewClass* result = ViewClass::createUninitialized(exec, structure, length);
        if (!result)
            return 0;
        
        if (!result->set(exec, object, 0, length))
            return 0;
        
        return bitwise_cast<char*>(result);
    }
    
    int length;
    if (value.isInt32())
        length = value.asInt32();
    else if (!value.isNumber()) {
        vm.throwException(exec, createTypeError(exec, "Invalid array length argument"));
        return 0;
    } else {
        length = static_cast<int>(value.asNumber());
        if (length != value.asNumber()) {
            vm.throwException(exec, createTypeError(exec, "Invalid array length argument (fractional lengths not allowed)"));
            return 0;
        }
    }
    
    if (length < 0) {
        vm.throwException(exec, createRangeError(exec, "Requested length is negative"));
        return 0;
    }
    
    return bitwise_cast<char*>(ViewClass::create(exec, structure, length));
}

extern "C" {

EncodedTiValue JIT_OPERATION operationToThis(ExecState* exec, EncodedTiValue encodedOp)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return TiValue::encode(TiValue::decode(encodedOp).toThis(exec, NotStrictMode));
}

EncodedTiValue JIT_OPERATION operationToThisStrict(ExecState* exec, EncodedTiValue encodedOp)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    return TiValue::encode(TiValue::decode(encodedOp).toThis(exec, StrictMode));
}

JSCell* JIT_OPERATION operationCreateThis(ExecState* exec, JSObject* constructor, int32_t inlineCapacity)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

#if !ASSERT_DISABLED
    ConstructData constructData;
    ASSERT(jsCast<JSFunction*>(constructor)->methodTable()->getConstructData(jsCast<JSFunction*>(constructor), constructData) == ConstructTypeJS);
#endif
    
    return constructEmptyObject(exec, jsCast<JSFunction*>(constructor)->allocationProfile(exec, inlineCapacity)->structure());
}

EncodedTiValue JIT_OPERATION operationValueAdd(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue op1 = TiValue::decode(encodedOp1);
    TiValue op2 = TiValue::decode(encodedOp2);
    
    return TiValue::encode(jsAdd(exec, op1, op2));
}

EncodedTiValue JIT_OPERATION operationValueAddNotNumber(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue op1 = TiValue::decode(encodedOp1);
    TiValue op2 = TiValue::decode(encodedOp2);
    
    ASSERT(!op1.isNumber() || !op2.isNumber());
    
    if (op1.isString() && !op2.isObject())
        return TiValue::encode(jsString(exec, asString(op1), op2.toString(exec)));

    return TiValue::encode(jsAddSlowCase(exec, op1, op2));
}

static inline EncodedTiValue getByVal(ExecState* exec, JSCell* base, uint32_t index)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (base->isObject()) {
        JSObject* object = asObject(base);
        if (object->canGetIndexQuickly(index))
            return TiValue::encode(object->getIndexQuickly(index));
    }

    if (isJSString(base) && asString(base)->canGetIndex(index))
        return TiValue::encode(asString(base)->getIndex(exec, index));

    return TiValue::encode(TiValue(base).get(exec, index));
}

EncodedTiValue JIT_OPERATION operationGetByVal(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue property = TiValue::decode(encodedProperty);

    if (LIKELY(baseValue.isCell())) {
        JSCell* base = baseValue.asCell();

        if (property.isUInt32()) {
            return getByVal(exec, base, property.asUInt32());
        } else if (property.isDouble()) {
            double propertyAsDouble = property.asDouble();
            uint32_t propertyAsUInt32 = static_cast<uint32_t>(propertyAsDouble);
            if (propertyAsUInt32 == propertyAsDouble)
                return getByVal(exec, base, propertyAsUInt32);
        } else if (property.isString()) {
            if (TiValue result = base->fastGetOwnProperty(exec, asString(property)->value(exec)))
                return TiValue::encode(result);
        }
    }

    if (isName(property))
        return TiValue::encode(baseValue.get(exec, jsCast<NameInstance*>(property.asCell())->privateName()));

    Identifier ident(exec, property.toString(exec)->value(exec));
    return TiValue::encode(baseValue.get(exec, ident));
}

EncodedTiValue JIT_OPERATION operationGetByValCell(ExecState* exec, JSCell* base, EncodedTiValue encodedProperty)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue property = TiValue::decode(encodedProperty);

    if (property.isUInt32())
        return getByVal(exec, base, property.asUInt32());
    if (property.isDouble()) {
        double propertyAsDouble = property.asDouble();
        uint32_t propertyAsUInt32 = static_cast<uint32_t>(propertyAsDouble);
        if (propertyAsUInt32 == propertyAsDouble)
            return getByVal(exec, base, propertyAsUInt32);
    } else if (property.isString()) {
        if (TiValue result = base->fastGetOwnProperty(exec, asString(property)->value(exec)))
            return TiValue::encode(result);
    }

    if (isName(property))
        return TiValue::encode(TiValue(base).get(exec, jsCast<NameInstance*>(property.asCell())->privateName()));

    Identifier ident(exec, property.toString(exec)->value(exec));
    return TiValue::encode(TiValue(base).get(exec, ident));
}

ALWAYS_INLINE EncodedTiValue getByValCellInt(ExecState* exec, JSCell* base, int32_t index)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    if (index < 0) {
        // Go the slowest way possible becase negative indices don't use indexed storage.
        return TiValue::encode(TiValue(base).get(exec, Identifier::from(exec, index)));
    }

    // Use this since we know that the value is out of bounds.
    return TiValue::encode(TiValue(base).get(exec, index));
}

EncodedTiValue JIT_OPERATION operationGetByValArrayInt(ExecState* exec, JSArray* base, int32_t index)
{
    return getByValCellInt(exec, base, index);
}

EncodedTiValue JIT_OPERATION operationGetByValStringInt(ExecState* exec, JSString* base, int32_t index)
{
    return getByValCellInt(exec, base, index);
}

void JIT_OPERATION operationPutByValStrict(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<true, false>(exec, encodedBase, encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValNonStrict(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<false, false>(exec, encodedBase, encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValCellStrict(ExecState* exec, JSCell* cell, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<true, false>(exec, TiValue::encode(cell), encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValCellNonStrict(ExecState* exec, JSCell* cell, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<false, false>(exec, TiValue::encode(cell), encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValBeyondArrayBoundsStrict(ExecState* exec, JSObject* array, int32_t index, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    if (index >= 0) {
        array->putByIndexInline(exec, index, TiValue::decode(encodedValue), true);
        return;
    }
    
    PutPropertySlot slot(array, true);
    array->methodTable()->put(
        array, exec, Identifier::from(exec, index), TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutByValBeyondArrayBoundsNonStrict(ExecState* exec, JSObject* array, int32_t index, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    if (index >= 0) {
        array->putByIndexInline(exec, index, TiValue::decode(encodedValue), false);
        return;
    }
    
    PutPropertySlot slot(array, false);
    array->methodTable()->put(
        array, exec, Identifier::from(exec, index), TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutDoubleByValBeyondArrayBoundsStrict(ExecState* exec, JSObject* array, int32_t index, double value)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue jsValue = TiValue(TiValue::EncodeAsDouble, value);
    
    if (index >= 0) {
        array->putByIndexInline(exec, index, jsValue, true);
        return;
    }
    
    PutPropertySlot slot(array, true);
    array->methodTable()->put(
        array, exec, Identifier::from(exec, index), jsValue, slot);
}

void JIT_OPERATION operationPutDoubleByValBeyondArrayBoundsNonStrict(ExecState* exec, JSObject* array, int32_t index, double value)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue jsValue = TiValue(TiValue::EncodeAsDouble, value);
    
    if (index >= 0) {
        array->putByIndexInline(exec, index, jsValue, false);
        return;
    }
    
    PutPropertySlot slot(array, false);
    array->methodTable()->put(
        array, exec, Identifier::from(exec, index), jsValue, slot);
}

void JIT_OPERATION operationPutByValDirectStrict(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<true, true>(exec, encodedBase, encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValDirectNonStrict(ExecState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<false, true>(exec, encodedBase, encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValDirectCellStrict(ExecState* exec, JSCell* cell, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<true, true>(exec, TiValue::encode(cell), encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValDirectCellNonStrict(ExecState* exec, JSCell* cell, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    operationPutByValInternal<false, true>(exec, TiValue::encode(cell), encodedProperty, encodedValue);
}

void JIT_OPERATION operationPutByValDirectBeyondArrayBoundsStrict(ExecState* exec, JSObject* array, int32_t index, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    if (index >= 0) {
        array->putDirectIndex(exec, index, TiValue::decode(encodedValue), 0, PutDirectIndexShouldThrow);
        return;
    }
    
    PutPropertySlot slot(array, true);
    array->putDirect(exec->vm(), Identifier::from(exec, index), TiValue::decode(encodedValue), slot);
}

void JIT_OPERATION operationPutByValDirectBeyondArrayBoundsNonStrict(ExecState* exec, JSObject* array, int32_t index, EncodedTiValue encodedValue)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    if (index >= 0) {
        array->putDirectIndex(exec, index, TiValue::decode(encodedValue));
        return;
    }
    
    PutPropertySlot slot(array, false);
    array->putDirect(exec->vm(), Identifier::from(exec, index), TiValue::decode(encodedValue), slot);
}

EncodedTiValue JIT_OPERATION operationArrayPush(ExecState* exec, EncodedTiValue encodedValue, JSArray* array)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    array->push(exec, TiValue::decode(encodedValue));
    return TiValue::encode(jsNumber(array->length()));
}

EncodedTiValue JIT_OPERATION operationArrayPushDouble(ExecState* exec, double value, JSArray* array)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    array->push(exec, TiValue(TiValue::EncodeAsDouble, value));
    return TiValue::encode(jsNumber(array->length()));
}

EncodedTiValue JIT_OPERATION operationArrayPop(ExecState* exec, JSArray* array)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return TiValue::encode(array->pop(exec));
}
        
EncodedTiValue JIT_OPERATION operationArrayPopAndRecoverLength(ExecState* exec, JSArray* array)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    array->butterfly()->setPublicLength(array->butterfly()->publicLength() + 1);
    
    return TiValue::encode(array->pop(exec));
}
        
EncodedTiValue JIT_OPERATION operationRegExpExec(ExecState* exec, JSCell* base, JSCell* argument)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (!base->inherits(RegExpObject::info()))
        return throwVMTypeError(exec);

    ASSERT(argument->isString() || argument->isObject());
    JSString* input = argument->isString() ? asString(argument) : asObject(argument)->toString(exec);
    return TiValue::encode(asRegExpObject(base)->exec(exec, input));
}
        
size_t JIT_OPERATION operationRegExpTest(ExecState* exec, JSCell* base, JSCell* argument)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    if (!base->inherits(RegExpObject::info())) {
        throwTypeError(exec);
        return false;
    }

    ASSERT(argument->isString() || argument->isObject());
    JSString* input = argument->isString() ? asString(argument) : asObject(argument)->toString(exec);
    return asRegExpObject(base)->test(exec, input);
}

size_t JIT_OPERATION operationCompareStrictEqCell(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    TiValue op1 = TiValue::decode(encodedOp1);
    TiValue op2 = TiValue::decode(encodedOp2);
    
    ASSERT(op1.isCell());
    ASSERT(op2.isCell());
    
    return TiValue::strictEqualSlowCaseInline(exec, op1, op2);
}

size_t JIT_OPERATION operationCompareStrictEq(ExecState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    TiValue src1 = TiValue::decode(encodedOp1);
    TiValue src2 = TiValue::decode(encodedOp2);
    
    return TiValue::strictEqual(exec, src1, src2);
}

EncodedTiValue JIT_OPERATION operationToPrimitive(ExecState* exec, EncodedTiValue value)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return TiValue::encode(TiValue::decode(value).toPrimitive(exec));
}

char* JIT_OPERATION operationNewArray(ExecState* exec, Structure* arrayStructure, void* buffer, size_t size)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return bitwise_cast<char*>(constructArray(exec, arrayStructure, static_cast<TiValue*>(buffer), size));
}

char* JIT_OPERATION operationNewEmptyArray(ExecState* exec, Structure* arrayStructure)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    return bitwise_cast<char*>(JSArray::create(*vm, arrayStructure));
}

char* JIT_OPERATION operationNewArrayWithSize(ExecState* exec, Structure* arrayStructure, int32_t size)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);

    if (UNLIKELY(size < 0))
        return bitwise_cast<char*>(exec->vm().throwException(exec, createRangeError(exec, ASCIILiteral("Array size is not a small enough positive integer."))));

    return bitwise_cast<char*>(JSArray::create(*vm, arrayStructure, size));
}

char* JIT_OPERATION operationNewArrayBuffer(ExecState* exec, Structure* arrayStructure, size_t start, size_t size)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    return bitwise_cast<char*>(constructArray(exec, arrayStructure, exec->codeBlock()->constantBuffer(start), size));
}

char* JIT_OPERATION operationNewInt8ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSInt8Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewInt8ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSInt8Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewInt16ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSInt16Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewInt16ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSInt16Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewInt32ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSInt32Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewInt32ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSInt32Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewUint8ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSUint8Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewUint8ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSUint8Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewUint8ClampedArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSUint8ClampedArray>(exec, structure, length);
}

char* JIT_OPERATION operationNewUint8ClampedArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSUint8ClampedArray>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewUint16ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSUint16Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewUint16ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSUint16Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewUint32ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSUint32Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewUint32ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSUint32Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewFloat32ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSFloat32Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewFloat32ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSFloat32Array>(exec, structure, encodedValue);
}

char* JIT_OPERATION operationNewFloat64ArrayWithSize(
    ExecState* exec, Structure* structure, int32_t length)
{
    return newTypedArrayWithSize<JSFloat64Array>(exec, structure, length);
}

char* JIT_OPERATION operationNewFloat64ArrayWithOneArgument(
    ExecState* exec, Structure* structure, EncodedTiValue encodedValue)
{
    return newTypedArrayWithOneArgument<JSFloat64Array>(exec, structure, encodedValue);
}

JSCell* JIT_OPERATION operationCreateInlinedArguments(
    ExecState* exec, InlineCallFrame* inlineCallFrame)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    // NB: This needs to be exceedingly careful with top call frame tracking, since it
    // may be called from OSR exit, while the state of the call stack is bizarre.
    Arguments* result = Arguments::create(vm, exec, inlineCallFrame);
    ASSERT(!vm.exception());
    return result;
}

void JIT_OPERATION operationTearOffInlinedArguments(
    ExecState* exec, JSCell* argumentsCell, JSCell* activationCell, InlineCallFrame* inlineCallFrame)
{
    ASSERT_UNUSED(activationCell, !activationCell); // Currently, we don't inline functions with activations.
    jsCast<Arguments*>(argumentsCell)->tearOff(exec, inlineCallFrame);
}

EncodedTiValue JIT_OPERATION operationGetArgumentByVal(ExecState* exec, int32_t argumentsRegister, int32_t index)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    TiValue argumentsValue = exec->uncheckedR(argumentsRegister).jsValue();
    
    // If there are no arguments, and we're accessing out of bounds, then we have to create the
    // arguments in case someone has installed a getter on a numeric property.
    if (!argumentsValue)
        exec->uncheckedR(argumentsRegister) = argumentsValue = Arguments::create(exec->vm(), exec);
    
    return TiValue::encode(argumentsValue.get(exec, index));
}

EncodedTiValue JIT_OPERATION operationGetInlinedArgumentByVal(
    ExecState* exec, int32_t argumentsRegister, InlineCallFrame* inlineCallFrame, int32_t index)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    TiValue argumentsValue = exec->uncheckedR(argumentsRegister).jsValue();
    
    // If there are no arguments, and we're accessing out of bounds, then we have to create the
    // arguments in case someone has installed a getter on a numeric property.
    if (!argumentsValue) {
        exec->uncheckedR(argumentsRegister) = argumentsValue =
            Arguments::create(exec->vm(), exec, inlineCallFrame);
    }
    
    return TiValue::encode(argumentsValue.get(exec, index));
}

JSCell* JIT_OPERATION operationNewFunctionNoCheck(ExecState* exec, JSCell* functionExecutable)
{
    ASSERT(functionExecutable->inherits(FunctionExecutable::info()));
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    return JSFunction::create(vm, static_cast<FunctionExecutable*>(functionExecutable), exec->scope());
}

size_t JIT_OPERATION operationIsObject(ExecState* exec, EncodedTiValue value)
{
    return jsIsObjectType(exec, TiValue::decode(value));
}

size_t JIT_OPERATION operationIsFunction(EncodedTiValue value)
{
    return jsIsFunctionType(TiValue::decode(value));
}

JSCell* JIT_OPERATION operationTypeOf(ExecState* exec, JSCell* value)
{
    return jsTypeStringForValue(exec, TiValue(value)).asCell();
}

char* JIT_OPERATION operationAllocatePropertyStorageWithInitialCapacity(ExecState* exec)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return reinterpret_cast<char*>(
        Butterfly::createUninitialized(vm, 0, 0, initialOutOfLineCapacity, false, 0));
}

char* JIT_OPERATION operationAllocatePropertyStorage(ExecState* exec, size_t newSize)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return reinterpret_cast<char*>(
        Butterfly::createUninitialized(vm, 0, 0, newSize, false, 0));
}

char* JIT_OPERATION operationReallocateButterflyToHavePropertyStorageWithInitialCapacity(ExecState* exec, JSObject* object)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    ASSERT(!object->structure()->outOfLineCapacity());
    Butterfly* result = object->growOutOfLineStorage(vm, 0, initialOutOfLineCapacity);
    object->setButterflyWithoutChangingStructure(vm, result);
    return reinterpret_cast<char*>(result);
}

char* JIT_OPERATION operationReallocateButterflyToGrowPropertyStorage(ExecState* exec, JSObject* object, size_t newSize)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    Butterfly* result = object->growOutOfLineStorage(vm, object->structure()->outOfLineCapacity(), newSize);
    object->setButterflyWithoutChangingStructure(vm, result);
    return reinterpret_cast<char*>(result);
}

char* JIT_OPERATION operationEnsureInt32(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (!cell->isObject())
        return 0;
    
    return reinterpret_cast<char*>(asObject(cell)->ensureInt32(vm).data());
}

char* JIT_OPERATION operationEnsureDouble(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (!cell->isObject())
        return 0;
    
    return reinterpret_cast<char*>(asObject(cell)->ensureDouble(vm).data());
}

char* JIT_OPERATION operationEnsureContiguous(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (!cell->isObject())
        return 0;
    
    return reinterpret_cast<char*>(asObject(cell)->ensureContiguous(vm).data());
}

char* JIT_OPERATION operationRageEnsureContiguous(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (!cell->isObject())
        return 0;
    
    return reinterpret_cast<char*>(asObject(cell)->rageEnsureContiguous(vm).data());
}

char* JIT_OPERATION operationEnsureArrayStorage(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    if (!cell->isObject())
        return 0;

    return reinterpret_cast<char*>(asObject(cell)->ensureArrayStorage(vm));
}

StringImpl* JIT_OPERATION operationResolveRope(ExecState* exec, JSString* string)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return string->value(exec).impl();
}

JSString* JIT_OPERATION operationSingleCharacterString(ExecState* exec, int32_t character)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    return jsSingleCharacterString(exec, static_cast<UChar>(character));
}

JSCell* JIT_OPERATION operationNewStringObject(ExecState* exec, JSString* string, Structure* structure)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    return StringObject::create(vm, structure, string);
}

JSCell* JIT_OPERATION operationToStringOnCell(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    return TiValue(cell).toString(exec);
}

JSCell* JIT_OPERATION operationToString(ExecState* exec, EncodedTiValue value)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return TiValue::decode(value).toString(exec);
}

JSCell* JIT_OPERATION operationMakeRope2(ExecState* exec, JSString* left, JSString* right)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return JSRopeString::create(vm, left, right);
}

JSCell* JIT_OPERATION operationMakeRope3(ExecState* exec, JSString* a, JSString* b, JSString* c)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return JSRopeString::create(vm, a, b, c);
}

char* JIT_OPERATION operationFindSwitchImmTargetForDouble(
    ExecState* exec, EncodedTiValue encodedValue, size_t tableIndex)
{
    CodeBlock* codeBlock = exec->codeBlock();
    SimpleJumpTable& table = codeBlock->switchJumpTable(tableIndex);
    TiValue value = TiValue::decode(encodedValue);
    ASSERT(value.isDouble());
    double asDouble = value.asDouble();
    int32_t asInt32 = static_cast<int32_t>(asDouble);
    if (asDouble == asInt32)
        return static_cast<char*>(table.ctiForValue(asInt32).executableAddress());
    return static_cast<char*>(table.ctiDefault.executableAddress());
}

char* JIT_OPERATION operationSwitchString(ExecState* exec, size_t tableIndex, JSString* string)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    return static_cast<char*>(exec->codeBlock()->stringSwitchJumpTable(tableIndex).ctiForValue(string->value(exec).impl()).executableAddress());
}

void JIT_OPERATION operationInvalidate(ExecState* exec, VariableWatchpointSet* set)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);

    set->invalidate();
}

double JIT_OPERATION operationFModOnInts(int32_t a, int32_t b)
{
    return fmod(a, b);
}

JSCell* JIT_OPERATION operationStringFromCharCode(ExecState* exec, int32_t op1)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    return TI::stringFromCharCode(exec, op1);
}

size_t JIT_OPERATION dfgConvertTiValueToInt32(ExecState* exec, EncodedTiValue value)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    // toInt32/toUInt32 return the same value; we want the value zero extended to fill the register.
    return TiValue::decode(value).toUInt32(exec);
}

void JIT_OPERATION debugOperationPrintSpeculationFailure(ExecState* exec, void* debugInfoRaw, void* scratch)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    
    SpeculationFailureDebugInfo* debugInfo = static_cast<SpeculationFailureDebugInfo*>(debugInfoRaw);
    CodeBlock* codeBlock = debugInfo->codeBlock;
    CodeBlock* alternative = codeBlock->alternative();
    dataLog(
        "Speculation failure in ", *codeBlock, " with ");
    if (alternative) {
        dataLog(
            "executeCounter = ", alternative->jitExecuteCounter(),
            ", reoptimizationRetryCounter = ", alternative->reoptimizationRetryCounter(),
            ", optimizationDelayCounter = ", alternative->optimizationDelayCounter());
    } else
        dataLog("no alternative code block (i.e. we've been jettisoned)");
    dataLog(", osrExitCounter = ", codeBlock->osrExitCounter(), "\n");
    dataLog("    GPRs at time of exit:");
    char* scratchPointer = static_cast<char*>(scratch);
    for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
        GPRReg gpr = GPRInfo::toRegister(i);
        dataLog(" ", GPRInfo::debugName(gpr), ":", RawPointer(*reinterpret_cast_ptr<void**>(scratchPointer)));
        scratchPointer += sizeof(EncodedTiValue);
    }
    dataLog("\n");
    dataLog("    FPRs at time of exit:");
    for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
        FPRReg fpr = FPRInfo::toRegister(i);
        dataLog(" ", FPRInfo::debugName(fpr), ":");
        uint64_t bits = *reinterpret_cast_ptr<uint64_t*>(scratchPointer);
        double value = *reinterpret_cast_ptr<double*>(scratchPointer);
        dataLogF("%llx:%lf", static_cast<long long>(bits), value);
        scratchPointer += sizeof(EncodedTiValue);
    }
    dataLog("\n");
}

extern "C" void JIT_OPERATION triggerReoptimizationNow(CodeBlock* codeBlock)
{
    // It's sort of preferable that we don't GC while in here. Anyways, doing so wouldn't
    // really be profitable.
    DeferGCForAWhile deferGC(codeBlock->vm()->heap);
    
    if (Options::verboseOSR())
        dataLog(*codeBlock, ": Entered reoptimize\n");
    // We must be called with the baseline code block.
    ASSERT(JITCode::isBaselineCode(codeBlock->jitType()));

    // If I am my own replacement, then reoptimization has already been triggered.
    // This can happen in recursive functions.
    if (codeBlock->replacement() == codeBlock) {
        if (Options::verboseOSR())
            dataLog(*codeBlock, ": Not reoptimizing because we've already been jettisoned.\n");
        return;
    }
    
    // Otherwise, the replacement must be optimized code. Use this as an opportunity
    // to check our logic.
    ASSERT(codeBlock->hasOptimizedReplacement());
    CodeBlock* optimizedCodeBlock = codeBlock->replacement();
    ASSERT(JITCode::isOptimizingJIT(optimizedCodeBlock->jitType()));

    // In order to trigger reoptimization, one of two things must have happened:
    // 1) We exited more than some number of times.
    // 2) We exited and got stuck in a loop, and now we're exiting again.
    bool didExitABunch = optimizedCodeBlock->shouldReoptimizeNow();
    bool didGetStuckInLoop =
        codeBlock->checkIfOptimizationThresholdReached()
        && optimizedCodeBlock->shouldReoptimizeFromLoopNow();
    
    if (!didExitABunch && !didGetStuckInLoop) {
        if (Options::verboseOSR())
            dataLog(*codeBlock, ": Not reoptimizing ", *optimizedCodeBlock, " because it either didn't exit enough or didn't loop enough after exit.\n");
        codeBlock->optimizeAfterLongWarmUp();
        return;
    }

    optimizedCodeBlock->jettison(CountReoptimization);
}

#if ENABLE(FTL_JIT)
void JIT_OPERATION triggerTierUpNow(ExecState* exec)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    DeferGC deferGC(vm->heap);
    CodeBlock* codeBlock = exec->codeBlock();
    
    JITCode* jitCode = codeBlock->jitCode()->dfg();
    
    if (Options::verboseOSR()) {
        dataLog(
            *codeBlock, ": Entered triggerTierUpNow with executeCounter = ",
            jitCode->tierUpCounter, "\n");
    }
    
    if (codeBlock->baselineVersion()->m_didFailFTLCompilation) {
        if (Options::verboseOSR())
            dataLog("Deferring FTL-optimization of ", *codeBlock, " indefinitely because there was an FTL failure.\n");
        jitCode->dontOptimizeAnytimeSoon(codeBlock);
        return;
    }
    
    if (!jitCode->checkIfOptimizationThresholdReached(codeBlock)) {
        if (Options::verboseOSR())
            dataLog("Choosing not to FTL-optimize ", *codeBlock, " yet.\n");
        return;
    }
    
    Worklist::State worklistState;
    if (Worklist* worklist = vm->worklist.get()) {
        worklistState = worklist->completeAllReadyPlansForVM(
            *vm, CompilationKey(codeBlock->baselineVersion(), FTLMode));
    } else
        worklistState = Worklist::NotKnown;
    
    if (worklistState == Worklist::Compiling) {
        jitCode->setOptimizationThresholdBasedOnCompilationResult(
            codeBlock, CompilationDeferred);
        return;
    }
    
    if (codeBlock->hasOptimizedReplacement()) {
        // That's great, we've compiled the code - next time we call this function,
        // we'll enter that replacement.
        jitCode->optimizeSoon(codeBlock);
        return;
    }
    
    if (worklistState == Worklist::Compiled) {
        // This means that we finished compiling, but failed somehow; in that case the
        // thresholds will be set appropriately.
        if (Options::verboseOSR())
            dataLog("Code block ", *codeBlock, " was compiled but it doesn't have an optimized replacement.\n");
        return;
    }

    // We need to compile the code.
    compile(
        *vm, codeBlock->newReplacement().get(), FTLMode, UINT_MAX, Operands<TiValue>(),
        ToFTLDeferredCompilationCallback::create(codeBlock), vm->ensureWorklist());
}

char* JIT_OPERATION triggerOSREntryNow(
    ExecState* exec, int32_t bytecodeIndex, int32_t streamIndex)
{
    VM* vm = &exec->vm();
    NativeCallFrameTracer tracer(vm, exec);
    DeferGC deferGC(vm->heap);
    CodeBlock* codeBlock = exec->codeBlock();
    
    JITCode* jitCode = codeBlock->jitCode()->dfg();
    
    if (Options::verboseOSR()) {
        dataLog(
            *codeBlock, ": Entered triggerTierUpNow with executeCounter = ",
            jitCode->tierUpCounter, "\n");
    }
    
    if (codeBlock->baselineVersion()->m_didFailFTLCompilation) {
        if (Options::verboseOSR())
            dataLog("Deferring FTL-optimization of ", *codeBlock, " indefinitely because there was an FTL failure.\n");
        jitCode->dontOptimizeAnytimeSoon(codeBlock);
        return 0;
    }
    
    if (!jitCode->checkIfOptimizationThresholdReached(codeBlock)) {
        if (Options::verboseOSR())
            dataLog("Choosing not to FTL-optimize ", *codeBlock, " yet.\n");
        return 0;
    }
    
    Worklist::State worklistState;
    if (Worklist* worklist = vm->worklist.get()) {
        worklistState = worklist->completeAllReadyPlansForVM(
            *vm, CompilationKey(codeBlock->baselineVersion(), FTLForOSREntryMode));
    } else
        worklistState = Worklist::NotKnown;
    
    if (worklistState == Worklist::Compiling) {
        ASSERT(!jitCode->osrEntryBlock);
        jitCode->setOptimizationThresholdBasedOnCompilationResult(
            codeBlock, CompilationDeferred);
        return 0;
    }
    
    if (CodeBlock* entryBlock = jitCode->osrEntryBlock.get()) {
        void* address = FTL::prepareOSREntry(
            exec, codeBlock, entryBlock, bytecodeIndex, streamIndex);
        if (address) {
            jitCode->optimizeSoon(codeBlock);
            return static_cast<char*>(address);
        }
        
        FTL::ForOSREntryJITCode* entryCode = entryBlock->jitCode()->ftlForOSREntry();
        entryCode->countEntryFailure();
        if (entryCode->entryFailureCount() <
            Options::ftlOSREntryFailureCountForReoptimization()) {
            
            jitCode->optimizeSoon(codeBlock);
            return 0;
        }
        
        // OSR entry failed. Oh no! This implies that we need to retry. We retry
        // without exponential backoff and we only do this for the entry code block.
        jitCode->osrEntryBlock.clear();
        
        jitCode->optimizeAfterWarmUp(codeBlock);
        return 0;
    }
    
    if (worklistState == Worklist::Compiled) {
        // This means that compilation failed and we already set the thresholds.
        if (Options::verboseOSR())
            dataLog("Code block ", *codeBlock, " was compiled but it doesn't have an optimized replacement.\n");
        return 0;
    }

    // The first order of business is to trigger a for-entry compile.
    Operands<TiValue> mustHandleValues;
    jitCode->reconstruct(
        exec, codeBlock, CodeOrigin(bytecodeIndex), streamIndex, mustHandleValues);
    CompilationResult forEntryResult = DFG::compile(
        *vm, codeBlock->newReplacement().get(), FTLForOSREntryMode, bytecodeIndex,
        mustHandleValues, ToFTLForOSREntryDeferredCompilationCallback::create(codeBlock),
        vm->ensureWorklist());
    
    // But we also want to trigger a replacement compile. Of course, we don't want to
    // trigger it if we don't need to. Note that this is kind of weird because we might
    // have just finished an FTL compile and that compile failed or was invalidated.
    // But this seems uncommon enough that we sort of don't care. It's certainly sound
    // to fire off another compile right now so long as we're not already compiling and
    // we don't already have an optimized replacement. Note, we don't do this for
    // obviously bad cases like global code, where we know that there is a slim chance
    // of this code being invoked ever again.
    CompilationKey keyForReplacement(codeBlock->baselineVersion(), FTLMode);
    if (codeBlock->codeType() != GlobalCode
        && !codeBlock->hasOptimizedReplacement()
        && (!vm->worklist.get()
            || vm->worklist->compilationState(keyForReplacement) == Worklist::NotKnown)) {
        compile(
            *vm, codeBlock->newReplacement().get(), FTLMode, UINT_MAX, Operands<TiValue>(),
            ToFTLDeferredCompilationCallback::create(codeBlock), vm->ensureWorklist());
    }
    
    if (forEntryResult != CompilationSuccessful)
        return 0;
    
    // It's possible that the for-entry compile already succeeded. In that case OSR
    // entry will succeed unless we ran out of stack. It's not clear what we should do.
    // We signal to try again after a while if that happens.
    void* address = FTL::prepareOSREntry(
        exec, codeBlock, jitCode->osrEntryBlock.get(), bytecodeIndex, streamIndex);
    if (address)
        jitCode->optimizeSoon(codeBlock);
    else
        jitCode->optimizeAfterWarmUp(codeBlock);
    return static_cast<char*>(address);
}

// FIXME: Make calls work well. Currently they're a pure regression.
// https://bugs.webkit.org/show_bug.cgi?id=113621
EncodedTiValue JIT_OPERATION operationFTLCall(ExecState* exec)
{
    ExecState* callerExec = exec->callerFrame();
    
    VM* vm = &callerExec->vm();
    NativeCallFrameTracer tracer(vm, callerExec);
    
    TiValue callee = exec->calleeAsValue();
    CallData callData;
    CallType callType = getCallData(callee, callData);
    if (callType == CallTypeNone) {
        vm->throwException(callerExec, createNotAFunctionError(callerExec, callee));
        return TiValue::encode(jsUndefined());
    }
    
    return TiValue::encode(call(callerExec, callee, callType, callData, exec->thisValue(), exec));
}

// FIXME: Make calls work well. Currently they're a pure regression.
// https://bugs.webkit.org/show_bug.cgi?id=113621
EncodedTiValue JIT_OPERATION operationFTLConstruct(ExecState* exec)
{
    ExecState* callerExec = exec->callerFrame();
    
    VM* vm = &callerExec->vm();
    NativeCallFrameTracer tracer(vm, callerExec);
    
    TiValue callee = exec->calleeAsValue();
    ConstructData constructData;
    ConstructType constructType = getConstructData(callee, constructData);
    if (constructType == ConstructTypeNone) {
        vm->throwException(callerExec, createNotAFunctionError(callerExec, callee));
        return TiValue::encode(jsUndefined());
    }
    
    return TiValue::encode(construct(callerExec, callee, constructType, constructData, exec));
}
#endif // ENABLE(FTL_JIT)

} // extern "C"
} } // namespace TI::DFG

#endif // ENABLE(DFG_JIT)

#endif // ENABLE(JIT)
