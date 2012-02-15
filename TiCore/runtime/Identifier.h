/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Identifier_h
#define Identifier_h

#include "TiGlobalData.h"
#include "ThreadSpecific.h"
#include "UString.h"
#include <wtf/text/CString.h>

namespace TI {

    class TiExcState;

    class Identifier {
        friend class Structure;
    public:
        Identifier() { }

        Identifier(TiExcState* exec, const char* s) : m_string(add(exec, s)) { } // Only to be used with string literals.
        Identifier(TiExcState* exec, const UChar* s, int length) : m_string(add(exec, s, length)) { }
        Identifier(TiExcState* exec, StringImpl* rep) : m_string(add(exec, rep)) { } 
        Identifier(TiExcState* exec, const UString& s) : m_string(add(exec, s.impl())) { }

        Identifier(TiGlobalData* globalData, const char* s) : m_string(add(globalData, s)) { } // Only to be used with string literals.
        Identifier(TiGlobalData* globalData, const UChar* s, int length) : m_string(add(globalData, s, length)) { }
        Identifier(TiGlobalData* globalData, StringImpl* rep) : m_string(add(globalData, rep)) { } 
        Identifier(TiGlobalData* globalData, const UString& s) : m_string(add(globalData, s.impl())) { }

        const UString& ustring() const { return m_string; }
        StringImpl* impl() const { return m_string.impl(); }
        
        const UChar* characters() const { return m_string.characters(); }
        int length() const { return m_string.length(); }
        
        CString ascii() const { return m_string.ascii(); }
        
        static Identifier from(TiExcState* exec, unsigned y);
        static Identifier from(TiExcState* exec, int y);
        static Identifier from(TiExcState* exec, double y);
        static Identifier from(TiGlobalData*, unsigned y);
        static Identifier from(TiGlobalData*, int y);
        static Identifier from(TiGlobalData*, double y);

        static uint32_t toUInt32(const UString&, bool& ok);
        uint32_t toUInt32(bool& ok) const { return toUInt32(m_string, ok); }
        unsigned toArrayIndex(bool& ok) const;

        bool isNull() const { return m_string.isNull(); }
        bool isEmpty() const { return m_string.isEmpty(); }
        
        friend bool operator==(const Identifier&, const Identifier&);
        friend bool operator!=(const Identifier&, const Identifier&);

        friend bool operator==(const Identifier&, const char*);
        friend bool operator!=(const Identifier&, const char*);
    
        static bool equal(const StringImpl*, const char*);
        static bool equal(const StringImpl*, const UChar*, unsigned length);
        static bool equal(const StringImpl* a, const StringImpl* b) { return ::equal(a, b); }

        static PassRefPtr<StringImpl> add(TiExcState*, const char*); // Only to be used with string literals.
        static PassRefPtr<StringImpl> add(TiGlobalData*, const char*); // Only to be used with string literals.

    private:
        UString m_string;
        
        static bool equal(const Identifier& a, const Identifier& b) { return a.m_string.impl() == b.m_string.impl(); }
        static bool equal(const Identifier& a, const char* b) { return equal(a.m_string.impl(), b); }

        static PassRefPtr<StringImpl> add(TiExcState*, const UChar*, int length);
        static PassRefPtr<StringImpl> add(TiGlobalData*, const UChar*, int length);

        static PassRefPtr<StringImpl> add(TiExcState* exec, StringImpl* r)
        {
#ifndef NDEBUG
            checkCurrentIdentifierTable(exec);
#endif
            if (r->isIdentifier())
                return r;
            return addSlowCase(exec, r);
        }
        static PassRefPtr<StringImpl> add(TiGlobalData* globalData, StringImpl* r)
        {
#ifndef NDEBUG
            checkCurrentIdentifierTable(globalData);
#endif
            if (r->isIdentifier())
                return r;
            return addSlowCase(globalData, r);
        }

        static PassRefPtr<StringImpl> addSlowCase(TiExcState*, StringImpl* r);
        static PassRefPtr<StringImpl> addSlowCase(TiGlobalData*, StringImpl* r);

        static void checkCurrentIdentifierTable(TiExcState*);
        static void checkCurrentIdentifierTable(TiGlobalData*);
    };
    
    inline bool operator==(const Identifier& a, const Identifier& b)
    {
        return Identifier::equal(a, b);
    }

    inline bool operator!=(const Identifier& a, const Identifier& b)
    {
        return !Identifier::equal(a, b);
    }

    inline bool operator==(const Identifier& a, const char* b)
    {
        return Identifier::equal(a, b);
    }

    inline bool operator!=(const Identifier& a, const char* b)
    {
        return !Identifier::equal(a, b);
    }

    inline bool Identifier::equal(const StringImpl* r, const UChar* s, unsigned length)
    {
        if (r->length() != length)
            return false;
        const UChar* d = r->characters();
        for (unsigned i = 0; i != length; ++i)
            if (d[i] != s[i])
                return false;
        return true;
    }
    
    IdentifierTable* createIdentifierTable();
    void deleteIdentifierTable(IdentifierTable*);

    struct IdentifierRepHash : PtrHash<RefPtr<StringImpl> > {
        static unsigned hash(const RefPtr<StringImpl>& key) { return key->existingHash(); }
        static unsigned hash(StringImpl* key) { return key->existingHash(); }
    };

} // namespace TI

#endif // Identifier_h
