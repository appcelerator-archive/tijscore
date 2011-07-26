/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef TiTypeInfo_h
#define TiTypeInfo_h

// This file would be called TypeInfo.h, but that conflicts with <typeinfo.h>
// in the STL on systems without case-sensitive file systems. 

#include "TiType.h"

namespace TI {

    // WebCore uses MasqueradesAsUndefined to make document.all and style.filter undetectable.
    static const unsigned MasqueradesAsUndefined = 1;
    static const unsigned ImplementsHasInstance = 1 << 1;
    static const unsigned OverridesHasInstance = 1 << 2;
    static const unsigned ImplementsDefaultHasInstance = 1 << 3;
    static const unsigned NeedsThisConversion = 1 << 4;
    static const unsigned OverridesGetOwnPropertySlot = 1 << 5;
    static const unsigned OverridesMarkChildren = 1 << 6;
    static const unsigned OverridesGetPropertyNames = 1 << 7;

    class TypeInfo {
        friend class JIT;
    public:
        TypeInfo(TiType type, unsigned flags = 0)
            : m_type(type)
        {
            ASSERT(flags <= 0xFF);
            ASSERT(type <= 0xFF);
            // ImplementsDefaultHasInstance means (ImplementsHasInstance & !OverridesHasInstance)
            if ((flags & (ImplementsHasInstance | OverridesHasInstance)) == ImplementsHasInstance)
                m_flags = flags | ImplementsDefaultHasInstance;
            else
                m_flags = flags;
        }

        TiType type() const { return (TiType)m_type; }

        bool masqueradesAsUndefined() const { return m_flags & MasqueradesAsUndefined; }
        bool implementsHasInstance() const { return m_flags & ImplementsHasInstance; }
        bool overridesHasInstance() const { return m_flags & OverridesHasInstance; }
        bool needsThisConversion() const { return m_flags & NeedsThisConversion; }
        bool overridesGetOwnPropertySlot() const { return m_flags & OverridesGetOwnPropertySlot; }
        bool overridesMarkChildren() const { return m_flags & OverridesMarkChildren; }
        bool overridesGetPropertyNames() const { return m_flags & OverridesGetPropertyNames; }
        unsigned flags() const { return m_flags; }

    private:
        unsigned char m_type;
        unsigned char m_flags;
    };

}

#endif // TiTypeInfo_h
