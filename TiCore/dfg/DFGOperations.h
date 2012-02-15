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

#ifndef DFGOperations_h
#define DFGOperations_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGJITCompiler.h>

namespace TI {

class Identifier;

namespace DFG {

// These typedefs provide typechecking when generating calls out to helper routines;
// this helps prevent calling a helper routine with the wrong arguments!
typedef EncodedTiValue (*J_DFGOperation_EJJ)(TiExcState*, EncodedTiValue, EncodedTiValue);
typedef EncodedTiValue (*J_DFGOperation_EJ)(TiExcState*, EncodedTiValue);
typedef EncodedTiValue (*J_DFGOperation_EJP)(TiExcState*, EncodedTiValue, void*);
typedef EncodedTiValue (*J_DFGOperation_EJI)(TiExcState*, EncodedTiValue, Identifier*);
typedef bool (*Z_DFGOperation_EJ)(TiExcState*, EncodedTiValue);
typedef bool (*Z_DFGOperation_EJJ)(TiExcState*, EncodedTiValue, EncodedTiValue);
typedef void (*V_DFGOperation_EJJJ)(TiExcState*, EncodedTiValue, EncodedTiValue, EncodedTiValue);
typedef void (*V_DFGOperation_EJJP)(TiExcState*, EncodedTiValue, EncodedTiValue, void*);
typedef void (*V_DFGOperation_EJJI)(TiExcState*, EncodedTiValue, EncodedTiValue, Identifier*);
typedef double (*D_DFGOperation_DD)(double, double);

// These routines are provide callbacks out to C++ implementations of operations too complex to JIT.
EncodedTiValue operationConvertThis(TiExcState*, EncodedTiValue encodedOp1);
EncodedTiValue operationValueAdd(TiExcState*, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2);
EncodedTiValue operationGetByVal(TiExcState*, EncodedTiValue encodedBase, EncodedTiValue encodedProperty);
EncodedTiValue operationGetById(TiExcState*, EncodedTiValue encodedBase, Identifier*);
void operationPutByValStrict(TiExcState*, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue);
void operationPutByValNonStrict(TiExcState*, EncodedTiValue encodedBase, EncodedTiValue encodedProperty, EncodedTiValue encodedValue);
void operationPutByIdStrict(TiExcState*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier*);
void operationPutByIdNonStrict(TiExcState*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier*);
void operationPutByIdDirectStrict(TiExcState*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier*);
void operationPutByIdDirectNonStrict(TiExcState*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, Identifier*);
bool operationCompareLess(TiExcState*, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2);
bool operationCompareLessEq(TiExcState*, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2);
bool operationCompareEq(TiExcState*, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2);
bool operationCompareStrictEq(TiExcState*, EncodedTiValue encodedOp1, EncodedTiValue encodedOp2);

// This method is used to lookup an exception hander, keyed by faultLocation, which is
// the return location from one of the calls out to one of the helper operations above.
struct DFGHandler {
    DFGHandler(TiExcState* exec, void* handler)
        : exec(exec)
        , handler(handler)
    {
    }

    TiExcState* exec;
    void* handler;
};
DFGHandler lookupExceptionHandler(TiExcState*, ReturnAddressPtr faultLocation);

// These operations implement the implicitly called ToInt32, ToNumber, and ToBoolean conversions from ES5.
double dfgConvertTiValueToNumber(TiExcState*, EncodedTiValue);
int32_t dfgConvertTiValueToInt32(TiExcState*, EncodedTiValue);
bool dfgConvertTiValueToBoolean(TiExcState*, EncodedTiValue);

} } // namespace TI::DFG

#endif
#endif
