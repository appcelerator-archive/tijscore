/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "Interpreter.h"
#include "TiArrayArray.h"
#include "TiGlobalData.h"
#include "Operations.h"

namespace TI { namespace DFG {

EncodedTiValue operationConvertThis(TiExcState* exec, EncodedTiValue encodedOp)
{
    return TiValue::encode(TiValue::decode(encodedOp).toThisObject(exec));
}

EncodedTiValue operationValueAdd(TiExcState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    TiValue op1 = TiValue::decode(encodedOp1);
    TiValue op2 = TiValue::decode(encodedOp2);

    if (op1.isInt32() && op2.isInt32()) {
        int64_t result64 = static_cast<int64_t>(op1.asInt32()) + static_cast<int64_t>(op2.asInt32());
        int32_t result32 = static_cast<int32_t>(result64);
        if (LIKELY(result32 == result64))
            return TiValue::encode(jsNumber(result32));
        return TiValue::encode(jsNumber((double)result64));
    }
    
    double number1;
    double number2;
    if (op1.getNumber(number1) && op2.getNumber(number2))
        return TiValue::encode(jsNumber(number1 + number2));

    return TiValue::encode(jsAddSlowCase(exec, op1, op2));
}

EncodedTiValue operationGetByVal(TiExcState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty)
{
    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue property = TiValue::decode(encodedProperty);

    if (LIKELY(baseValue.isCell())) {
        TiCell* base = baseValue.asCell();

        if (property.isUInt32()) {
            TiGlobalData* globalData = &exec->globalData();
            uint32_t i = property.asUInt32();

            // FIXME: the JIT used to handle these in compiled code!
            if (isTiArray(globalData, base) && asArray(base)->canGetIndex(i))
                return TiValue::encode(asArray(base)->getIndex(i));

            // FIXME: the JITstub used to relink this to an optimized form!
            if (isTiString(globalData, base) && asString(base)->canGetIndex(i))
                return TiValue::encode(asString(base)->getIndex(exec, i));

            // FIXME: the JITstub used to relink this to an optimized form!
            if (isTiArrayArray(globalData, base) && asByteArray(base)->canAccessIndex(i))
                return TiValue::encode(asByteArray(base)->getIndex(exec, i));

            return TiValue::encode(baseValue.get(exec, i));
        }

        if (property.isString()) {
            Identifier propertyName(exec, asString(property)->value(exec));
            PropertySlot slot(base);
            if (base->fastGetOwnPropertySlot(exec, propertyName, slot))
                return TiValue::encode(slot.getValue(exec, propertyName));
        }
    }

    Identifier ident(exec, property.toString(exec));
    return TiValue::encode(baseValue.get(exec, ident));
}

EncodedTiValue operationGetById(TiExcState* exec, EncodedTiValue encodedBase, Identifier* identifier)
{
    TiValue baseValue = TiValue::decode(encodedBase);
    PropertySlot slot(baseValue);
    return TiValue::encode(baseValue.get(exec, *identifier, slot));
}

template<bool strict>
ALWAYS_INLINE static void operationPutByValInternal(TiExcState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    TiGlobalData* globalData = &exec->globalData();

    TiValue baseValue = TiValue::decode(encodedBase);
    TiValue property = TiValue::decode(encodedProperty);
    TiValue value = TiValue::decode(encodedValue);

    if (LIKELY(property.isUInt32())) {
        uint32_t i = property.asUInt32();

        if (isTiArray(globalData, baseValue)) {
            TiArray* jsArray = asArray(baseValue);
            if (jsArray->canSetIndex(i)) {
                jsArray->setIndex(*globalData, i, value);
                return;
            }

            jsArray->TiArray::put(exec, i, value);
            return;
        }

        if (isTiArrayArray(globalData, baseValue) && asByteArray(baseValue)->canAccessIndex(i)) {
            TiArrayArray* jsByteArray = asByteArray(baseValue);
            // FIXME: the JITstub used to relink this to an optimized form!
            if (value.isInt32()) {
                jsByteArray->setIndex(i, value.asInt32());
                return;
            }

            double dValue = 0;
            if (value.getNumber(dValue)) {
                jsByteArray->setIndex(i, dValue);
                return;
            }
        }

        baseValue.put(exec, i, value);
        return;
    }

    // Don't put to an object if toString throws an exception.
    Identifier ident(exec, property.toString(exec));
    if (!globalData->exception) {
        PutPropertySlot slot(strict);
        baseValue.put(exec, ident, value, slot);
    }
}

void operationPutByValStrict(TiExcState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    operationPutByValInternal<true>(exec, encodedBase, encodedProperty, encodedValue);
}

void operationPutByValNonStrict(TiExcState* exec, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue)
{
    operationPutByValInternal<false>(exec, encodedBase, encodedProperty, encodedValue);
}

void operationPutByIdStrict(TiExcState* exec, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(true);
    TiValue::decode(encodedBase).put(exec, *identifier, TiValue::decode(encodedValue), slot);
}

void operationPutByIdNonStrict(TiExcState* exec, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(false);
    TiValue::decode(encodedBase).put(exec, *identifier, TiValue::decode(encodedValue), slot);
}

void operationPutByIdDirectStrict(TiExcState* exec, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(true);
    TiValue::decode(encodedBase).putDirect(exec, *identifier, TiValue::decode(encodedValue), slot);
}

void operationPutByIdDirectNonStrict(TiExcState* exec, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier* identifier)
{
    PutPropertySlot slot(false);
    TiValue::decode(encodedBase).putDirect(exec, *identifier, TiValue::decode(encodedValue), slot);
}

bool operationCompareLess(TiExcState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    return jsLess(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

bool operationCompareLessEq(TiExcState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    return jsLessEq(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

bool operationCompareEq(TiExcState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    return TiValue::equal(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

bool operationCompareStrictEq(TiExcState* exec, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2)
{
    return TiValue::strictEqual(exec, TiValue::decode(encodedOp1), TiValue::decode(encodedOp2));
}

DFGHandler lookupExceptionHandler(TiExcState* exec, ReturnAddressPtr faultLocation)
{
    TiValue exceptionValue = exec->exception();
    ASSERT(exceptionValue);

    unsigned vPCIndex = exec->codeBlock()->bytecodeOffset(faultLocation);
    HandlerInfo* handler = exec->globalData().interpreter->throwException(exec, exceptionValue, vPCIndex);

    void* catchRoutine = handler ? handler->nativeCode.executableAddress() : (void*)ctiOpThrowNotCaught;
    ASSERT(catchRoutine);
    return DFGHandler(exec, catchRoutine);
}

double dfgConvertTiValueToNumber(TiExcState* exec, EncodedTiValue value)
{
    return TiValue::decode(value).toNumber(exec);
}

int32_t dfgConvertTiValueToInt32(TiExcState* exec, EncodedTiValue value)
{
    return TiValue::decode(value).toInt32(exec);
}

bool dfgConvertTiValueToBoolean(TiExcState* exec, EncodedTiValue encodedOp)
{
    return TiValue::decode(encodedOp).toBoolean(exec);
}

} } // namespace TI::DFG

#endif
