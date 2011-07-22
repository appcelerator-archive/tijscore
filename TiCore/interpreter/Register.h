/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef Register_h
#define Register_h

#include "TiValue.h"
#include <wtf/Assertions.h>
#include <wtf/FastAllocBase.h>
#include <wtf/VectorTraits.h>

namespace TI {

    class Arguments;
    class CodeBlock;
    class TiExcState;
    class JSActivation;
    class TiFunction;
    class TiPropertyNameIterator;
    class ScopeChainNode;

    struct Instruction;

    typedef TiExcState CallFrame;

    class Register : public WTI::FastAllocBase {
    public:
        Register();

        Register(const TiValue&);
        Register& operator=(const TiValue&);
        TiValue jsValue() const;
        
        Register& operator=(JSActivation*);
        Register& operator=(CallFrame*);
        Register& operator=(CodeBlock*);
        Register& operator=(TiFunction*);
        Register& operator=(TiPropertyNameIterator*);
        Register& operator=(ScopeChainNode*);
        Register& operator=(Instruction*);

        int32_t i() const;
        JSActivation* activation() const;
        Arguments* arguments() const;
        CallFrame* callFrame() const;
        CodeBlock* codeBlock() const;
        TiFunction* function() const;
        TiPropertyNameIterator* propertyNameIterator() const;
        ScopeChainNode* scopeChain() const;
        Instruction* vPC() const;

        static Register withInt(int32_t i)
        {
            Register r;
            r.u.i = i;
            return r;
        }

    private:
        union {
            int32_t i;
            EncodedTiValue value;

            JSActivation* activation;
            CallFrame* callFrame;
            CodeBlock* codeBlock;
            TiFunction* function;
            TiPropertyNameIterator* propertyNameIterator;
            ScopeChainNode* scopeChain;
            Instruction* vPC;
        } u;
    };

    ALWAYS_INLINE Register::Register()
    {
#ifndef NDEBUG
        *this = TiValue();
#endif
    }

    ALWAYS_INLINE Register::Register(const TiValue& v)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!v.isZombie());
#endif
        u.value = TiValue::encode(v);
    }

    ALWAYS_INLINE Register& Register::operator=(const TiValue& v)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!v.isZombie());
#endif
        u.value = TiValue::encode(v);
        return *this;
    }

    ALWAYS_INLINE TiValue Register::jsValue() const
    {
        return TiValue::decode(u.value);
    }

    // Interpreter functions

    ALWAYS_INLINE Register& Register::operator=(JSActivation* activation)
    {
        u.activation = activation;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(CallFrame* callFrame)
    {
        u.callFrame = callFrame;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(CodeBlock* codeBlock)
    {
        u.codeBlock = codeBlock;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(TiFunction* function)
    {
        u.function = function;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(Instruction* vPC)
    {
        u.vPC = vPC;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(ScopeChainNode* scopeChain)
    {
        u.scopeChain = scopeChain;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(TiPropertyNameIterator* propertyNameIterator)
    {
        u.propertyNameIterator = propertyNameIterator;
        return *this;
    }

    ALWAYS_INLINE int32_t Register::i() const
    {
        return u.i;
    }
    
    ALWAYS_INLINE JSActivation* Register::activation() const
    {
        return u.activation;
    }
    
    ALWAYS_INLINE CallFrame* Register::callFrame() const
    {
        return u.callFrame;
    }
    
    ALWAYS_INLINE CodeBlock* Register::codeBlock() const
    {
        return u.codeBlock;
    }
    
    ALWAYS_INLINE TiFunction* Register::function() const
    {
        return u.function;
    }
    
    ALWAYS_INLINE TiPropertyNameIterator* Register::propertyNameIterator() const
    {
        return u.propertyNameIterator;
    }
    
    ALWAYS_INLINE ScopeChainNode* Register::scopeChain() const
    {
        return u.scopeChain;
    }
    
    ALWAYS_INLINE Instruction* Register::vPC() const
    {
        return u.vPC;
    }

} // namespace TI

namespace WTI {

    template<> struct VectorTraits<TI::Register> : VectorTraitsBase<true, TI::Register> { };

} // namespace WTI

#endif // Register_h
