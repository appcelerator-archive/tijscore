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

#ifndef DFGLazyTiValue_h
#define DFGLazyTiValue_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "JSCTiValue.h"
#include <wtf/text/StringImpl.h>

namespace TI { namespace DFG {

// Represents either a TiValue, or for TiValues that require allocation in the heap,
// it tells you everything you'd need to know in order to allocate it.

enum LazinessKind {
    KnownValue,
    SingleCharacterString,
    KnownStringImpl
};

class LazyTiValue {
public:
    LazyTiValue(TiValue value = TiValue())
        : m_kind(KnownValue)
    {
        u.value = TiValue::encode(value);
    }
    
    static LazyTiValue singleCharacterString(UChar character)
    {
        LazyTiValue result;
        result.m_kind = SingleCharacterString;
        result.u.character = character;
        return result;
    }
    
    static LazyTiValue knownStringImpl(StringImpl* string)
    {
        LazyTiValue result;
        result.m_kind = KnownStringImpl;
        result.u.stringImpl = string;
        return result;
    }
    
    TiValue tryGetValue() const
    {
        if (m_kind == KnownValue)
            return value();
        return TiValue();
    }
    
    TiValue getValue(VM&) const;
    
    TiValue value() const
    {
        ASSERT(m_kind == KnownValue);
        return TiValue::decode(u.value);
    }
    
    UChar character() const
    {
        ASSERT(m_kind == SingleCharacterString);
        return u.character;
    }
    
    StringImpl* stringImpl() const
    {
        ASSERT(m_kind == KnownStringImpl);
        return u.stringImpl;
    }
    
    TriState strictEqual(const LazyTiValue& other) const;
    
    unsigned switchLookupValue() const
    {
        // NB. Not every kind of TiValue will be able to give you a switch lookup
        // value, and this method will assert, or do bad things, if you use it
        // for a kind of value that can't.
        switch (m_kind) {
        case KnownValue:
            return value().asInt32();
        case SingleCharacterString:
            return character();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return 0;
        }
    }
    
    void dump(PrintStream&) const;
    void dumpInContext(PrintStream&, DumpContext*) const;
    
private:
    union {
        EncodedTiValue value;
        UChar character;
        StringImpl* stringImpl;
    } u;
    LazinessKind m_kind;
};

} } // namespace TI::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGLazyTiValue_h

