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
#include "FTLOSREntry.h"

#include "CallFrame.h"
#include "CodeBlock.h"
#include "DFGJITCode.h"
#include "FTLForOSREntryJITCode.h"
#include "JSStackInlines.h"

#if ENABLE(FTL_JIT)

namespace TI { namespace FTL {

void* prepareOSREntry(
    ExecState* exec, CodeBlock* dfgCodeBlock, CodeBlock* entryCodeBlock,
    unsigned bytecodeIndex, unsigned streamIndex)
{
    VM& vm = exec->vm();
    CodeBlock* baseline = dfgCodeBlock->baselineVersion();
    DFG::JITCode* dfgCode = dfgCodeBlock->jitCode()->dfg();
    ForOSREntryJITCode* entryCode = entryCodeBlock->jitCode()->ftlForOSREntry();
    
    if (Options::verboseOSR()) {
        dataLog(
            "FTL OSR from ", *dfgCodeBlock, " to ", *entryCodeBlock, " at bc#",
            bytecodeIndex, ".\n");
    }
    
    if (bytecodeIndex != entryCode->bytecodeIndex()) {
        if (Options::verboseOSR())
            dataLog("    OSR failed because we don't have an entrypoint for bc#", bytecodeIndex, "; ours is for bc#", entryCode->bytecodeIndex());
        return 0;
    }
    
    Operands<TiValue> values;
    dfgCode->reconstruct(
        exec, dfgCodeBlock, CodeOrigin(bytecodeIndex), streamIndex, values);
    
    if (Options::verboseOSR())
        dataLog("    Values at entry: ", values, "\n");
    
    for (int argument = values.numberOfArguments(); argument--;) {
        RELEASE_ASSERT(
            exec->r(virtualRegisterForArgument(argument).offset()).jsValue() == values.argument(argument));
    }
    
    RELEASE_ASSERT(
        static_cast<int>(values.numberOfLocals()) == baseline->m_numCalleeRegisters);
    
    EncodedTiValue* scratch = static_cast<EncodedTiValue*>(
        entryCode->entryBuffer()->dataBuffer());
    
    for (int local = values.numberOfLocals(); local--;)
        scratch[local] = TiValue::encode(values.local(local));
    
    int stackFrameSize = entryCode->common.requiredRegisterCountForExecutionAndExit();
    if (!vm.interpreter->stack().grow(&exec->registers()[virtualRegisterForLocal(stackFrameSize).offset()])) {
        if (Options::verboseOSR())
            dataLog("    OSR failed because stack growth failed.\n");
        return 0;
    }
    
    exec->setCodeBlock(entryCodeBlock);
    
    void* result = entryCode->addressForCall().executableAddress();
    if (Options::verboseOSR())
        dataLog("    Entry will succeed, going to address", RawPointer(result), "\n");
    
    return result;
}

} } // namespace TI::FTL

#endif // ENABLE(FTL_JIT)


