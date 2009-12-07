/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "UString.h"

namespace TI {

    class TiExcState;

    class Identifier {
        friend class Structure;
    public:
        Identifier() { }

        Identifier(TiExcState* exec, const char* s) : _ustring(add(exec, s)) { } // Only to be used with string literals.
        Identifier(TiExcState* exec, const UChar* s, int length) : _ustring(add(exec, s, length)) { }
        Identifier(TiExcState* exec, UString::Rep* rep) : _ustring(add(exec, rep)) { } 
        Identifier(TiExcState* exec, const UString& s) : _ustring(add(exec, s.rep())) { }

        Identifier(TiGlobalData* globalData, const char* s) : _ustring(add(globalData, s)) { } // Only to be used with string literals.
        Identifier(TiGlobalData* globalData, const UChar* s, int length) : _ustring(add(globalData, s, length)) { }
        Identifier(TiGlobalData* globalData, UString::Rep* rep) : _ustring(add(globalData, rep)) { } 
        Identifier(TiGlobalData* globalData, const UString& s) : _ustring(add(globalData, s.rep())) { }

        // Special constructor for cases where we overwrite an object in place.
        Identifier(PlacementNewAdoptType) : _ustring(PlacementNewAdopt) { }
        
        const UString& ustring() const { return _ustring; }
        
        const UChar* data() const { return _ustring.data(); }
        int size() const { return _ustring.size(); }
        
        const char* ascii() const { return _ustring.ascii(); }
        
        static Identifier from(TiExcState* exec, unsigned y) { return Identifier(exec, UString::from(y)); }
        static Identifier from(TiExcState* exec, int y) { return Identifier(exec, UString::from(y)); }
        static Identifier from(TiExcState* exec, double y) { return Identifier(exec, UString::from(y)); }
        
        bool isNull() const { return _ustring.isNull(); }
        bool isEmpty() const { return _ustring.isEmpty(); }
        
        uint32_t toUInt32(bool* ok) const { return _ustring.toUInt32(ok); }
        uint32_t toUInt32(bool* ok, bool tolerateEmptyString) const { return _ustring.toUInt32(ok, tolerateEmptyString); };
        uint32_t toStrictUInt32(bool* ok) const { return _ustring.toStrictUInt32(ok); }
        unsigned toArrayIndex(bool* ok) const { return _ustring.toArrayIndex(ok); }
        double toDouble() const { return _ustring.toDouble(); }
        
        friend bool operator==(const Identifier&, const Identifier&);
        friend bool operator!=(const Identifier&, const Identifier&);

        friend bool operator==(const Identifier&, const char*);
        friend bool operator!=(const Identifier&, const char*);
    
        static void remove(UString::Rep*);

        static bool equal(const UString::Rep*, const char*);
        static bool equal(const UString::Rep*, const UChar*, int length);
        static bool equal(const UString::Rep* a, const UString::Rep* b) { return TI::equal(a, b); }

        static PassRefPtr<UString::Rep> add(TiExcState*, const char*); // Only to be used with string literals.
        static PassRefPtr<UString::Rep> add(TiGlobalData*, const char*); // Only to be used with string literals.

    private:
        UString _ustring;
        
        static bool equal(const Identifier& a, const Identifier& b) { return a._ustring.rep() == b._ustring.rep(); }
        static bool equal(const Identifier& a, const char* b) { return equal(a._ustring.rep(), b); }

        static PassRefPtr<UString::Rep> add(TiExcState*, const UChar*, int length);
        static PassRefPtr<UString::Rep> add(TiGlobalData*, const UChar*, int length);

        static PassRefPtr<UString::Rep> add(TiExcState* exec, UString::Rep* r)
        {
            if (r->identifierTable()) {
#ifndef NDEBUG
                checkSameIdentifierTable(exec, r);
#endif
                return r;
            }
            return addSlowCase(exec, r);
        }
        static PassRefPtr<UString::Rep> add(TiGlobalData* globalData, UString::Rep* r)
        {
            if (r->identifierTable()) {
#ifndef NDEBUG
                checkSameIdentifierTable(globalData, r);
#endif
                return r;
            }
            return addSlowCase(globalData, r);
        }

        static PassRefPtr<UString::Rep> addSlowCase(TiExcState*, UString::Rep* r);
        static PassRefPtr<UString::Rep> addSlowCase(TiGlobalData*, UString::Rep* r);

        static void checkSameIdentifierTable(TiExcState*, UString::Rep*);
        static void checkSameIdentifierTable(TiGlobalData*, UString::Rep*);
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

    IdentifierTable* createIdentifierTable();
    void deleteIdentifierTable(IdentifierTable*);

} // namespace TI

#endif // Identifier_h
