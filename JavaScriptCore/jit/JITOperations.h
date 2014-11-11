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

#ifndef JITOperations_h
#define JITOperations_h

#if ENABLE(JIT)

#include "CallFrame.h"
#include "JITExceptions.h"
#include "JSArray.h"
#include "JSCTiValue.h"
#include "MacroAssembler.h"
#include "PutKind.h"
#include "StructureStubInfo.h"
#include "VariableWatchpointSet.h"

namespace TI {

class ArrayAllocationProfile;

#if CALLING_CONVENTION_IS_STDCALL
#define JIT_OPERATION CDECL
#else
#define JIT_OPERATION
#endif

extern "C" {

// These typedefs provide typechecking when generating calls out to helper routines;
// this helps prevent calling a helper routine with the wrong arguments!
/*
    Key:
    A: JSArray*
    Aap: ArrayAllocationProfile*
    C: JSCell*
    Cb: CodeBlock*
    D: double
    E: ExecState*
    F: CallFrame*
    I: StringImpl*
    Icf: InlineCalLFrame*
    Idc: const Identifier*
    J: EncodedTiValue
    Jcp: const TiValue*
    Jsa: JSActivation*
    Jss: JSString*
    O: JSObject*
    P: pointer (char*)
    Pc: Instruction* i.e. bytecode PC
    R: Register
    S: size_t
    Ssi: StructureStubInfo*
    St: Structure*
    V: void
    Vm: VM*
    Vws: VariableWatchpointSet*
    Z: int32_t
*/

typedef CallFrame* JIT_OPERATION (*F_JITOperation_EFJJ)(ExecState*, CallFrame*, EncodedTiValue, EncodedTiValue);
typedef CallFrame* JIT_OPERATION (*F_JITOperation_EJZ)(ExecState*, EncodedTiValue, int32_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_E)(ExecState*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EA)(ExecState*, JSArray*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EAZ)(ExecState*, JSArray*, int32_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EAapJ)(ExecState*, ArrayAllocationProfile*, EncodedTiValue);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EAapJcpZ)(ExecState*, ArrayAllocationProfile*, const TiValue*, int32_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EC)(ExecState*, JSCell*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_ECI)(ExecState*, JSCell*, StringImpl*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_ECJ)(ExecState*, JSCell*, EncodedTiValue);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EDA)(ExecState*, double, JSArray*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EI)(ExecState*, StringImpl*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EJ)(ExecState*, EncodedTiValue);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EJA)(ExecState*, EncodedTiValue, JSArray*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EJIdc)(ExecState*, EncodedTiValue, const Identifier*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EJJ)(ExecState*, EncodedTiValue, EncodedTiValue);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EJssZ)(ExecState*, JSString*, int32_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EJP)(ExecState*, EncodedTiValue, void*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EP)(ExecState*, void*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EPP)(ExecState*, void*, void*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EPS)(ExecState*, void*, size_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EPc)(ExecState*, Instruction*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_ESS)(ExecState*, size_t, size_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_ESsiCI)(ExecState*, StructureStubInfo*, JSCell*, StringImpl*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_ESsiJI)(ExecState*, StructureStubInfo*, EncodedTiValue, StringImpl*);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EZ)(ExecState*, int32_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EZIcfZ)(ExecState*, int32_t, InlineCallFrame*, int32_t);
typedef EncodedTiValue JIT_OPERATION (*J_JITOperation_EZZ)(ExecState*, int32_t, int32_t);
typedef JSCell* JIT_OPERATION (*C_JITOperation_E)(ExecState*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EZ)(ExecState*, int32_t);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EC)(ExecState*, JSCell*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EIcf)(ExecState*, InlineCallFrame*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EJ)(ExecState*, EncodedTiValue);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EJssSt)(ExecState*, JSString*, Structure*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EJssJss)(ExecState*, JSString*, JSString*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EJssJssJss)(ExecState*, JSString*, JSString*, JSString*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EO)(ExecState*, JSObject*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EOZ)(ExecState*, JSObject*, int32_t);
typedef JSCell* JIT_OPERATION (*C_JITOperation_ESt)(ExecState*, Structure*);
typedef JSCell* JIT_OPERATION (*C_JITOperation_EZ)(ExecState*, int32_t);
typedef double JIT_OPERATION (*D_JITOperation_D)(double);
typedef double JIT_OPERATION (*D_JITOperation_DD)(double, double);
typedef double JIT_OPERATION (*D_JITOperation_ZZ)(int32_t, int32_t);
typedef double JIT_OPERATION (*D_JITOperation_EJ)(ExecState*, EncodedTiValue);
typedef int32_t JIT_OPERATION (*Z_JITOperation_D)(double);
typedef int32_t JIT_OPERATION (*Z_JITOperation_E)(ExecState*);
typedef size_t JIT_OPERATION (*S_JITOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef size_t JIT_OPERATION (*S_JITOperation_EJ)(ExecState*, EncodedTiValue);
typedef size_t JIT_OPERATION (*S_JITOperation_EJJ)(ExecState*, EncodedTiValue, EncodedTiValue);
typedef size_t JIT_OPERATION (*S_JITOperation_EOJss)(ExecState*, JSObject*, JSString*);
typedef size_t JIT_OPERATION (*S_JITOperation_J)(EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_E)(ExecState*);
typedef void JIT_OPERATION (*V_JITOperation_EC)(ExecState*, JSCell*);
typedef void JIT_OPERATION (*V_JITOperation_ECb)(ExecState*, CodeBlock*);
typedef void JIT_OPERATION (*V_JITOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef void JIT_OPERATION (*V_JITOperation_ECIcf)(ExecState*, JSCell*, InlineCallFrame*);
typedef void JIT_OPERATION (*V_JITOperation_ECICC)(ExecState*, JSCell*, Identifier*, JSCell*, JSCell*);
typedef void JIT_OPERATION (*V_JITOperation_ECCIcf)(ExecState*, JSCell*, JSCell*, InlineCallFrame*);
typedef void JIT_OPERATION (*V_JITOperation_ECJJ)(ExecState*, JSCell*, EncodedTiValue, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_ECPSPS)(ExecState*, JSCell*, void*, size_t, void*, size_t);
typedef void JIT_OPERATION (*V_JITOperation_ECZ)(ExecState*, JSCell*, int32_t);
typedef void JIT_OPERATION (*V_JITOperation_ECC)(ExecState*, JSCell*, JSCell*);
typedef void JIT_OPERATION (*V_JITOperation_EIdJZ)(ExecState*, Identifier*, EncodedTiValue, int32_t);
typedef void JIT_OPERATION (*V_JITOperation_EJ)(ExecState*, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_EJCI)(ExecState*, EncodedTiValue, JSCell*, StringImpl*);
typedef void JIT_OPERATION (*V_JITOperation_EJIdJJ)(ExecState*, EncodedTiValue, Identifier*, EncodedTiValue, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_EJJJ)(ExecState*, EncodedTiValue, EncodedTiValue, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_EJPP)(ExecState*, EncodedTiValue, void*, void*);
typedef void JIT_OPERATION (*V_JITOperation_EJZJ)(ExecState*, EncodedTiValue, int32_t, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_EJZ)(ExecState*, EncodedTiValue, int32_t);
typedef void JIT_OPERATION (*V_JITOperation_EOZD)(ExecState*, JSObject*, int32_t, double);
typedef void JIT_OPERATION (*V_JITOperation_EOZJ)(ExecState*, JSObject*, int32_t, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_EPc)(ExecState*, Instruction*);
typedef void JIT_OPERATION (*V_JITOperation_EPZJ)(ExecState*, void*, int32_t, EncodedTiValue);
typedef void JIT_OPERATION (*V_JITOperation_ESsiJJI)(ExecState*, StructureStubInfo*, EncodedTiValue, EncodedTiValue, StringImpl*);
typedef void JIT_OPERATION (*V_JITOperation_EVws)(ExecState*, VariableWatchpointSet*);
typedef void JIT_OPERATION (*V_JITOperation_EZ)(ExecState*, int32_t);
typedef void JIT_OPERATION (*V_JITOperation_EVm)(ExecState*, VM*);
typedef char* JIT_OPERATION (*P_JITOperation_E)(ExecState*);
typedef char* JIT_OPERATION (*P_JITOperation_EC)(ExecState*, JSCell*);
typedef char* JIT_OPERATION (*P_JITOperation_EJS)(ExecState*, EncodedTiValue, size_t);
typedef char* JIT_OPERATION (*P_JITOperation_EO)(ExecState*, JSObject*);
typedef char* JIT_OPERATION (*P_JITOperation_EOS)(ExecState*, JSObject*, size_t);
typedef char* JIT_OPERATION (*P_JITOperation_EOZ)(ExecState*, JSObject*, int32_t);
typedef char* JIT_OPERATION (*P_JITOperation_EPS)(ExecState*, void*, size_t);
typedef char* JIT_OPERATION (*P_JITOperation_ES)(ExecState*, size_t);
typedef char* JIT_OPERATION (*P_JITOperation_ESJss)(ExecState*, size_t, JSString*);
typedef char* JIT_OPERATION (*P_JITOperation_ESt)(ExecState*, Structure*);
typedef char* JIT_OPERATION (*P_JITOperation_EStJ)(ExecState*, Structure*, EncodedTiValue);
typedef char* JIT_OPERATION (*P_JITOperation_EStPS)(ExecState*, Structure*, void*, size_t);
typedef char* JIT_OPERATION (*P_JITOperation_EStSS)(ExecState*, Structure*, size_t, size_t);
typedef char* JIT_OPERATION (*P_JITOperation_EStZ)(ExecState*, Structure*, int32_t);
typedef char* JIT_OPERATION (*P_JITOperation_EZ)(ExecState*, int32_t);
typedef char* JIT_OPERATION (*P_JITOperation_EZZ)(ExecState*, int32_t, int32_t);
typedef StringImpl* JIT_OPERATION (*I_JITOperation_EJss)(ExecState*, JSString*);
typedef JSString* JIT_OPERATION (*Jss_JITOperation_EZ)(ExecState*, int32_t);

// This method is used to lookup an exception hander, keyed by faultLocation, which is
// the return location from one of the calls out to one of the helper operations above.
    
void JIT_OPERATION lookupExceptionHandler(ExecState*) WTF_INTERNAL;
void JIT_OPERATION operationVMHandleException(ExecState*) WTF_INTERNAL;

void JIT_OPERATION operationStackCheck(ExecState*, CodeBlock*) WTF_INTERNAL;
int32_t JIT_OPERATION operationCallArityCheck(ExecState*) WTF_INTERNAL;
int32_t JIT_OPERATION operationConstructArityCheck(ExecState*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetById(ExecState*, StructureStubInfo*, EncodedTiValue, StringImpl*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetByIdBuildList(ExecState*, StructureStubInfo*, EncodedTiValue, StringImpl*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetByIdOptimize(ExecState*, StructureStubInfo*, EncodedTiValue, StringImpl*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationInOptimize(ExecState*, StructureStubInfo*, JSCell*, StringImpl*);
EncodedTiValue JIT_OPERATION operationIn(ExecState*, StructureStubInfo*, JSCell*, StringImpl*);
EncodedTiValue JIT_OPERATION operationGenericIn(ExecState*, JSCell*, EncodedTiValue);
EncodedTiValue JIT_OPERATION operationCallCustomGetter(ExecState*, JSCell*, PropertySlot::GetValueFunc, StringImpl*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationCallGetter(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdStrict(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdNonStrict(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdDirectStrict(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdDirectNonStrict(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdStrictOptimize(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdNonStrictOptimize(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdDirectStrictOptimize(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdDirectNonStrictOptimize(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdStrictBuildList(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdNonStrictBuildList(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdDirectStrictBuildList(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationPutByIdDirectNonStrictBuildList(ExecState*, StructureStubInfo*, EncodedTiValue encodedValue, EncodedTiValue encodedBase, StringImpl*) WTF_INTERNAL;
void JIT_OPERATION operationReallocateStorageAndFinishPut(ExecState*, JSObject*, Structure*, PropertyOffset, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByVal(ExecState*, EncodedTiValue, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationDirectPutByVal(ExecState*, EncodedTiValue, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValGeneric(ExecState*, EncodedTiValue, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationDirectPutByValGeneric(ExecState*, EncodedTiValue, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationCallEval(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION operationVirtualCall(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION operationLinkCall(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION operationLinkClosureCall(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION operationVirtualConstruct(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION operationLinkConstruct(ExecState*) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareLess(ExecState*, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareLessEq(ExecState*, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareGreater(ExecState*, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareGreaterEq(ExecState*, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
size_t JIT_OPERATION operationConvertTiValueToBoolean(ExecState*, EncodedTiValue) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareEq(ExecState*, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
#if USE(JSVALUE64)
EncodedTiValue JIT_OPERATION operationCompareStringEq(ExecState*, JSCell* left, JSCell* right) WTF_INTERNAL;
#else
size_t JIT_OPERATION operationCompareStringEq(ExecState*, JSCell* left, JSCell* right) WTF_INTERNAL;
#endif
size_t JIT_OPERATION operationHasProperty(ExecState*, JSObject*, JSString*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationNewArrayWithProfile(ExecState*, ArrayAllocationProfile*, const TiValue* values, int32_t size) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationNewArrayBufferWithProfile(ExecState*, ArrayAllocationProfile*, const TiValue* values, int32_t size) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationNewArrayWithSizeAndProfile(ExecState*, ArrayAllocationProfile*, EncodedTiValue size) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationNewFunction(ExecState*, JSCell*) WTF_INTERNAL;
JSCell* JIT_OPERATION operationNewObject(ExecState*, Structure*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationNewRegexp(ExecState*, void*) WTF_INTERNAL;
void JIT_OPERATION operationHandleWatchdogTimer(ExecState*) WTF_INTERNAL;
void JIT_OPERATION operationThrowStaticError(ExecState*, EncodedTiValue, int32_t) WTF_INTERNAL;
void JIT_OPERATION operationThrow(ExecState*, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationDebug(ExecState*, int32_t) WTF_INTERNAL;
#if ENABLE(DFG_JIT)
char* JIT_OPERATION operationOptimize(ExecState*, int32_t) WTF_INTERNAL;
#endif
void JIT_OPERATION operationPutByIndex(ExecState*, EncodedTiValue, int32_t, EncodedTiValue);
#if USE(JSVALUE64)
void JIT_OPERATION operationPutGetterSetter(ExecState*, EncodedTiValue, Identifier*, EncodedTiValue, EncodedTiValue) WTF_INTERNAL;
#else
void JIT_OPERATION operationPutGetterSetter(ExecState*, JSCell*, Identifier*, JSCell*, JSCell*) WTF_INTERNAL;
#endif
void JIT_OPERATION operationPushNameScope(ExecState*, Identifier*, EncodedTiValue, int32_t) WTF_INTERNAL;
void JIT_OPERATION operationPushWithScope(ExecState*, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationPopScope(ExecState*) WTF_INTERNAL;
void JIT_OPERATION operationProfileDidCall(ExecState*, EncodedTiValue) WTF_INTERNAL;
void JIT_OPERATION operationProfileWillCall(ExecState*, EncodedTiValue) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationCheckHasInstance(ExecState*, EncodedTiValue, EncodedTiValue baseVal) WTF_INTERNAL;
JSCell* JIT_OPERATION operationCreateActivation(ExecState*, int32_t offset) WTF_INTERNAL;
JSCell* JIT_OPERATION operationCreateArguments(ExecState*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetArgumentsLength(ExecState*, int32_t) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetByValDefault(ExecState*, EncodedTiValue encodedBase, EncodedTiValue encodedSubscript) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetByValGeneric(ExecState*, EncodedTiValue encodedBase, EncodedTiValue encodedSubscript) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetByValString(ExecState*, EncodedTiValue encodedBase, EncodedTiValue encodedSubscript) WTF_INTERNAL;
void JIT_OPERATION operationTearOffActivation(ExecState*, JSCell*) WTF_INTERNAL;
void JIT_OPERATION operationTearOffArguments(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationDeleteById(ExecState*, EncodedTiValue base, const Identifier*) WTF_INTERNAL;
JSCell* JIT_OPERATION operationGetPNames(ExecState*, JSObject*) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationInstanceOf(ExecState*, EncodedTiValue, EncodedTiValue proto) WTF_INTERNAL;
CallFrame* JIT_OPERATION operationSizeAndAllocFrameForVarargs(ExecState*, EncodedTiValue arguments, int32_t firstFreeRegister) WTF_INTERNAL;
CallFrame* JIT_OPERATION operationLoadVarargs(ExecState*, CallFrame*, EncodedTiValue thisValue, EncodedTiValue arguments) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationToObject(ExecState*, EncodedTiValue) WTF_INTERNAL;

char* JIT_OPERATION operationSwitchCharWithUnknownKeyType(ExecState*, EncodedTiValue key, size_t tableIndex) WTF_INTERNAL;
char* JIT_OPERATION operationSwitchImmWithUnknownKeyType(ExecState*, EncodedTiValue key, size_t tableIndex) WTF_INTERNAL;
char* JIT_OPERATION operationSwitchStringWithUnknownKeyType(ExecState*, EncodedTiValue key, size_t tableIndex) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationResolveScope(ExecState*, int32_t identifierIndex) WTF_INTERNAL;
EncodedTiValue JIT_OPERATION operationGetFromScope(ExecState*, Instruction* bytecodePC) WTF_INTERNAL;
void JIT_OPERATION operationPutToScope(ExecState*, Instruction* bytecodePC) WTF_INTERNAL;

void JIT_OPERATION operationFlushWriteBarrierBuffer(ExecState*, JSCell*);
void JIT_OPERATION operationWriteBarrier(ExecState*, JSCell*, JSCell*);
void JIT_OPERATION operationUnconditionalWriteBarrier(ExecState*, JSCell*);
void JIT_OPERATION operationOSRWriteBarrier(ExecState*, JSCell*);

void JIT_OPERATION operationInitGlobalConst(ExecState*, Instruction*);

} // extern "C"

} // namespace TI

#endif // ENABLE(JIT)

#endif // JITOperations_h

