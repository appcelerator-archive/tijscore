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
#include "AssemblyHelpers.h"

#if ENABLE(JIT)

namespace TI {

ExecutableBase* AssemblyHelpers::executableFor(const CodeOrigin& codeOrigin)
{
    if (!codeOrigin.inlineCallFrame)
        return m_codeBlock->ownerExecutable();
    
    return codeOrigin.inlineCallFrame->executable.get();
}

Vector<BytecodeAndMachineOffset>& AssemblyHelpers::decodedCodeMapFor(CodeBlock* codeBlock)
{
    ASSERT(codeBlock == codeBlock->baselineVersion());
    ASSERT(codeBlock->jitType() == JITCode::BaselineJIT);
    ASSERT(codeBlock->jitCodeMap());
    
    HashMap<CodeBlock*, Vector<BytecodeAndMachineOffset>>::AddResult result = m_decodedCodeMaps.add(codeBlock, Vector<BytecodeAndMachineOffset>());
    
    if (result.isNewEntry)
        codeBlock->jitCodeMap()->decode(result.iterator->value);
    
    return result.iterator->value;
}

#if ENABLE(SAMPLING_FLAGS)
void AssemblyHelpers::setSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    or32(TrustedImm32(1u << (flag - 1)), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}

void AssemblyHelpers::clearSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    and32(TrustedImm32(~(1u << (flag - 1))), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}
#endif

#if !ASSERT_DISABLED
#if USE(JSVALUE64)
void AssemblyHelpers::jitAssertIsInt32(GPRReg gpr)
{
#if CPU(X86_64)
    Jump checkInt32 = branch64(BelowOrEqual, gpr, TrustedImm64(static_cast<uintptr_t>(0xFFFFFFFFu)));
    breakpoint();
    checkInt32.link(this);
#else
    UNUSED_PARAM(gpr);
#endif
}

void AssemblyHelpers::jitAssertIsJSInt32(GPRReg gpr)
{
    Jump checkTIInt32 = branch64(AboveOrEqual, gpr, GPRInfo::tagTypeNumberRegister);
    breakpoint();
    checkTIInt32.link(this);
}

void AssemblyHelpers::jitAssertIsJSNumber(GPRReg gpr)
{
    Jump checkTINumber = branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagTypeNumberRegister);
    breakpoint();
    checkTINumber.link(this);
}

void AssemblyHelpers::jitAssertIsJSDouble(GPRReg gpr)
{
    Jump checkTIInt32 = branch64(AboveOrEqual, gpr, GPRInfo::tagTypeNumberRegister);
    Jump checkTINumber = branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagTypeNumberRegister);
    checkTIInt32.link(this);
    breakpoint();
    checkTINumber.link(this);
}

void AssemblyHelpers::jitAssertIsCell(GPRReg gpr)
{
    Jump checkCell = branchTest64(MacroAssembler::Zero, gpr, GPRInfo::tagMaskRegister);
    breakpoint();
    checkCell.link(this);
}
#elif USE(JSVALUE32_64)
void AssemblyHelpers::jitAssertIsInt32(GPRReg gpr)
{
    UNUSED_PARAM(gpr);
}

void AssemblyHelpers::jitAssertIsJSInt32(GPRReg gpr)
{
    Jump checkTIInt32 = branch32(Equal, gpr, TrustedImm32(TiValue::Int32Tag));
    breakpoint();
    checkTIInt32.link(this);
}

void AssemblyHelpers::jitAssertIsJSNumber(GPRReg gpr)
{
    Jump checkTIInt32 = branch32(Equal, gpr, TrustedImm32(TiValue::Int32Tag));
    Jump checkTIDouble = branch32(Below, gpr, TrustedImm32(TiValue::LowestTag));
    breakpoint();
    checkTIInt32.link(this);
    checkTIDouble.link(this);
}

void AssemblyHelpers::jitAssertIsJSDouble(GPRReg gpr)
{
    Jump checkTIDouble = branch32(Below, gpr, TrustedImm32(TiValue::LowestTag));
    breakpoint();
    checkTIDouble.link(this);
}

void AssemblyHelpers::jitAssertIsCell(GPRReg gpr)
{
    Jump checkCell = branch32(Equal, gpr, TrustedImm32(TiValue::CellTag));
    breakpoint();
    checkCell.link(this);
}
#endif // USE(JSVALUE32_64)

void AssemblyHelpers::jitAssertHasValidCallFrame()
{
    Jump checkCFR = branchTestPtr(Zero, GPRInfo::callFrameRegister, TrustedImm32(7));
    breakpoint();
    checkCFR.link(this);
}

void AssemblyHelpers::jitAssertIsNull(GPRReg gpr)
{
    Jump checkNull = branchTestPtr(Zero, gpr);
    breakpoint();
    checkNull.link(this);
}
#endif // !ASSERT_DISABLED

} // namespace TI

#endif // ENABLE(JIT)

