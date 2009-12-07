/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef TiString_h
#define TiString_h

#include "CallFrame.h"
#include "CommonIdentifiers.h"
#include "Identifier.h"
#include "JSNumberCell.h"
#include "PropertyDescriptor.h"
#include "PropertySlot.h"

namespace TI {

    class TiString;

    TiString* jsEmptyString(TiGlobalData*);
    TiString* jsEmptyString(TiExcState*);
    TiString* jsString(TiGlobalData*, const UString&); // returns empty string if passed null string
    TiString* jsString(TiExcState*, const UString&); // returns empty string if passed null string

    TiString* jsSingleCharacterString(TiGlobalData*, UChar);
    TiString* jsSingleCharacterString(TiExcState*, UChar);
    TiString* jsSingleCharacterSubstring(TiGlobalData*, const UString&, unsigned offset);
    TiString* jsSingleCharacterSubstring(TiExcState*, const UString&, unsigned offset);
    TiString* jsSubstring(TiGlobalData*, const UString&, unsigned offset, unsigned length);
    TiString* jsSubstring(TiExcState*, const UString&, unsigned offset, unsigned length);

    // Non-trivial strings are two or more characters long.
    // These functions are faster than just calling jsString.
    TiString* jsNontrivialString(TiGlobalData*, const UString&);
    TiString* jsNontrivialString(TiExcState*, const UString&);
    TiString* jsNontrivialString(TiGlobalData*, const char*);
    TiString* jsNontrivialString(TiExcState*, const char*);

    // Should be used for strings that are owned by an object that will
    // likely outlive the TiValue this makes, such as the parse tree or a
    // DOM object that contains a UString
    TiString* jsOwnedString(TiGlobalData*, const UString&); 
    TiString* jsOwnedString(TiExcState*, const UString&); 

    class TiString : public TiCell {
        friend class JIT;
        friend struct VPtrSet;

    public:
        TiString(TiGlobalData* globalData, const UString& value)
            : TiCell(globalData->stringStructure.get())
            , m_value(value)
        {
            Heap::heap(this)->reportExtraMemoryCost(value.cost());
        }

        enum HasOtherOwnerType { HasOtherOwner };
        TiString(TiGlobalData* globalData, const UString& value, HasOtherOwnerType)
            : TiCell(globalData->stringStructure.get())
            , m_value(value)
        {
        }
        TiString(TiGlobalData* globalData, PassRefPtr<UString::Rep> value, HasOtherOwnerType)
            : TiCell(globalData->stringStructure.get())
            , m_value(value)
        {
        }
        
        const UString& value() const { return m_value; }

        bool getStringPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        bool getStringPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        bool getStringPropertyDescriptor(TiExcState*, const Identifier& propertyName, PropertyDescriptor&);

        bool canGetIndex(unsigned i) { return i < static_cast<unsigned>(m_value.size()); }
        TiString* getIndex(TiGlobalData*, unsigned);

        static PassRefPtr<Structure> createStructure(TiValue proto) { return Structure::create(proto, TypeInfo(StringType, OverridesGetOwnPropertySlot | NeedsThisConversion)); }

    private:
        enum VPtrStealingHackType { VPtrStealingHack };
        TiString(VPtrStealingHackType) 
            : TiCell(0)
        {
        }

        virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(TiExcState*, double& number, TiValue& value);
        virtual bool toBoolean(TiExcState*) const;
        virtual double toNumber(TiExcState*) const;
        virtual TiObject* toObject(TiExcState*) const;
        virtual UString toString(TiExcState*) const;

        virtual TiObject* toThisObject(TiExcState*) const;
        virtual UString toThisString(TiExcState*) const;
        virtual TiString* toThisTiString(TiExcState*);

        // Actually getPropertySlot, not getOwnPropertySlot (see TiCell).
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);

        UString m_value;
    };

    TiString* asString(TiValue);

    inline TiString* asString(TiValue value)
    {
        ASSERT(asCell(value)->isString());
        return static_cast<TiString*>(asCell(value));
    }

    inline TiString* jsEmptyString(TiGlobalData* globalData)
    {
        return globalData->smallStrings.emptyString(globalData);
    }

    inline TiString* jsSingleCharacterString(TiGlobalData* globalData, UChar c)
    {
        if (c <= 0xFF)
            return globalData->smallStrings.singleCharacterString(globalData, c);
        return new (globalData) TiString(globalData, UString(&c, 1));
    }

    inline TiString* jsSingleCharacterSubstring(TiGlobalData* globalData, const UString& s, unsigned offset)
    {
        ASSERT(offset < static_cast<unsigned>(s.size()));
        UChar c = s.data()[offset];
        if (c <= 0xFF)
            return globalData->smallStrings.singleCharacterString(globalData, c);
        return new (globalData) TiString(globalData, UString::Rep::create(s.rep(), offset, 1));
    }

    inline TiString* jsNontrivialString(TiGlobalData* globalData, const char* s)
    {
        ASSERT(s);
        ASSERT(s[0]);
        ASSERT(s[1]);
        return new (globalData) TiString(globalData, s);
    }

    inline TiString* jsNontrivialString(TiGlobalData* globalData, const UString& s)
    {
        ASSERT(s.size() > 1);
        return new (globalData) TiString(globalData, s);
    }

    inline TiString* TiString::getIndex(TiGlobalData* globalData, unsigned i)
    {
        ASSERT(canGetIndex(i));
        return jsSingleCharacterSubstring(globalData, m_value, i);
    }

    inline TiString* jsString(TiGlobalData* globalData, const UString& s)
    {
        int size = s.size();
        if (!size)
            return globalData->smallStrings.emptyString(globalData);
        if (size == 1) {
            UChar c = s.data()[0];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return new (globalData) TiString(globalData, s);
    }
        
    inline TiString* jsSubstring(TiGlobalData* globalData, const UString& s, unsigned offset, unsigned length)
    {
        ASSERT(offset <= static_cast<unsigned>(s.size()));
        ASSERT(length <= static_cast<unsigned>(s.size()));
        ASSERT(offset + length <= static_cast<unsigned>(s.size()));
        if (!length)
            return globalData->smallStrings.emptyString(globalData);
        if (length == 1) {
            UChar c = s.data()[offset];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return new (globalData) TiString(globalData, UString::Rep::create(s.rep(), offset, length));
    }

    inline TiString* jsOwnedString(TiGlobalData* globalData, const UString& s)
    {
        int size = s.size();
        if (!size)
            return globalData->smallStrings.emptyString(globalData);
        if (size == 1) {
            UChar c = s.data()[0];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return new (globalData) TiString(globalData, s, TiString::HasOtherOwner);
    }

    inline TiString* jsEmptyString(TiExcState* exec) { return jsEmptyString(&exec->globalData()); }
    inline TiString* jsString(TiExcState* exec, const UString& s) { return jsString(&exec->globalData(), s); }
    inline TiString* jsSingleCharacterString(TiExcState* exec, UChar c) { return jsSingleCharacterString(&exec->globalData(), c); }
    inline TiString* jsSingleCharacterSubstring(TiExcState* exec, const UString& s, unsigned offset) { return jsSingleCharacterSubstring(&exec->globalData(), s, offset); }
    inline TiString* jsSubstring(TiExcState* exec, const UString& s, unsigned offset, unsigned length) { return jsSubstring(&exec->globalData(), s, offset, length); }
    inline TiString* jsNontrivialString(TiExcState* exec, const UString& s) { return jsNontrivialString(&exec->globalData(), s); }
    inline TiString* jsNontrivialString(TiExcState* exec, const char* s) { return jsNontrivialString(&exec->globalData(), s); }
    inline TiString* jsOwnedString(TiExcState* exec, const UString& s) { return jsOwnedString(&exec->globalData(), s); } 

    ALWAYS_INLINE bool TiString::getStringPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        if (propertyName == exec->propertyNames().length) {
            slot.setValue(jsNumber(exec, m_value.size()));
            return true;
        }

        bool isStrictUInt32;
        unsigned i = propertyName.toStrictUInt32(&isStrictUInt32);
        if (isStrictUInt32 && i < static_cast<unsigned>(m_value.size())) {
            slot.setValue(jsSingleCharacterSubstring(exec, m_value, i));
            return true;
        }

        return false;
    }
        
    ALWAYS_INLINE bool TiString::getStringPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
    {
        if (propertyName < static_cast<unsigned>(m_value.size())) {
            slot.setValue(jsSingleCharacterSubstring(exec, m_value, propertyName));
            return true;
        }

        return false;
    }

    inline bool isTiString(TiGlobalData* globalData, TiValue v) { return v.isCell() && v.asCell()->vptr() == globalData->jsStringVPtr; }

    // --- TiValue inlines ----------------------------

    inline TiString* TiValue::toThisTiString(TiExcState* exec)
    {
        return isCell() ? asCell()->toThisTiString(exec) : jsString(exec, toString(exec));
    }

    inline UString TiValue::toString(TiExcState* exec) const
    {
        if (isString())
            return static_cast<TiString*>(asCell())->value();
        if (isInt32())
            return exec->globalData().numericStrings.add(asInt32());
        if (isDouble())
            return exec->globalData().numericStrings.add(asDouble());
        if (isTrue())
            return "true";
        if (isFalse())
            return "false";
        if (isNull())
            return "null";
        if (isUndefined())
            return "undefined";
        ASSERT(isCell());
        return asCell()->toString(exec);
    }

} // namespace TI

#endif // TiString_h
