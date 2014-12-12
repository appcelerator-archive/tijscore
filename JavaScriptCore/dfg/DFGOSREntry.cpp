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
#include "DFGOSREntry.h"

#if ENABLE(DFG_JIT)

#include "CallFrame.h"
#include "CodeBlock.h"
#include "DFGJITCode.h"
#include "DFGNode.h"
#include "JIT.h"
#include "JSStackInlines.h"
#include "Operations.h"

namespace TI { namespace DFG {

void* prepareOSREntry(ExecState* exec, CodeBlock* codeBlock, unsigned bytecodeIndex)
{
    ASSERT(JITCode::isOptimizingJIT(codeBlock->jitType()));
    ASSERT(codeBlock->alternative());
    ASSERT(codeBlock->alternative()->jitType() == JITCode::BaselineJIT);
    ASSERT(!codeBlock->jitCodeMap());
    
    if (Options::verboseOSR()) {
        dataLog(
            "DFG OSR in ", *codeBlock->alternative(), " -> ", *codeBlock,
            " from bc#", bytecodeIndex, "\n");
    }
    
    VM* vm = &exec->vm();
    
    vm->interpreter->stack().sanitizeStack();
    
    if (codeBlock->jitType() != JITCode::DFGJIT) {
        RELEASE_ASSERT(codeBlock->jitType() == JITCode::FTLJIT);
        
        // When will this happen? We could have:
        //
        // - An exit from the FTL JIT into the baseline JIT followed by an attempt
        //   to reenter. We're fine with allowing this to fail. If it happens
        //   enough we'll just reoptimize. It basically means that the OSR exit cost
        //   us dearly and so reoptimizing is the right thing to do.
        //
        // - We have recursive code with hot loops. Consider that foo has a hot loop
        //   that calls itself. We have two foo's on the stack, lets call them foo1
        //   and foo2, with foo1 having called foo2 from foo's hot loop. foo2 gets
        //   optimized all the way into the FTL. Then it returns into foo1, and then
        //   foo1 wants to get optimized. It might reach this conclusion from its
        //   hot loop and attempt to OSR enter. And we'll tell it that it can't. It
        //   might be worth addressing this case, but I just think this case will
        //   be super rare. For now, if it does happen, it'll cause some compilation
        //   thrashing.
        
        if (Options::verboseOSR())
            dataLog("    OSR failed because the target code block is not DFG.\n");
        return 0;
    }
    
    JITCode* jitCode = codeBlock->jitCode()->dfg();
    OSREntryData* entry = jitCode->osrEntryDataForBytecodeIndex(bytecodeIndex);
    
    if (!entry) {
        if (Options::verboseOSR())
            dataLogF("    OSR failed because the entrypoint was optimized out.\n");
        return 0;
    }
    
    ASSERT(entry->m_bytecodeIndex == bytecodeIndex);
    
    // The code below checks if it is safe to perform OSR entry. It may find
    // that it is unsafe to do so, for any number of reasons, which are documented
    // below. If the code decides not to OSR then it returns 0, and it's the caller's
    // responsibility to patch up the state in such a way as to ensure that it's
    // both safe and efficient to continue executing baseline code for now. This
    // should almost certainly include calling either codeBlock->optimizeAfterWarmUp()
    // or codeBlock->dontOptimizeAnytimeSoon().
    
    // 1) Verify predictions. If the predictions are inconsistent with the actual
    //    values, then OSR entry is not possible at this time. It's tempting to
    //    assume that we could somehow avoid this case. We can certainly avoid it
    //    for first-time loop OSR - that is, OSR into a CodeBlock that we have just
    //    compiled. Then we are almost guaranteed that all of the predictions will
    //    check out. It would be pretty easy to make that a hard guarantee. But
    //    then there would still be the case where two call frames with the same
    //    baseline CodeBlock are on the stack at the same time. The top one
    //    triggers compilation and OSR. In that case, we may no longer have
    //    accurate value profiles for the one deeper in the stack. Hence, when we
    //    pop into the CodeBlock that is deeper on the stack, we might OSR and
    //    realize that the predictions are wrong. Probably, in most cases, this is
    //    just an anomaly in the sense that the older CodeBlock simply went off
    //    into a less-likely path. So, the wisest course of action is to simply not
    //    OSR at this time.
    
    for (size_t argument = 0; argument < entry->m_expectedValues.numberOfArguments(); ++argument) {
        if (argument >= exec->argumentCountIncludingThis()) {
            if (Options::verboseOSR()) {
                dataLogF("    OSR failed because argument %zu was not passed, expected ", argument);
                entry->m_expectedValues.argument(argument).dump(WTI::dataFile());
                dataLogF(".\n");
            }
            return 0;
        }
        
        TiValue value;
        if (!argument)
            value = exec->hostThisValue();
        else
            value = exec->argument(argument - 1);
        
        if (!entry->m_expectedValues.argument(argument).validate(value)) {
            if (Options::verboseOSR()) {
                dataLog(
                    "    OSR failed because argument ", argument, " is ", value,
                    ", expected ", entry->m_expectedValues.argument(argument), ".\n");
            }
            return 0;
        }
    }
    
    for (size_t local = 0; local < entry->m_expectedValues.numberOfLocals(); ++local) {
        int localOffset = virtualRegisterForLocal(local).offset();
        if (entry->m_localsForcedDouble.get(local)) {
            if (!exec->registers()[localOffset].jsValue().isNumber()) {
                if (Options::verboseOSR()) {
                    dataLog(
                        "    OSR failed because variable ", localOffset, " is ",
                        exec->registers()[localOffset].jsValue(), ", expected number.\n");
                }
                return 0;
            }
            continue;
        }
        if (entry->m_localsForcedMachineInt.get(local)) {
            if (!exec->registers()[localOffset].jsValue().isMachineInt()) {
                if (Options::verboseOSR()) {
                    dataLog(
                        "    OSR failed because variable ", localOffset, " is ",
                        exec->registers()[localOffset].jsValue(), ", expected ",
                        "machine int.\n");
                }
                return 0;
            }
            continue;
        }
        if (!entry->m_expectedValues.local(local).validate(exec->registers()[localOffset].jsValue())) {
            if (Options::verboseOSR()) {
                dataLog(
                    "    OSR failed because variable ", localOffset, " is ",
                    exec->registers()[localOffset].jsValue(), ", expected ",
                    entry->m_expectedValues.local(local), ".\n");
            }
            return 0;
        }
    }

    // 2) Check the stack height. The DFG JIT may require a taller stack than the
    //    baseline JIT, in some cases. If we can't grow the stack, then don't do
    //    OSR right now. That's the only option we have unless we want basic block
    //    boundaries to start throwing RangeErrors. Although that would be possible,
    //    it seems silly: you'd be diverting the program to error handling when it
    //    would have otherwise just kept running albeit less quickly.
    
    unsigned frameSize = jitCode->common.requiredRegisterCountForExecutionAndExit();
    if (!vm->interpreter->stack().grow(&exec->registers()[virtualRegisterForLocal(frameSize).offset()])) {
        if (Options::verboseOSR())
            dataLogF("    OSR failed because stack growth failed.\n");
        return 0;
    }
    
    if (Options::verboseOSR())
        dataLogF("    OSR should succeed.\n");
    
    // 3) Perform data format conversions.
    for (size_t local = 0; local < entry->m_expectedValues.numberOfLocals(); ++local) {
        if (entry->m_localsForcedDouble.get(local))
            *bitwise_cast<double*>(exec->registers() + virtualRegisterForLocal(local).offset()) = exec->registers()[virtualRegisterForLocal(local).offset()].jsValue().asNumber();
        if (entry->m_localsForcedMachineInt.get(local))
            *bitwise_cast<int64_t*>(exec->registers() + virtualRegisterForLocal(local).offset()) = exec->registers()[virtualRegisterForLocal(local).offset()].jsValue().asMachineInt() << TiValue::int52ShiftAmount;
    }
    
    // 4) Reshuffle those registers that need reshuffling.
    
    Vector<EncodedTiValue> temporaryLocals(entry->m_reshufflings.size());
    EncodedTiValue* registers = bitwise_cast<EncodedTiValue*>(exec->registers());
    for (unsigned i = entry->m_reshufflings.size(); i--;)
        temporaryLocals[i] = registers[entry->m_reshufflings[i].fromOffset];
    for (unsigned i = entry->m_reshufflings.size(); i--;)
        registers[entry->m_reshufflings[i].toOffset] = temporaryLocals[i];
    
    // 5) Clear those parts of the call frame that the DFG ain't using. This helps GC on some
    //    programs by eliminating some stale pointer pathologies.
    
    for (unsigned i = frameSize; i--;) {
        if (entry->m_machineStackUsed.get(i))
            continue;
        registers[virtualRegisterForLocal(i).offset()] = TiValue::encode(TiValue());
    }
    
    // 6) Fix the call frame.
    
    exec->setCodeBlock(codeBlock);
    
    // 7) Find and return the destination machine code address.
    
    void* result = codeBlock->jitCode()->executableAddressAtOffset(entry->m_machineCodeOffset);
    
    if (Options::verboseOSR())
        dataLogF("    OSR returning machine code address %p.\n", result);
    
    return result;
}

} } // namespace TI::DFG

#endif // ENABLE(DFG_JIT)
