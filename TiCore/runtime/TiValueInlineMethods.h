/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef TiValueInlineMethods_h
#define TiValueInlineMethods_h

#include "TiValue.h"

namespace TI {

    ALWAYS_INLINE int32_t TiValue::toInt32(TiExcState* exec) const
    {
        if (isInt32())
            return asInt32();
        return TI::toInt32(toNumber(exec));
    }

    inline uint32_t TiValue::toUInt32(TiExcState* exec) const
    {
        // See comment on TI::toUInt32, above.
        return toInt32(exec);
    }

    inline bool TiValue::isUInt32() const
    {
        return isInt32() && asInt32() >= 0;
    }

    inline uint32_t TiValue::asUInt32() const
    {
        ASSERT(isUInt32());
        return asInt32();
    }

    inline double TiValue::uncheckedGetNumber() const
    {
        ASSERT(isNumber());
        return isInt32() ? asInt32() : asDouble();
    }

    ALWAYS_INLINE TiValue TiValue::toJSNumber(TiExcState* exec) const
    {
        return isNumber() ? asValue() : jsNumber(this->toNumber(exec));
    }

    inline TiValue jsNaN()
    {
        return TiValue(nonInlineNaN());
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

    inline TiValue::TiValue(char i)
    {
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(unsigned char i)
    {
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(short i)
    {
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(unsigned short i)
    {
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(unsigned i)
    {
        if (static_cast<int32_t>(i) < 0) {
            *this = TiValue(EncodeAsDouble, static_cast<double>(i));
            return;
        }
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(long i)
    {
        if (static_cast<int32_t>(i) != i) {
            *this = TiValue(EncodeAsDouble, static_cast<double>(i));
            return;
        }
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(unsigned long i)
    {
        if (static_cast<uint32_t>(i) != i) {
            *this = TiValue(EncodeAsDouble, static_cast<double>(i));
            return;
        }
        *this = TiValue(static_cast<uint32_t>(i));
    }

    inline TiValue::TiValue(long long i)
    {
        if (static_cast<int32_t>(i) != i) {
            *this = TiValue(EncodeAsDouble, static_cast<double>(i));
            return;
        }
        *this = TiValue(static_cast<int32_t>(i));
    }

    inline TiValue::TiValue(unsigned long long i)
    {
        if (static_cast<uint32_t>(i) != i) {
            *this = TiValue(EncodeAsDouble, static_cast<double>(i));
            return;
        }
        *this = TiValue(static_cast<uint32_t>(i));
    }

    inline TiValue::TiValue(double d)
    {
        const int32_t asInt32 = static_cast<int32_t>(d);
        if (asInt32 != d || (!asInt32 && signbit(d))) { // true for -0.0
            *this = TiValue(EncodeAsDouble, d);
            return;
        }
        *this = TiValue(static_cast<int32_t>(d));
    }

#if USE(JSVALUE32_64)
    inline EncodedTiValue TiValue::encode(TiValue value)
    {
        return value.u.asInt64;
    }

    inline TiValue TiValue::decode(EncodedTiValue encodedTiValue)
    {
        TiValue v;
        v.u.asInt64 = encodedTiValue;
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
        u.asBits.tag = BooleanTag;
        u.asBits.payload = 1;
    }
    
    inline TiValue::TiValue(JSFalseTag)
    {
        u.asBits.tag = BooleanTag;
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
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie());
#endif
    }

    inline TiValue::TiValue(const TiCell* ptr)
    {
        if (ptr)
            u.asBits.tag = CellTag;
        else
            u.asBits.tag = EmptyValueTag;
        u.asBits.payload = reinterpret_cast<int32_t>(const_cast<TiCell*>(ptr));
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie());
#endif
    }

    inline TiValue::operator bool() const
    {
        ASSERT(tag() != DeletedValueTag);
        return tag() != EmptyValueTag;
    }

    inline bool TiValue::operator==(const TiValue& other) const
    {
        return u.asInt64 == other.u.asInt64;
    }

    inline bool TiValue::operator!=(const TiValue& other) const
    {
        return u.asInt64 != other.u.asInt64;
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

    inline bool TiValue::isDouble() const
    {
        return tag() < LowestTag;
    }

    inline bool TiValue::isTrue() const
    {
        return tag() == BooleanTag && payload();
    }

    inline bool TiValue::isFalse() const
    {
        return tag() == BooleanTag && !payload();
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

    ALWAYS_INLINE TiValue::TiValue(EncodeAsDoubleTag, double d)
    {
        u.asDouble = d;
    }

    inline TiValue::TiValue(int i)
    {
        u.asBits.tag = Int32Tag;
        u.asBits.payload = i;
    }

    inline bool TiValue::isNumber() const
    {
        return isInt32() || isDouble();
    }

    inline bool TiValue::isBoolean() const
    {
        return isTrue() || isFalse();
    }

    inline bool TiValue::getBoolean() const
    {
        ASSERT(isBoolean());
        return payload();
    }

#else // USE(JSVALUE32_64)

    // TiValue member functions.
    inline EncodedTiValue TiValue::encode(TiValue value)
    {
        return value.u.ptr;
    }

    inline TiValue TiValue::decode(EncodedTiValue ptr)
    {
        return TiValue(reinterpret_cast<TiCell*>(ptr));
    }

    // 0x0 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x0, which is in the (invalid) zero page.
    inline TiValue::TiValue()
    {
        u.asInt64 = ValueEmpty;
    }

    // 0x4 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x4, which is in the (invalid) zero page.
    inline TiValue::TiValue(HashTableDeletedValueTag)
    {
        u.asInt64 = ValueDeleted;
    }

    inline TiValue::TiValue(TiCell* ptr)
    {
        u.ptr = ptr;
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie());
#endif
    }

    inline TiValue::TiValue(const TiCell* ptr)
    {
        u.ptr = const_cast<TiCell*>(ptr);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie());
#endif
    }

    inline TiValue::operator bool() const
    {
        return u.ptr;
    }

    inline bool TiValue::operator==(const TiValue& other) const
    {
        return u.ptr == other.u.ptr;
    }

    inline bool TiValue::operator!=(const TiValue& other) const
    {
        return u.ptr != other.u.ptr;
    }

    inline bool TiValue::isUndefined() const
    {
        return asValue() == jsUndefined();
    }

    inline bool TiValue::isNull() const
    {
        return asValue() == jsNull();
    }

    inline bool TiValue::isTrue() const
    {
        return asValue() == TiValue(JSTrue);
    }

    inline bool TiValue::isFalse() const
    {
        return asValue() == TiValue(JSFalse);
    }

    inline bool TiValue::getBoolean() const
    {
        ASSERT(asValue() == jsBoolean(true) || asValue() == jsBoolean(false));
        return asValue() == jsBoolean(true);
    }

    inline int32_t TiValue::asInt32() const
    {
        ASSERT(isInt32());
        return static_cast<int32_t>(u.asInt64);
    }

    inline bool TiValue::isDouble() const
    {
        return isNumber() && !isInt32();
    }

    inline TiValue::TiValue(JSNullTag)
    {
        u.asInt64 = ValueNull;
    }
    
    inline TiValue::TiValue(JSUndefinedTag)
    {
        u.asInt64 = ValueUndefined;
    }

    inline TiValue::TiValue(JSTrueTag)
    {
        u.asInt64 = ValueTrue;
    }

    inline TiValue::TiValue(JSFalseTag)
    {
        u.asInt64 = ValueFalse;
    }

    inline bool TiValue::isUndefinedOrNull() const
    {
        // Undefined and null share the same value, bar the 'undefined' bit in the extended tag.
        return (u.asInt64 & ~TagBitUndefined) == ValueNull;
    }

    inline bool TiValue::isBoolean() const
    {
        return (u.asInt64 & ~1) == ValueFalse;
    }

    inline bool TiValue::isCell() const
    {
        return !(u.asInt64 & TagMask);
    }

    inline bool TiValue::isInt32() const
    {
        return (u.asInt64 & TagTypeNumber) == TagTypeNumber;
    }

    inline intptr_t reinterpretDoubleToIntptr(double value)
    {
        return bitwise_cast<intptr_t>(value);
    }
    inline double reinterpretIntptrToDouble(intptr_t value)
    {
        return bitwise_cast<double>(value);
    }

    ALWAYS_INLINE TiValue::TiValue(EncodeAsDoubleTag, double d)
    {
        u.asInt64 = reinterpretDoubleToIntptr(d) + DoubleEncodeOffset;
    }

    inline TiValue::TiValue(int i)
    {
        u.asInt64 = TagTypeNumber | static_cast<uint32_t>(i);
    }

    inline double TiValue::asDouble() const
    {
        return reinterpretIntptrToDouble(u.asInt64 - DoubleEncodeOffset);
    }

    inline bool TiValue::isNumber() const
    {
        return u.asInt64 & TagTypeNumber;
    }

    ALWAYS_INLINE TiCell* TiValue::asCell() const
    {
        ASSERT(isCell());
        return u.ptr;
    }

#endif // USE(JSVALUE64)

} // namespace TI

#endif // TiValueInlineMethods_h
