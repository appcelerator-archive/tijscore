/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef TiValue_h
#define TiValue_h

#include "CallData.h"
#include "ConstructData.h"
#include <math.h>
#include <stddef.h> // for size_t
#include <stdint.h>
#include <wtf/AlwaysInline.h>
#include <wtf/Assertions.h>
#include <wtf/HashTraits.h>
#include <wtf/MathExtras.h>

namespace TI {

    class Identifier;
    class TiCell;
    class TiGlobalData;
    class JSImmediate;
    class TiObject;
    class TiString;
    class PropertySlot;
    class PutPropertySlot;
    class UString;

    struct ClassInfo;
    struct Instruction;

    enum PreferredPrimitiveType { NoPreference, PreferNumber, PreferString };

#if USE(JSVALUE32_64)
    typedef int64_t EncodedTiValue;
#else
    typedef void* EncodedTiValue;
#endif

    double nonInlineNaN();
    int32_t toInt32SlowCase(double, bool& ok);
    uint32_t toUInt32SlowCase(double, bool& ok);

    class TiValue {
        friend class JSImmediate;
        friend struct EncodedTiValueHashTraits;
        friend class JIT;
        friend class JITStubs;
        friend class JITStubCall;

    public:
        static EncodedTiValue encode(TiValue value);
        static TiValue decode(EncodedTiValue ptr);
#if !USE(JSVALUE32_64)
    private:
        static TiValue makeImmediate(intptr_t value);
        intptr_t immediateValue();
    public:
#endif
        enum JSNullTag { JSNull };
        enum JSUndefinedTag { JSUndefined };
        enum JSTrueTag { JSTrue };
        enum JSFalseTag { JSFalse };

        TiValue();
        TiValue(JSNullTag);
        TiValue(JSUndefinedTag);
        TiValue(JSTrueTag);
        TiValue(JSFalseTag);
        TiValue(TiCell* ptr);
        TiValue(const TiCell* ptr);

        // Numbers
        TiValue(TiExcState*, double);
        TiValue(TiExcState*, char);
        TiValue(TiExcState*, unsigned char);
        TiValue(TiExcState*, short);
        TiValue(TiExcState*, unsigned short);
        TiValue(TiExcState*, int);
        TiValue(TiExcState*, unsigned);
        TiValue(TiExcState*, long);
        TiValue(TiExcState*, unsigned long);
        TiValue(TiExcState*, long long);
        TiValue(TiExcState*, unsigned long long);
        TiValue(TiGlobalData*, double);
        TiValue(TiGlobalData*, int);
        TiValue(TiGlobalData*, unsigned);

        operator bool() const;
        bool operator==(const TiValue& other) const;
        bool operator!=(const TiValue& other) const;

        bool isInt32() const;
        bool isUInt32() const;
        bool isDouble() const;
        bool isTrue() const;
        bool isFalse() const;

        int32_t asInt32() const;
        uint32_t asUInt32() const;
        double asDouble() const;

        // Querying the type.
        bool isUndefined() const;
        bool isNull() const;
        bool isUndefinedOrNull() const;
        bool isBoolean() const;
        bool isNumber() const;
        bool isString() const;
        bool isGetterSetter() const;
        bool isObject() const;
        bool inherits(const ClassInfo*) const;
        
        // Extracting the value.
        bool getBoolean(bool&) const;
        bool getBoolean() const; // false if not a boolean
        bool getNumber(double&) const;
        double uncheckedGetNumber() const;
        bool getString(UString&) const;
        UString getString() const; // null string if not a string
        TiObject* getObject() const; // 0 if not an object

        CallType getCallData(CallData&);
        ConstructType getConstructData(ConstructData&);

        // Extracting integer values.
        bool getUInt32(uint32_t&) const;
        
        // Basic conversions.
        TiValue toPrimitive(TiExcState*, PreferredPrimitiveType = NoPreference) const;
        bool getPrimitiveNumber(TiExcState*, double& number, TiValue&);

        bool toBoolean(TiExcState*) const;

        // toNumber conversion is expected to be side effect free if an exception has
        // been set in the TiExcState already.
        double toNumber(TiExcState*) const;
        TiValue toJSNumber(TiExcState*) const; // Fast path for when you expect that the value is an immediate number.
        UString toString(TiExcState*) const;
        TiObject* toObject(TiExcState*) const;

        // Integer conversions.
        double toInteger(TiExcState*) const;
        double toIntegerPreserveNaN(TiExcState*) const;
        int32_t toInt32(TiExcState*) const;
        int32_t toInt32(TiExcState*, bool& ok) const;
        uint32_t toUInt32(TiExcState*) const;
        uint32_t toUInt32(TiExcState*, bool& ok) const;

        // Floating point conversions (this is a convenience method for webcore;
        // signle precision float is not a representation used in JS or JSC).
        float toFloat(TiExcState* exec) const { return static_cast<float>(toNumber(exec)); }

        // Object operations, with the toObject operation included.
        TiValue get(TiExcState*, const Identifier& propertyName) const;
        TiValue get(TiExcState*, const Identifier& propertyName, PropertySlot&) const;
        TiValue get(TiExcState*, unsigned propertyName) const;
        TiValue get(TiExcState*, unsigned propertyName, PropertySlot&) const;
        void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        void put(TiExcState*, unsigned propertyName, TiValue);

        bool needsThisConversion() const;
        TiObject* toThisObject(TiExcState*) const;
        UString toThisString(TiExcState*) const;
        TiString* toThisTiString(TiExcState*);

        static bool equal(TiExcState* exec, TiValue v1, TiValue v2);
        static bool equalSlowCase(TiExcState* exec, TiValue v1, TiValue v2);
        static bool equalSlowCaseInline(TiExcState* exec, TiValue v1, TiValue v2);
        static bool strictEqual(TiValue v1, TiValue v2);
        static bool strictEqualSlowCase(TiValue v1, TiValue v2);
        static bool strictEqualSlowCaseInline(TiValue v1, TiValue v2);

        TiValue getJSNumber(); // TiValue() if this is not a JSNumber or number object

        bool isCell() const;
        TiCell* asCell() const;

#ifndef NDEBUG
        char* description();
#endif

    private:
        enum HashTableDeletedValueTag { HashTableDeletedValue };
        TiValue(HashTableDeletedValueTag);

        inline const TiValue asValue() const { return *this; }
        TiObject* toObjectSlowCase(TiExcState*) const;
        TiObject* toThisObjectSlowCase(TiExcState*) const;

        enum { Int32Tag =        0xffffffff };
        enum { CellTag =         0xfffffffe };
        enum { TrueTag =         0xfffffffd };
        enum { FalseTag =        0xfffffffc };
        enum { NullTag =         0xfffffffb };
        enum { UndefinedTag =    0xfffffffa };
        enum { EmptyValueTag =   0xfffffff9 };
        enum { DeletedValueTag = 0xfffffff8 };

        enum { LowestTag =  DeletedValueTag };

        uint32_t tag() const;
        int32_t payload() const;

        TiObject* synthesizePrototype(TiExcState*) const;
        TiObject* synthesizeObject(TiExcState*) const;

#if USE(JSVALUE32_64)
        union {
            EncodedTiValue asEncodedTiValue;
            double asDouble;
#if PLATFORM(BIG_ENDIAN)
            struct {
                int32_t tag;
                int32_t payload;
            } asBits;
#else
            struct {
                int32_t payload;
                int32_t tag;
            } asBits;
#endif
        } u;
#else // USE(JSVALUE32_64)
        TiCell* m_ptr;
#endif // USE(JSVALUE32_64)
    };

#if USE(JSVALUE32_64)
    typedef IntHash<EncodedTiValue> EncodedTiValueHash;

    struct EncodedTiValueHashTraits : HashTraits<EncodedTiValue> {
        static const bool emptyValueIsZero = false;
        static EncodedTiValue emptyValue() { return TiValue::encode(TiValue()); }
        static void constructDeletedValue(EncodedTiValue& slot) { slot = TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
        static bool isDeletedValue(EncodedTiValue value) { return value == TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
    };
#else
    typedef PtrHash<EncodedTiValue> EncodedTiValueHash;

    struct EncodedTiValueHashTraits : HashTraits<EncodedTiValue> {
        static void constructDeletedValue(EncodedTiValue& slot) { slot = TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
        static bool isDeletedValue(EncodedTiValue value) { return value == TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
    };
#endif

    // Stand-alone helper functions.
    inline TiValue jsNull()
    {
        return TiValue(TiValue::JSNull);
    }

    inline TiValue jsUndefined()
    {
        return TiValue(TiValue::JSUndefined);
    }

    inline TiValue jsBoolean(bool b)
    {
        return b ? TiValue(TiValue::JSTrue) : TiValue(TiValue::JSFalse);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, double d)
    {
        return TiValue(exec, d);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, char i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, unsigned char i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, short i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, unsigned short i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, int i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, unsigned i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, long i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, unsigned long i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, long long i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiExcState* exec, unsigned long long i)
    {
        return TiValue(exec, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiGlobalData* globalData, double d)
    {
        return TiValue(globalData, d);
    }

    ALWAYS_INLINE TiValue jsNumber(TiGlobalData* globalData, int i)
    {
        return TiValue(globalData, i);
    }

    ALWAYS_INLINE TiValue jsNumber(TiGlobalData* globalData, unsigned i)
    {
        return TiValue(globalData, i);
    }

    inline bool operator==(const TiValue a, const TiCell* b) { return a == TiValue(b); }
    inline bool operator==(const TiCell* a, const TiValue b) { return TiValue(a) == b; }

    inline bool operator!=(const TiValue a, const TiCell* b) { return a != TiValue(b); }
    inline bool operator!=(const TiCell* a, const TiValue b) { return TiValue(a) != b; }

    inline int32_t toInt32(double val)
    {
        if (!(val >= -2147483648.0 && val < 2147483648.0)) {
            bool ignored;
            return toInt32SlowCase(val, ignored);
        }
        return static_cast<int32_t>(val);
    }

    inline uint32_t toUInt32(double val)
    {
        if (!(val >= 0.0 && val < 4294967296.0)) {
            bool ignored;
            return toUInt32SlowCase(val, ignored);
        }
        return static_cast<uint32_t>(val);
    }

    // FIXME: We should deprecate this and just use TiValue::asCell() instead.
    TiCell* asCell(TiValue);

    inline TiCell* asCell(TiValue value)
    {
        return value.asCell();
    }

    ALWAYS_INLINE int32_t TiValue::toInt32(TiExcState* exec) const
    {
        if (isInt32())
            return asInt32();
        bool ignored;
        return toInt32SlowCase(toNumber(exec), ignored);
    }

    inline uint32_t TiValue::toUInt32(TiExcState* exec) const
    {
        if (isUInt32())
            return asInt32();
        bool ignored;
        return toUInt32SlowCase(toNumber(exec), ignored);
    }

    inline int32_t TiValue::toInt32(TiExcState* exec, bool& ok) const
    {
        if (isInt32()) {
            ok = true;
            return asInt32();
        }
        return toInt32SlowCase(toNumber(exec), ok);
    }

    inline uint32_t TiValue::toUInt32(TiExcState* exec, bool& ok) const
    {
        if (isUInt32()) {
            ok = true;
            return asInt32();
        }
        return toUInt32SlowCase(toNumber(exec), ok);
    }

#if USE(JSVALUE32_64)
    inline TiValue jsNaN(TiExcState* exec)
    {
        return TiValue(exec, nonInlineNaN());
    }

    // TiValue member functions.
    inline EncodedTiValue TiValue::encode(TiValue value)
    {
        return value.u.asEncodedTiValue;
    }

    inline TiValue TiValue::decode(EncodedTiValue encodedTiValue)
    {
        TiValue v;
        v.u.asEncodedTiValue = encodedTiValue;
        return v;
    }

    inline TiValue::TiValue()
    {
        u.asBits.tag = EmptyValueTag;
        u.asBits.payload = 0;
    }

    inline TiValue::TiValue(JSNullTag)
    {
        u.asBits.tag = NullTag;
        u.asBits.payload = 0;
    }
    
    inline TiValue::TiValue(JSUndefinedTag)
    {
        u.asBits.tag = UndefinedTag;
        u.asBits.payload = 0;
    }
    
    inline TiValue::TiValue(JSTrueTag)
    {
        u.asBits.tag = TrueTag;
        u.asBits.payload = 0;
    }
    
    inline TiValue::TiValue(JSFalseTag)
    {
        u.asBits.tag = FalseTag;
        u.asBits.payload = 0;
    }

    inline TiValue::TiValue(HashTableDeletedValueTag)
    {
        u.asBits.tag = DeletedValueTag;
        u.asBits.payload = 0;
    }

    inline TiValue::TiValue(TiCell* ptr)
    {
        if (ptr)
            u.asBits.tag = CellTag;
        else
            u.asBits.tag = EmptyValueTag;
        u.asBits.payload = reinterpret_cast<int32_t>(ptr);
    }

    inline TiValue::TiValue(const TiCell* ptr)
    {
        if (ptr)
            u.asBits.tag = CellTag;
        else
            u.asBits.tag = EmptyValueTag;
        u.asBits.payload = reinterpret_cast<int32_t>(const_cast<TiCell*>(ptr));
    }

    inline TiValue::operator bool() const
    {
        ASSERT(tag() != DeletedValueTag);
        return tag() != EmptyValueTag;
    }

    inline bool TiValue::operator==(const TiValue& other) const
    {
        return u.asEncodedTiValue == other.u.asEncodedTiValue;
    }

    inline bool TiValue::operator!=(const TiValue& other) const
    {
        return u.asEncodedTiValue != other.u.asEncodedTiValue;
    }

    inline bool TiValue::isUndefined() const
    {
        return tag() == UndefinedTag;
    }

    inline bool TiValue::isNull() const
    {
        return tag() == NullTag;
    }

    inline bool TiValue::isUndefinedOrNull() const
    {
        return isUndefined() || isNull();
    }

    inline bool TiValue::isCell() const
    {
        return tag() == CellTag;
    }

    inline bool TiValue::isInt32() const
    {
        return tag() == Int32Tag;
    }

    inline bool TiValue::isUInt32() const
    {
        return tag() == Int32Tag && asInt32() > -1;
    }

    inline bool TiValue::isDouble() const
    {
        return tag() < LowestTag;
    }

    inline bool TiValue::isTrue() const
    {
        return tag() == TrueTag;
    }

    inline bool TiValue::isFalse() const
    {
        return tag() == FalseTag;
    }

    inline uint32_t TiValue::tag() const
    {
        return u.asBits.tag;
    }
    
    inline int32_t TiValue::payload() const
    {
        return u.asBits.payload;
    }
    
    inline int32_t TiValue::asInt32() const
    {
        ASSERT(isInt32());
        return u.asBits.payload;
    }
    
    inline uint32_t TiValue::asUInt32() const
    {
        ASSERT(isUInt32());
        return u.asBits.payload;
    }
    
    inline double TiValue::asDouble() const
    {
        ASSERT(isDouble());
        return u.asDouble;
    }
    
    ALWAYS_INLINE TiCell* TiValue::asCell() const
    {
        ASSERT(isCell());
        return reinterpret_cast<TiCell*>(u.asBits.payload);
    }

    inline TiValue::TiValue(TiExcState* exec, double d)
    {
        const int32_t asInt32 = static_cast<int32_t>(d);
        if (asInt32 != d || (!asInt32 && signbit(d))) { // true for -0.0
            u.asDouble = d;
            return;
        }
        *this = TiValue(exec, static_cast<int32_t>(d));
    }

    inline TiValue::TiValue(TiExcState* exec, char i)
    {
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned char i)
    {
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, short i)
    {
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned short i)
    {
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState*, int i)
    {
        u.asBits.tag = Int32Tag;
        u.asBits.payload = i;
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned i)
    {
        if (static_cast<int32_t>(i) < 0) {
            *this = TiValue(exec, static_cast<double>(i));
            return;
        }
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, long i)
    {
        if (static_cast<int32_t>(i) != i) {
            *this = TiValue(exec, static_cast<double>(i));
            return;
        }
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned long i)
    {
        if (static_cast<uint32_t>(i) != i) {
            *this = TiValue(exec, static_cast<double>(i));
            return;
        }
        *this = TiValue(exec, static_cast<uint32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, long long i)
    {
        if (static_cast<int32_t>(i) != i) {
            *this = TiValue(exec, static_cast<double>(i));
            return;
        }
        *this = TiValue(exec, static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned long long i)
    {
        if (static_cast<uint32_t>(i) != i) {
            *this = TiValue(exec, static_cast<double>(i));
            return;
        }
        *this = TiValue(exec, static_cast<uint32_t>(i));
    }

    inline TiValue::TiValue(TiGlobalData* globalData, double d)
    {
        const int32_t asInt32 = static_cast<int32_t>(d);
        if (asInt32 != d || (!asInt32 && signbit(d))) { // true for -0.0
            u.asDouble = d;
            return;
        }
        *this = TiValue(globalData, static_cast<int32_t>(d));
    }
    
    inline TiValue::TiValue(TiGlobalData*, int i)
    {
        u.asBits.tag = Int32Tag;
        u.asBits.payload = i;
    }
    
    inline TiValue::TiValue(TiGlobalData* globalData, unsigned i)
    {
        if (static_cast<int32_t>(i) < 0) {
            *this = TiValue(globalData, static_cast<double>(i));
            return;
        }
        *this = TiValue(globalData, static_cast<int32_t>(i));
    }

    inline bool TiValue::isNumber() const
    {
        return isInt32() || isDouble();
    }

    inline bool TiValue::isBoolean() const
    {
        return isTrue() || isFalse();
    }

    inline bool TiValue::getBoolean(bool& v) const
    {
        if (isTrue()) {
            v = true;
            return true;
        }
        if (isFalse()) {
            v = false;
            return true;
        }
        
        return false;
    }

    inline bool TiValue::getBoolean() const
    {
        ASSERT(isBoolean());
        return tag() == TrueTag;
    }

    inline double TiValue::uncheckedGetNumber() const
    {
        ASSERT(isNumber());
        return isInt32() ? asInt32() : asDouble();
    }

    ALWAYS_INLINE TiValue TiValue::toJSNumber(TiExcState* exec) const
    {
        return isNumber() ? asValue() : jsNumber(exec, this->toNumber(exec));
    }

    inline bool TiValue::getNumber(double& result) const
    {
        if (isInt32()) {
            result = asInt32();
            return true;
        }
        if (isDouble()) {
            result = asDouble();
            return true;
        }
        return false;
    }

#else // USE(JSVALUE32_64)

    // TiValue member functions.
    inline EncodedTiValue TiValue::encode(TiValue value)
    {
        return reinterpret_cast<EncodedTiValue>(value.m_ptr);
    }

    inline TiValue TiValue::decode(EncodedTiValue ptr)
    {
        return TiValue(reinterpret_cast<TiCell*>(ptr));
    }

    inline TiValue TiValue::makeImmediate(intptr_t value)
    {
        return TiValue(reinterpret_cast<TiCell*>(value));
    }

    inline intptr_t TiValue::immediateValue()
    {
        return reinterpret_cast<intptr_t>(m_ptr);
    }
    
    // 0x0 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x0, which is in the (invalid) zero page.
    inline TiValue::TiValue()
        : m_ptr(0)
    {
    }

    // 0x4 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x4, which is in the (invalid) zero page.
    inline TiValue::TiValue(HashTableDeletedValueTag)
        : m_ptr(reinterpret_cast<TiCell*>(0x4))
    {
    }

    inline TiValue::TiValue(TiCell* ptr)
        : m_ptr(ptr)
    {
    }

    inline TiValue::TiValue(const TiCell* ptr)
        : m_ptr(const_cast<TiCell*>(ptr))
    {
    }

    inline TiValue::operator bool() const
    {
        return m_ptr;
    }

    inline bool TiValue::operator==(const TiValue& other) const
    {
        return m_ptr == other.m_ptr;
    }

    inline bool TiValue::operator!=(const TiValue& other) const
    {
        return m_ptr != other.m_ptr;
    }

    inline bool TiValue::isUndefined() const
    {
        return asValue() == jsUndefined();
    }

    inline bool TiValue::isNull() const
    {
        return asValue() == jsNull();
    }
#endif // USE(JSVALUE32_64)

} // namespace TI

#endif // TiValue_h
