/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SpecializedThunkJIT_h
#define SpecializedThunkJIT_h

#if ENABLE(JIT)

#include "Executable.h"
#include "JSInterfaceJIT.h"
#include "LinkBuffer.h"

namespace TI {

    class SpecializedThunkJIT : public JSInterfaceJIT {
    public:
        static const int ThisArgument = -1;
        SpecializedThunkJIT(int expectedArgCount, TiGlobalData* globalData, ExecutablePool* pool)
            : m_expectedArgCount(expectedArgCount)
            , m_globalData(globalData)
            , m_pool(pool)
        {
            // Check that we have the expected number of arguments
            m_failures.append(branch32(NotEqual, Address(callFrameRegister, RegisterFile::ArgumentCount * (int)sizeof(Register)), Imm32(expectedArgCount + 1)));
        }
        
        void loadDoubleArgument(int argument, FPRegisterID dst, RegisterID scratch)
        {
            unsigned src = argumentToVirtualRegister(argument);
            m_failures.append(emitLoadDouble(src, dst, scratch));
        }
        
        void loadCellArgument(int argument, RegisterID dst)
        {
            unsigned src = argumentToVirtualRegister(argument);
            m_failures.append(emitLoadTiCell(src, dst));
        }
        
        void loadTiStringArgument(int argument, RegisterID dst)
        {
            loadCellArgument(argument, dst);
            m_failures.append(branchPtr(NotEqual, Address(dst, 0), ImmPtr(m_globalData->jsStringVPtr)));
            m_failures.append(branchTest32(NonZero, Address(dst, OBJECT_OFFSETOF(TiString, m_fiberCount))));
        }
        
        void loadInt32Argument(int argument, RegisterID dst, Jump& failTarget)
        {
            unsigned src = argumentToVirtualRegister(argument);
            failTarget = emitLoadInt32(src, dst);
        }
        
        void loadInt32Argument(int argument, RegisterID dst)
        {
            Jump conversionFailed;
            loadInt32Argument(argument, dst, conversionFailed);
            m_failures.append(conversionFailed);
        }
        
        void appendFailure(const Jump& failure)
        {
            m_failures.append(failure);
        }

        void returnTiValue(RegisterID src)
        {
            if (src != regT0)
                move(src, regT0);
            loadPtr(Address(callFrameRegister, RegisterFile::CallerFrame * (int)sizeof(Register)), callFrameRegister);
            ret();
        }
        
        void returnDouble(FPRegisterID src)
        {
#if USE(JSVALUE64)
            moveDoubleToPtr(src, regT0);
            subPtr(tagTypeNumberRegister, regT0);
#elif USE(JSVALUE32_64)
            storeDouble(src, Address(stackPointerRegister, -(int)sizeof(double)));
            loadPtr(Address(stackPointerRegister, OBJECT_OFFSETOF(TiValue, u.asBits.tag) - sizeof(double)), regT1);
            loadPtr(Address(stackPointerRegister, OBJECT_OFFSETOF(TiValue, u.asBits.payload) - sizeof(double)), regT0);
#else
            UNUSED_PARAM(src);
            ASSERT_NOT_REACHED();
            m_failures.append(jump());
#endif
            loadPtr(Address(callFrameRegister, RegisterFile::CallerFrame * (int)sizeof(Register)), callFrameRegister);
            ret();
        }

        void returnInt32(RegisterID src)
        {
            if (src != regT0)
                move(src, regT0);
            tagReturnAsInt32();
            loadPtr(Address(callFrameRegister, RegisterFile::CallerFrame * (int)sizeof(Register)), callFrameRegister);
            ret();
        }

        void returnTiCell(RegisterID src)
        {
            if (src != regT0)
                move(src, regT0);
            tagReturnAsTiCell();
            loadPtr(Address(callFrameRegister, RegisterFile::CallerFrame * (int)sizeof(Register)), callFrameRegister);
            ret();
        }
        
        PassRefPtr<NativeExecutable> finalize()
        {
            LinkBuffer patchBuffer(this, m_pool.get(), 0);
            patchBuffer.link(m_failures, CodeLocationLabel(m_globalData->jitStubs->ctiNativeCallThunk()->generatedJITCode().addressForCall()));
            return adoptRef(new NativeExecutable(patchBuffer.finalizeCode()));
        }
        
    private:
        int argumentToVirtualRegister(unsigned argument)
        {
            return -static_cast<int>(RegisterFile::CallFrameHeaderSize + (m_expectedArgCount - argument));
        }

        void tagReturnAsInt32()
        {
#if USE(JSVALUE64)
            orPtr(tagTypeNumberRegister, regT0);
#elif USE(JSVALUE32_64)
            move(Imm32(TiValue::Int32Tag), regT1);
#else
            signExtend32ToPtr(regT0, regT0);
            // If we can't tag the result, give up and jump to the slow case
            m_failures.append(branchAddPtr(Overflow, regT0, regT0));
            addPtr(Imm32(JSImmediate::TagTypeNumber), regT0);
#endif
        }

        void tagReturnAsTiCell()
        {
#if USE(JSVALUE32_64)
            move(Imm32(TiValue::CellTag), regT1);
#endif
        }
        
        int m_expectedArgCount;
        TiGlobalData* m_globalData;
        RefPtr<ExecutablePool> m_pool;
        MacroAssembler::JumpList m_failures;
    };

}

#endif // ENABLE(JIT)

#endif // SpecializedThunkJIT_h
