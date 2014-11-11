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
#include "DFGLazyTiValue.h"

#if ENABLE(DFG_JIT)

#include "Operations.h"

namespace TI { namespace DFG {

TiValue LazyTiValue::getValue(VM& vm) const
{
    switch (m_kind) {
    case KnownValue:
        return value();
    case SingleCharacterString:
        return jsSingleCharacterString(&vm, u.character);
    case KnownStringImpl:
        return jsString(&vm, u.stringImpl);
    }
    RELEASE_ASSERT_NOT_REACHED();
    return value();
}

static TriState equalToSingleCharacter(TiValue value, UChar character)
{
    if (!value.isString())
        return FalseTriState;
    
    JSString* jsString = asString(value);
    if (jsString->length() != 1)
        return FalseTriState;
    
    const StringImpl* string = jsString->tryGetValueImpl();
    if (!string)
        return MixedTriState;
    
    return triState(string->at(0) == character);
}

static TriState equalToStringImpl(TiValue value, StringImpl* stringImpl)
{
    if (!value.isString())
        return FalseTriState;
    
    JSString* jsString = asString(value);
    const StringImpl* string = jsString->tryGetValueImpl();
    if (!string)
        return MixedTriState;
    
    return triState(WTI::equal(stringImpl, string));
}

TriState LazyTiValue::strictEqual(const LazyTiValue& other) const
{
    switch (m_kind) {
    case KnownValue:
        switch (other.m_kind) {
        case KnownValue:
            return TiValue::pureStrictEqual(value(), other.value());
        case SingleCharacterString:
            return equalToSingleCharacter(value(), other.character());
        case KnownStringImpl:
            return equalToStringImpl(value(), other.stringImpl());
        }
        break;
    case SingleCharacterString:
        switch (other.m_kind) {
        case SingleCharacterString:
            return triState(character() == other.character());
        case KnownStringImpl:
            if (other.stringImpl()->length() != 1)
                return FalseTriState;
            return triState(other.stringImpl()->at(0) == character());
        default:
            return other.strictEqual(*this);
        }
        break;
    case KnownStringImpl:
        switch (other.m_kind) {
        case KnownStringImpl:
            return triState(WTI::equal(stringImpl(), other.stringImpl()));
        default:
            return other.strictEqual(*this);
        }
        break;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return FalseTriState;
}

void LazyTiValue::dumpInContext(PrintStream& out, DumpContext* context) const
{
    switch (m_kind) {
    case KnownValue:
        value().dumpInContext(out, context);
        return;
    case SingleCharacterString:
        out.print("Lazy:SingleCharacterString(");
        out.printf("%04X", static_cast<unsigned>(character()));
        out.print(" / ", StringImpl::utf8ForCharacters(&u.character, 1), ")");
        return;
    case KnownStringImpl:
        out.print("Lazy:String(", stringImpl(), ")");
        return;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

void LazyTiValue::dump(PrintStream& out) const
{
    dumpInContext(out, 0);
}

} } // namespace TI::DFG

#endif // ENABLE(DFG_JIT)

