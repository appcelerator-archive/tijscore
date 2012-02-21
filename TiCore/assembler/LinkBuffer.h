/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef LinkBuffer_h
#define LinkBuffer_h

#if ENABLE(ASSEMBLER)

#define DUMP_LINK_STATISTICS 0
#define DUMP_CODE 0

#include <MacroAssembler.h>
#include <wtf/Noncopyable.h>

namespace TI {

class TiGlobalData;

// LinkBuffer:
//
// This class assists in linking code generated by the macro assembler, once code generation
// has been completed, and the code has been copied to is final location in memory.  At this
// time pointers to labels within the code may be resolved, and relative offsets to external
// addresses may be fixed.
//
// Specifically:
//   * Jump objects may be linked to external targets,
//   * The address of Jump objects may taken, such that it can later be relinked.
//   * The return address of a Call may be acquired.
//   * The address of a Label pointing into the code may be resolved.
//   * The value referenced by a DataLabel may be set.
//
class LinkBuffer {
    WTF_MAKE_NONCOPYABLE(LinkBuffer);
    typedef MacroAssemblerCodeRef CodeRef;
    typedef MacroAssemblerCodePtr CodePtr;
    typedef MacroAssembler::Label Label;
    typedef MacroAssembler::Jump Jump;
    typedef MacroAssembler::JumpList JumpList;
    typedef MacroAssembler::Call Call;
    typedef MacroAssembler::DataLabelCompact DataLabelCompact;
    typedef MacroAssembler::DataLabel32 DataLabel32;
    typedef MacroAssembler::DataLabelPtr DataLabelPtr;
#if ENABLE(BRANCH_COMPACTION)
    typedef MacroAssembler::LinkRecord LinkRecord;
    typedef MacroAssembler::JumpLinkType JumpLinkType;
#endif

public:
    LinkBuffer(TiGlobalData& globalData, MacroAssembler* masm, PassRefPtr<ExecutablePool> executablePool)
        : m_executablePool(executablePool)
        , m_size(0)
        , m_code(0)
        , m_assembler(masm)
        , m_globalData(&globalData)
#ifndef NDEBUG
        , m_completed(false)
#endif
    {
        linkCode();
    }

    LinkBuffer(TiGlobalData& globalData, MacroAssembler* masm, ExecutableAllocator& allocator)
        : m_executablePool(allocator.poolForSize(globalData, masm->m_assembler.codeSize()))
        , m_size(0)
        , m_code(0)
        , m_assembler(masm)
        , m_globalData(&globalData)
#ifndef NDEBUG
        , m_completed(false)
#endif
    {
        linkCode();
    }

    ~LinkBuffer()
    {
        ASSERT(m_completed);
    }

    // These methods are used to link or set values at code generation time.

    void link(Call call, FunctionPtr function)
    {
        ASSERT(call.isFlagSet(Call::Linkable));
        call.m_jmp = applyOffset(call.m_jmp);
        MacroAssembler::linkCall(code(), call, function);
    }
    
    void link(Jump jump, CodeLocationLabel label)
    {
        jump.m_jmp = applyOffset(jump.m_jmp);
        MacroAssembler::linkJump(code(), jump, label);
    }

    void link(JumpList list, CodeLocationLabel label)
    {
        for (unsigned i = 0; i < list.m_jumps.size(); ++i)
            link(list.m_jumps[i], label);
    }

    void patch(DataLabelPtr label, void* value)
    {
        AssemblerLabel target = applyOffset(label.m_label);
        MacroAssembler::linkPointer(code(), target, value);
    }

    void patch(DataLabelPtr label, CodeLocationLabel value)
    {
        AssemblerLabel target = applyOffset(label.m_label);
        MacroAssembler::linkPointer(code(), target, value.executableAddress());
    }

    // These methods are used to obtain handles to allow the code to be relinked / repatched later.

    CodeLocationCall locationOf(Call call)
    {
        ASSERT(call.isFlagSet(Call::Linkable));
        ASSERT(!call.isFlagSet(Call::Near));
        return CodeLocationCall(MacroAssembler::getLinkerAddress(code(), applyOffset(call.m_jmp)));
    }

    CodeLocationNearCall locationOfNearCall(Call call)
    {
        ASSERT(call.isFlagSet(Call::Linkable));
        ASSERT(call.isFlagSet(Call::Near));
        return CodeLocationNearCall(MacroAssembler::getLinkerAddress(code(), applyOffset(call.m_jmp)));
    }

    CodeLocationLabel locationOf(Label label)
    {
        return CodeLocationLabel(MacroAssembler::getLinkerAddress(code(), applyOffset(label.m_label)));
    }

    CodeLocationDataLabelPtr locationOf(DataLabelPtr label)
    {
        return CodeLocationDataLabelPtr(MacroAssembler::getLinkerAddress(code(), applyOffset(label.m_label)));
    }

    CodeLocationDataLabel32 locationOf(DataLabel32 label)
    {
        return CodeLocationDataLabel32(MacroAssembler::getLinkerAddress(code(), applyOffset(label.m_label)));
    }
    
    CodeLocationDataLabelCompact locationOf(DataLabelCompact label)
    {
        return CodeLocationDataLabelCompact(MacroAssembler::getLinkerAddress(code(), applyOffset(label.m_label)));
    }

    // This method obtains the return address of the call, given as an offset from
    // the start of the code.
    unsigned returnAddressOffset(Call call)
    {
        call.m_jmp = applyOffset(call.m_jmp);
        return MacroAssembler::getLinkerCallReturnOffset(call);
    }

    // Upon completion of all patching either 'finalizeCode()' or 'finalizeCodeAddendum()' should be called
    // once to complete generation of the code.  'finalizeCode()' is suited to situations
    // where the executable pool must also be retained, the lighter-weight 'finalizeCodeAddendum()' is
    // suited to adding to an existing allocation.
    CodeRef finalizeCode()
    {
        performFinalization();

        return CodeRef(m_code, m_executablePool, m_size);
    }

    CodeLocationLabel finalizeCodeAddendum()
    {
        performFinalization();

        return CodeLocationLabel(code());
    }

    CodePtr trampolineAt(Label label)
    {
        return CodePtr(MacroAssembler::AssemblerType_T::getRelocatedAddress(code(), applyOffset(label.m_label)));
    }

#ifndef NDEBUG
    void* debugAddress()
    {
        return m_code;
    }
#endif

private:
    template <typename T> T applyOffset(T src)
    {
#if ENABLE(BRANCH_COMPACTION)
        src.m_offset -= m_assembler->executableOffsetFor(src.m_offset);
#endif
        return src;
    }
    
    // Keep this private! - the underlying code should only be obtained externally via 
    // finalizeCode() or finalizeCodeAddendum().
    void* code()
    {
        return m_code;
    }

    void linkCode()
    {
        ASSERT(!m_code);
#if !ENABLE(BRANCH_COMPACTION)
        m_code = m_assembler->m_assembler.executableCopy(*m_globalData, m_executablePool.get());
        m_size = m_assembler->m_assembler.codeSize();
        ASSERT(m_code);
#else
        size_t initialSize = m_assembler->m_assembler.codeSize();
        m_code = (uint8_t*)m_executablePool->alloc(*m_globalData, initialSize);
        if (!m_code)
            return;
        ExecutableAllocator::makeWritable(m_code, initialSize);
        uint8_t* inData = (uint8_t*)m_assembler->unlinkedCode();
        uint8_t* outData = reinterpret_cast<uint8_t*>(m_code);
        int readPtr = 0;
        int writePtr = 0;
        Vector<LinkRecord>& jumpsToLink = m_assembler->jumpsToLink();
        unsigned jumpCount = jumpsToLink.size();
        for (unsigned i = 0; i < jumpCount; ++i) {
            int offset = readPtr - writePtr;
            ASSERT(!(offset & 1));
            
            // Copy the instructions from the last jump to the current one.
            size_t regionSize = jumpsToLink[i].from() - readPtr;
            uint16_t* copySource = reinterpret_cast<uint16_t*>(inData + readPtr);
            uint16_t* copyEnd = reinterpret_cast<uint16_t*>(inData + readPtr + regionSize);
            uint16_t* copyDst = reinterpret_cast<uint16_t*>(outData + writePtr);
            ASSERT(!(regionSize % 2));
            ASSERT(!(readPtr % 2));
            ASSERT(!(writePtr % 2));
            while (copySource != copyEnd)
                *copyDst++ = *copySource++;
            m_assembler->recordLinkOffsets(readPtr, jumpsToLink[i].from(), offset);
            readPtr += regionSize;
            writePtr += regionSize;
            
            // Calculate absolute address of the jump target, in the case of backwards
            // branches we need to be precise, forward branches we are pessimistic
            const uint8_t* target;
            if (jumpsToLink[i].to() >= jumpsToLink[i].from())
                target = outData + jumpsToLink[i].to() - offset; // Compensate for what we have collapsed so far
            else
                target = outData + jumpsToLink[i].to() - m_assembler->executableOffsetFor(jumpsToLink[i].to());
            
            JumpLinkType jumpLinkType = m_assembler->computeJumpType(jumpsToLink[i], outData + writePtr, target);
            // Compact branch if we can...
            if (m_assembler->canCompact(jumpsToLink[i].type())) {
                // Step back in the write stream
                int32_t delta = m_assembler->jumpSizeDelta(jumpsToLink[i].type(), jumpLinkType);
                if (delta) {
                    writePtr -= delta;
                    m_assembler->recordLinkOffsets(jumpsToLink[i].from() - delta, readPtr, readPtr - writePtr);
                }
            }
            jumpsToLink[i].setFrom(writePtr);
        }
        // Copy everything after the last jump
        memcpy(outData + writePtr, inData + readPtr, initialSize - readPtr);
        m_assembler->recordLinkOffsets(readPtr, initialSize, readPtr - writePtr);
        
        for (unsigned i = 0; i < jumpCount; ++i) {
            uint8_t* location = outData + jumpsToLink[i].from();
            uint8_t* target = outData + jumpsToLink[i].to() - m_assembler->executableOffsetFor(jumpsToLink[i].to());
            m_assembler->link(jumpsToLink[i], location, target);
        }

        jumpsToLink.clear();
        m_size = writePtr + initialSize - readPtr;
        m_executablePool->tryShrink(m_code, initialSize, m_size);

#if DUMP_LINK_STATISTICS
        dumpLinkStatistics(m_code, initialSize, m_size);
#endif
#if DUMP_CODE
        dumpCode(m_code, m_size);
#endif
#endif
    }

    void performFinalization()
    {
#ifndef NDEBUG
        ASSERT(!m_completed);
        m_completed = true;
#endif

        ExecutableAllocator::makeExecutable(code(), m_size);
        ExecutableAllocator::cacheFlush(code(), m_size);
    }

#if DUMP_LINK_STATISTICS
    static void dumpLinkStatistics(void* code, size_t initialSize, size_t finalSize)
    {
        static unsigned linkCount = 0;
        static unsigned totalInitialSize = 0;
        static unsigned totalFinalSize = 0;
        linkCount++;
        totalInitialSize += initialSize;
        totalFinalSize += finalSize;
        printf("link %p: orig %u, compact %u (delta %u, %.2f%%)\n", 
               code, static_cast<unsigned>(initialSize), static_cast<unsigned>(finalSize),
               static_cast<unsigned>(initialSize - finalSize),
               100.0 * (initialSize - finalSize) / initialSize);
        printf("\ttotal %u: orig %u, compact %u (delta %u, %.2f%%)\n", 
               linkCount, totalInitialSize, totalFinalSize, totalInitialSize - totalFinalSize,
               100.0 * (totalInitialSize - totalFinalSize) / totalInitialSize);
    }
#endif
    
#if DUMP_CODE
    static void dumpCode(void* code, size_t size)
    {
#if CPU(ARM_THUMB2)
        // Dump the generated code in an asm file format that can be assembled and then disassembled
        // for debugging purposes. For example, save this output as jit.s:
        //   gcc -arch armv7 -c jit.s
        //   otool -tv jit.o
        static unsigned codeCount = 0;
        unsigned short* tcode = static_cast<unsigned short*>(code);
        size_t tsize = size / sizeof(short);
        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "_jsc_jit%u", codeCount++);
        printf("\t.syntax unified\n"
               "\t.section\t__TEXT,__text,regular,pure_instructions\n"
               "\t.globl\t%s\n"
               "\t.align 2\n"
               "\t.code 16\n"
               "\t.thumb_func\t%s\n"
               "# %p\n"
               "%s:\n", nameBuf, nameBuf, code, nameBuf);
        
        for (unsigned i = 0; i < tsize; i++)
            printf("\t.short\t0x%x\n", tcode[i]);
#endif
    }
#endif
    
    RefPtr<ExecutablePool> m_executablePool;
    size_t m_size;
    void* m_code;
    MacroAssembler* m_assembler;
    TiGlobalData* m_globalData;
#ifndef NDEBUG
    bool m_completed;
#endif
};

} // namespace TI

#endif // ENABLE(ASSEMBLER)

#endif // LinkBuffer_h
