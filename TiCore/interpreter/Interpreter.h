/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Interpreter_h
#define Interpreter_h

#include "ArgList.h"
#include "FastAllocBase.h"
#include "TiCell.h"
#include "TiValue.h"
#include "TiObject.h"
#include "Opcode.h"
#include "RegisterFile.h"

#include <wtf/HashMap.h>

namespace TI {

    class CodeBlock;
    class EvalExecutable;
    class FunctionExecutable;
    class InternalFunction;
    class TiFunction;
    class TiGlobalObject;
    class ProgramExecutable;
    class Register;
    class ScopeChainNode;
    class SamplingTool;
    struct CallFrameClosure;
    struct HandlerInfo;
    struct Instruction;
    
    enum DebugHookID {
        WillExecuteProgram,
        DidExecuteProgram,
        DidEnterCallFrame,
        DidReachBreakpoint,
        WillLeaveCallFrame,
        WillExecuteStatement
    };

    enum { MaxMainThreadReentryDepth = 256, MaxSecondaryThreadReentryDepth = 32 };

    class Interpreter : public FastAllocBase {
        friend class JIT;
        friend class CachedCall;
    public:
        Interpreter();

        RegisterFile& registerFile() { return m_registerFile; }
        
        Opcode getOpcode(OpcodeID id)
        {
            #if HAVE(COMPUTED_GOTO)
                return m_opcodeTable[id];
            #else
                return id;
            #endif
        }

        OpcodeID getOpcodeID(Opcode opcode)
        {
            #if HAVE(COMPUTED_GOTO)
                ASSERT(isOpcode(opcode));
                return m_opcodeIDTable.get(opcode);
            #else
                return opcode;
            #endif
        }

        bool isOpcode(Opcode);
        
        TiValue execute(ProgramExecutable*, CallFrame*, ScopeChainNode*, TiObject* thisObj, TiValue* exception);
        TiValue execute(FunctionExecutable*, CallFrame*, TiFunction*, TiObject* thisObj, const ArgList& args, ScopeChainNode*, TiValue* exception);
        TiValue execute(EvalExecutable* evalNode, CallFrame* exec, TiObject* thisObj, ScopeChainNode* scopeChain, TiValue* exception);

        TiValue retrieveArguments(CallFrame*, TiFunction*) const;
        TiValue retrieveCaller(CallFrame*, InternalFunction*) const;
        void retrieveLastCaller(CallFrame*, int& lineNumber, intptr_t& sourceID, UString& sourceURL, TiValue& function) const;
        
        void getArgumentsData(CallFrame*, TiFunction*&, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc);
        
        SamplingTool* sampler() { return m_sampler.get(); }

        NEVER_INLINE TiValue callEval(CallFrame*, RegisterFile*, Register* argv, int argc, int registerOffset, TiValue& exceptionValue);
        NEVER_INLINE HandlerInfo* throwException(CallFrame*&, TiValue&, unsigned bytecodeOffset, bool);
        NEVER_INLINE void debug(CallFrame*, DebugHookID, int firstLine, int lastLine);

        void dumpSampleData(TiExcState* exec);
        void startSampling();
        void stopSampling();
    private:
        enum ExecutionFlag { Normal, InitializeAndReturn };

        CallFrameClosure prepareForRepeatCall(FunctionExecutable*, CallFrame*, TiFunction*, int argCount, ScopeChainNode*, TiValue* exception);
        void endRepeatCall(CallFrameClosure&);
        TiValue execute(CallFrameClosure&, TiValue* exception);

        TiValue execute(EvalExecutable*, CallFrame*, TiObject* thisObject, int globalRegisterOffset, ScopeChainNode*, TiValue* exception);

#if USE(INTERPRETER)
        NEVER_INLINE bool resolve(CallFrame*, Instruction*, TiValue& exceptionValue);
        NEVER_INLINE bool resolveSkip(CallFrame*, Instruction*, TiValue& exceptionValue);
        NEVER_INLINE bool resolveGlobal(CallFrame*, Instruction*, TiValue& exceptionValue);
        NEVER_INLINE void resolveBase(CallFrame*, Instruction* vPC);
        NEVER_INLINE bool resolveBaseAndProperty(CallFrame*, Instruction*, TiValue& exceptionValue);
        NEVER_INLINE ScopeChainNode* createExceptionScope(CallFrame*, const Instruction* vPC);

        void tryCacheGetByID(CallFrame*, CodeBlock*, Instruction*, TiValue baseValue, const Identifier& propertyName, const PropertySlot&);
        void uncacheGetByID(CodeBlock*, Instruction* vPC);
        void tryCachePutByID(CallFrame*, CodeBlock*, Instruction*, TiValue baseValue, const PutPropertySlot&);
        void uncachePutByID(CodeBlock*, Instruction* vPC);        
#endif

        NEVER_INLINE bool unwindCallFrame(CallFrame*&, TiValue, unsigned& bytecodeOffset, CodeBlock*&);

        static ALWAYS_INLINE CallFrame* slideRegisterWindowForCall(CodeBlock*, RegisterFile*, CallFrame*, size_t registerOffset, int argc);

        static CallFrame* findFunctionCallFrame(CallFrame*, InternalFunction*);

        TiValue privateExecute(ExecutionFlag, RegisterFile*, CallFrame*, TiValue* exception);

        void dumpCallFrame(CallFrame*);
        void dumpRegisters(CallFrame*);
        
        bool isCallBytecode(Opcode opcode) { return opcode == getOpcode(op_call) || opcode == getOpcode(op_construct) || opcode == getOpcode(op_call_eval); }

        void enableSampler();
        int m_sampleEntryDepth;
        OwnPtr<SamplingTool> m_sampler;

        int m_reentryDepth;

        RegisterFile m_registerFile;
        
#if HAVE(COMPUTED_GOTO)
        Opcode m_opcodeTable[numOpcodeIDs]; // Maps OpcodeID => Opcode for compiling
        HashMap<Opcode, OpcodeID> m_opcodeIDTable; // Maps Opcode => OpcodeID for decompiling
#endif
    };
    
} // namespace TI

#endif // Interpreter_h
