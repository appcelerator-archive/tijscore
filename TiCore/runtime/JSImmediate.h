/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
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

#ifndef JSImmediate_h
#define JSImmediate_h

#if !USE(JSVALUE32_64)

#include <wtf/Assertions.h>
#include <wtf/AlwaysInline.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include "TiValue.h"
#include <limits>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

namespace TI {

    class TiExcState;
    class TiCell;
    class JSFastMath;
    class TiGlobalData;
    class TiObject;
    class UString;

#if USE(JSVALUE64)
    inline intptr_t reinterpretDoubleToIntptr(double value)
    {
        return WTI::bitwise_cast<intptr_t>(value);
    }

    inline double reinterpretIntptrToDouble(intptr_t value)
    {
        return WTI::bitwise_cast<double>(value);
    }
#endif

    /*
     * A TiValue* is either a pointer to a cell (a heap-allocated object) or an immediate (a type-tagged 
     * value masquerading as a pointer). The low two bits in a TiValue* are available for type tagging
     * because allocator alignment guarantees they will be 00 in cell pointers.
     *
     * For example, on a 32 bit system:
     *
     * TiCell*:             XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX                     00
     *                      [ high 30 bits: pointer address ]  [ low 2 bits -- always 0 ]
     * JSImmediate:         XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX                     TT
     *                      [ high 30 bits: 'payload' ]             [ low 2 bits -- tag ]
     *
     * Where the bottom two bits are non-zero they either indicate that the immediate is a 31 bit signed
     * integer, or they mark the value as being an immediate of a type other than integer, with a secondary
     * tag used to indicate the exact type.
     *
     * Where the lowest bit is set (TT is equal to 01 or 11) the high 31 bits form a 31 bit signed int value.
     * Where TT is equal to 10 this indicates this is a type of immediate other than an integer, and the next
     * two bits will form an extended tag.
     *
     * 31 bit signed int:   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX                     X1
     *                      [ high 30 bits of the value ]      [ high bit part of value ]
     * Other:               YYYYYYYYYYYYYYYYYYYYYYYYYYYY      ZZ               10
     *                      [ extended 'payload' ]  [  extended tag  ]  [  tag 'other'  ]
     *
     * Where the first bit of the extended tag is set this flags the value as being a boolean, and the following
     * bit would flag the value as undefined.  If neither bits are set, the value is null.
     *
     * Other:               YYYYYYYYYYYYYYYYYYYYYYYYYYYY      UB               10
     *                      [ extended 'payload' ]  [ undefined | bool ]  [ tag 'other' ]
     *
     * For boolean value the lowest bit in the payload holds the value of the bool, all remaining bits are zero.
     * For undefined or null immediates the payload is zero.
     *
     * Boolean:             000000000000000000000000000V      01               10
     *                      [ boolean value ]              [ bool ]       [ tag 'other' ]
     * Undefined:           0000000000000000000000000000      10               10
     *                      [ zero ]                    [ undefined ]     [ tag 'other' ]
     * Null:                0000000000000000000000000000      00               10
     *                      [ zero ]                       [ zero ]       [ tag 'other' ]
     */

    /*
     * On 64-bit platforms, we support an alternative encoding form for immediates, if
     * USE(JSVALUE64) is defined.  When this format is used, double precision
     * floating point values may also be encoded as JSImmediates.
     *
     * The encoding makes use of unused NaN space in the IEEE754 representation.  Any value
     * with the top 13 bits set represents a QNaN (with the sign bit set).  QNaN values
     * can encode a 51-bit payload.  Hardware produced and C-library payloads typically
     * have a payload of zero.  We assume that non-zero payloads are available to encode
     * pointer and integer values.  Since any 64-bit bit pattern where the top 15 bits are
     * all set represents a NaN with a non-zero payload, we can use this space in the NaN
     * ranges to encode other values (however there are also other ranges of NaN space that
     * could have been selected).  This range of NaN space is represented by 64-bit numbers
     * begining with the 16-bit hex patterns 0xFFFE and 0xFFFF - we rely on the fact that no
     * valid double-precision numbers will begin fall in these ranges.
     *
     * The scheme we have implemented encodes double precision values by adding 2^48 to the
     * 64-bit integer representation of the number.  After this manipulation, no encoded
     * double-precision value will begin with the pattern 0x0000 or 0xFFFF.
     *
     * The top 16-bits denote the type of the encoded JSImmediate:
     *
     * Pointer: 0000:PPPP:PPPP:PPPP
     *          0001:****:****:****
     * Double:{         ...
     *          FFFE:****:****:****
     * Integer: FFFF:0000:IIII:IIII
     *
     * 32-bit signed integers are marked with the 16-bit tag 0xFFFF.  The tag 0x0000
     * denotes a pointer, or another form of tagged immediate.  Boolean, null and undefined
     * values are encoded in the same manner as the default format.
     */

    class JSImmediate {
    private:
        friend class JIT;
        friend class TiValue;
        friend class JSFastMath;
        friend class JSInterfaceJIT;
        friend class SpecializedThunkJIT;
        friend TiValue jsNumber(TiExcState* exec, double d);
        friend TiValue jsNumber(TiExcState*, char i);
        friend TiValue jsNumber(TiExcState*, unsigned char i);
        friend TiValue jsNumber(TiExcState*, short i);
        friend TiValue jsNumber(TiExcState*, unsigned short i);
        friend TiValue jsNumber(TiExcState* exec, int i);
        friend TiValue jsNumber(TiExcState* exec, unsigned i);
        friend TiValue jsNumber(TiExcState* exec, long i);
        friend TiValue jsNumber(TiExcState* exec, unsigned long i);
        friend TiValue jsNumber(TiExcState* exec, long long i);
        friend TiValue jsNumber(TiExcState* exec, unsigned long long i);
        friend TiValue jsNumber(TiGlobalData* globalData, double d);
        friend TiValue jsNumber(TiGlobalData* globalData, short i);
        friend TiValue jsNumber(TiGlobalData* globalData, unsigned short i);
        friend TiValue jsNumber(TiGlobalData* globalData, int i);
        friend TiValue jsNumber(TiGlobalData* globalData, unsigned i);
        friend TiValue jsNumber(TiGlobalData* globalData, long i);
        friend TiValue jsNumber(TiGlobalData* globalData, unsigned long i);
        friend TiValue jsNumber(TiGlobalData* globalData, long long i);
        friend TiValue jsNumber(TiGlobalData* globalData, unsigned long long i);

#if USE(JSVALUE64)
        // If all bits in the mask are set, this indicates an integer number,
        // if any but not all are set this value is a double precision number.
        static const intptr_t TagTypeNumber = 0xffff000000000000ll;
        // This value is 2^48, used to encode doubles such that the encoded value will begin
        // with a 16-bit pattern within the range 0x0001..0xFFFE.
        static const intptr_t DoubleEncodeOffset = 0x1000000000000ll;
#elif USE(JSVALUE32)
        static const intptr_t TagTypeNumber = 0x1; // bottom bit set indicates integer, this dominates the following bit
#endif
        static const intptr_t TagBitTypeOther   = 0x2; // second bit set indicates immediate other than an integer
        static const intptr_t TagMask           = TagTypeNumber | TagBitTypeOther;

        static const intptr_t ExtendedTagMask         = 0xC; // extended tag holds a further two bits
        static const intptr_t ExtendedTagBitBool      = 0x4;
        static const intptr_t ExtendedTagBitUndefined = 0x8;

        static const intptr_t FullTagTypeMask      = TagMask | ExtendedTagMask;
        static const intptr_t FullTagTypeBool      = TagBitTypeOther | ExtendedTagBitBool;
        static const intptr_t FullTagTypeUndefined = TagBitTypeOther | ExtendedTagBitUndefined;
        static const intptr_t FullTagTypeNull      = TagBitTypeOther;

#if USE(JSVALUE64)
        static const int32_t IntegerPayloadShift  = 0;
#else
        static const int32_t IntegerPayloadShift  = 1;
#endif
        static const int32_t ExtendedPayloadShift = 4;

        static const intptr_t ExtendedPayloadBitBoolValue = 1 << ExtendedPayloadShift;

        static const int32_t signBit = 0x80000000;
 
        static ALWAYS_INLINE bool isImmediate(TiValue v)
        {
            return rawValue(v) & TagMask;
        }
        
        static ALWAYS_INLINE bool isNumber(TiValue v)
        {
            return rawValue(v) & TagTypeNumber;
        }

        static ALWAYS_INLINE bool isIntegerNumber(TiValue v)
        {
#if USE(JSVALUE64)
            return (rawValue(v) & TagTypeNumber) == TagTypeNumber;
#else
            return isNumber(v);
#endif
        }

#if USE(JSVALUE64)
        static ALWAYS_INLINE bool isDouble(TiValue v)
        {
            return isNumber(v) && !isIntegerNumber(v);
        }
#endif

        static ALWAYS_INLINE bool isPositiveIntegerNumber(TiValue v)
        {
            // A single mask to check for the sign bit and the number tag all at once.
            return (rawValue(v) & (signBit | TagTypeNumber)) == TagTypeNumber;
        }
        
        static ALWAYS_INLINE bool isBoolean(TiValue v)
        {
            return (rawValue(v) & FullTagTypeMask) == FullTagTypeBool;
        }
        
        static ALWAYS_INLINE bool isUndefinedOrNull(TiValue v)
        {
            // Undefined and null share the same value, bar the 'undefined' bit in the extended tag.
            return (rawValue(v) & ~ExtendedTagBitUndefined) == FullTagTypeNull;
        }

        static TiValue from(char);
        static TiValue from(signed char);
        static TiValue from(unsigned char);
        static TiValue from(short);
        static TiValue from(unsigned short);
        static TiValue from(int);
        static TiValue from(unsigned);
        static TiValue from(long);
        static TiValue from(unsigned long);
        static TiValue from(long long);
        static TiValue from(unsigned long long);
        static TiValue from(double);

        static ALWAYS_INLINE bool isEitherImmediate(TiValue v1, TiValue v2)
        {
            return (rawValue(v1) | rawValue(v2)) & TagMask;
        }

        static ALWAYS_INLINE bool areBothImmediate(TiValue v1, TiValue v2)
        {
            return isImmediate(v1) & isImmediate(v2);
        }

        static ALWAYS_INLINE bool areBothImmediateIntegerNumbers(TiValue v1, TiValue v2)
        {
#if USE(JSVALUE64)
            return (rawValue(v1) & rawValue(v2) & TagTypeNumber) == TagTypeNumber;
#else
            return rawValue(v1) & rawValue(v2) & TagTypeNumber;
#endif
        }

        static double toDouble(TiValue);
        static bool toBoolean(TiValue);

        static bool getUInt32(TiValue, uint32_t&);
        static bool getTruncatedInt32(TiValue, int32_t&);
        static bool getTruncatedUInt32(TiValue, uint32_t&);

        static int32_t getTruncatedInt32(TiValue);
        static uint32_t getTruncatedUInt32(TiValue);

        static TiValue trueImmediate();
        static TiValue falseImmediate();
        static TiValue undefinedImmediate();
        static TiValue nullImmediate();
        static TiValue zeroImmediate();
        static TiValue oneImmediate();

    private:
#if USE(JSVALUE64)
        static const int minImmediateInt = ((-INT_MAX) - 1);
        static const int maxImmediateInt = INT_MAX;
#else
        static const int minImmediateInt = ((-INT_MAX) - 1) >> IntegerPayloadShift;
        static const int maxImmediateInt = INT_MAX >> IntegerPayloadShift;
#endif
        static const unsigned maxImmediateUInt = maxImmediateInt;

        static ALWAYS_INLINE TiValue makeValue(intptr_t integer)
        {
            return TiValue::makeImmediate(integer);
        }

        // With USE(JSVALUE64) we want the argument to be zero extended, so the
        // integer doesn't interfere with the tag bits in the upper word.  In the default encoding,
        // if intptr_t id larger then int32_t we sign extend the value through the upper word.
#if USE(JSVALUE64)
        static ALWAYS_INLINE TiValue makeInt(uint32_t value)
#else
        static ALWAYS_INLINE TiValue makeInt(int32_t value)
#endif
        {
            return makeValue((static_cast<intptr_t>(value) << IntegerPayloadShift) | TagTypeNumber);
        }
        
#if USE(JSVALUE64)
        static ALWAYS_INLINE TiValue makeDouble(double value)
        {
            return makeValue(reinterpretDoubleToIntptr(value) + DoubleEncodeOffset);
        }
#endif
        
        static ALWAYS_INLINE TiValue makeBool(bool b)
        {
            return makeValue((static_cast<intptr_t>(b) << ExtendedPayloadShift) | FullTagTypeBool);
        }
        
        static ALWAYS_INLINE TiValue makeUndefined()
        {
            return makeValue(FullTagTypeUndefined);
        }
        
        static ALWAYS_INLINE TiValue makeNull()
        {
            return makeValue(FullTagTypeNull);
        }

        template<typename T>
        static TiValue fromNumberOutsideIntegerRange(T);

#if USE(JSVALUE64)
        static ALWAYS_INLINE double doubleValue(TiValue v)
        {
            return reinterpretIntptrToDouble(rawValue(v) - DoubleEncodeOffset);
        }
#endif

        static ALWAYS_INLINE int32_t intValue(TiValue v)
        {
            return static_cast<int32_t>(rawValue(v) >> IntegerPayloadShift);
        }
        
        static ALWAYS_INLINE uint32_t uintValue(TiValue v)
        {
            return static_cast<uint32_t>(rawValue(v) >> IntegerPayloadShift);
        }
        
        static ALWAYS_INLINE bool boolValue(TiValue v)
        {
            return rawValue(v) & ExtendedPayloadBitBoolValue;
        }
        
        static ALWAYS_INLINE intptr_t rawValue(TiValue v)
        {
            return v.immediateValue();
        }
    };

    ALWAYS_INLINE TiValue JSImmediate::trueImmediate() { return makeBool(true); }
    ALWAYS_INLINE TiValue JSImmediate::falseImmediate() { return makeBool(false); }
    ALWAYS_INLINE TiValue JSImmediate::undefinedImmediate() { return makeUndefined(); }
    ALWAYS_INLINE TiValue JSImmediate::nullImmediate() { return makeNull(); }
    ALWAYS_INLINE TiValue JSImmediate::zeroImmediate() { return makeInt(0); }
    ALWAYS_INLINE TiValue JSImmediate::oneImmediate() { return makeInt(1); }

#if USE(JSVALUE64)
    inline bool doubleToBoolean(double value)
    {
        return value < 0.0 || value > 0.0;
    }

    ALWAYS_INLINE bool JSImmediate::toBoolean(TiValue v)
    {
        ASSERT(isImmediate(v));
        return isNumber(v) ? isIntegerNumber(v) ? v != zeroImmediate()
            : doubleToBoolean(doubleValue(v)) : v == trueImmediate();
    }
#else
    ALWAYS_INLINE bool JSImmediate::toBoolean(TiValue v)
    {
        ASSERT(isImmediate(v));
        return isIntegerNumber(v) ? v != zeroImmediate() : v == trueImmediate();
    }
#endif

    ALWAYS_INLINE uint32_t JSImmediate::getTruncatedUInt32(TiValue v)
    {
        // FIXME: should probably be asserting isPositiveIntegerNumber here.
        ASSERT(isIntegerNumber(v));
        return intValue(v);
    }

#if USE(JSVALUE64)
    template<typename T>
    inline TiValue JSImmediate::fromNumberOutsideIntegerRange(T value)
    {
        return makeDouble(static_cast<double>(value));
    }
#else
    template<typename T>
    inline TiValue JSImmediate::fromNumberOutsideIntegerRange(T)
    {
        return TiValue();
    }
#endif

    ALWAYS_INLINE TiValue JSImmediate::from(char i)
    {
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(signed char i)
    {
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(unsigned char i)
    {
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(short i)
    {
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(unsigned short i)
    {
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(int i)
    {
#if !USE(JSVALUE64)
        if ((i < minImmediateInt) | (i > maxImmediateInt))
            return fromNumberOutsideIntegerRange(i);
#endif
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(unsigned i)
    {
        if (i > maxImmediateUInt)
            return fromNumberOutsideIntegerRange(i);
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(long i)
    {
        if ((i < minImmediateInt) | (i > maxImmediateInt))
            return fromNumberOutsideIntegerRange(i);
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(unsigned long i)
    {
        if (i > maxImmediateUInt)
            return fromNumberOutsideIntegerRange(i);
        return makeInt(i);
    }

    ALWAYS_INLINE TiValue JSImmediate::from(long long i)
    {
        if ((i < minImmediateInt) | (i > maxImmediateInt))
            return TiValue();
        return makeInt(static_cast<intptr_t>(i));
    }

    ALWAYS_INLINE TiValue JSImmediate::from(unsigned long long i)
    {
        if (i > maxImmediateUInt)
            return fromNumberOutsideIntegerRange(i);
        return makeInt(static_cast<intptr_t>(i));
    }

    ALWAYS_INLINE TiValue JSImmediate::from(double d)
    {
        const int intVal = static_cast<int>(d);

        // Check for data loss from conversion to int.
        if (intVal != d || (!intVal && signbit(d)))
            return fromNumberOutsideIntegerRange(d);

        return from(intVal);
    }

    ALWAYS_INLINE int32_t JSImmediate::getTruncatedInt32(TiValue v)
    {
        ASSERT(isIntegerNumber(v));
        return intValue(v);
    }

    ALWAYS_INLINE double JSImmediate::toDouble(TiValue v)
    {
        ASSERT(isImmediate(v));

        if (isIntegerNumber(v))
            return intValue(v);

#if USE(JSVALUE64)
        if (isNumber(v)) {
            ASSERT(isDouble(v));
            return doubleValue(v);
        }
#else
        ASSERT(!isNumber(v));
#endif

        if (rawValue(v) == FullTagTypeUndefined)
            return nonInlineNaN();

        ASSERT(JSImmediate::isBoolean(v) || (v == JSImmediate::nullImmediate()));
        return rawValue(v) >> ExtendedPayloadShift;
    }

    ALWAYS_INLINE bool JSImmediate::getUInt32(TiValue v, uint32_t& i)
    {
        i = uintValue(v);
        return isPositiveIntegerNumber(v);
    }

    ALWAYS_INLINE bool JSImmediate::getTruncatedInt32(TiValue v, int32_t& i)
    {
        i = intValue(v);
        return isIntegerNumber(v);
    }

    ALWAYS_INLINE bool JSImmediate::getTruncatedUInt32(TiValue v, uint32_t& i)
    {
        return getUInt32(v, i);
    }

    inline TiValue::TiValue(JSNullTag)
    {
        *this = JSImmediate::nullImmediate();
    }
    
    inline TiValue::TiValue(JSUndefinedTag)
    {
        *this = JSImmediate::undefinedImmediate();
    }

    inline TiValue::TiValue(JSTrueTag)
    {
        *this = JSImmediate::trueImmediate();
    }

    inline TiValue::TiValue(JSFalseTag)
    {
        *this = JSImmediate::falseImmediate();
    }

    inline bool TiValue::isUndefinedOrNull() const
    {
        return JSImmediate::isUndefinedOrNull(asValue());
    }

    inline bool TiValue::isBoolean() const
    {
        return JSImmediate::isBoolean(asValue());
    }

    inline bool TiValue::isTrue() const
    {
        return asValue() == JSImmediate::trueImmediate();
    }

    inline bool TiValue::isFalse() const
    {
        return asValue() == JSImmediate::falseImmediate();
    }

    inline bool TiValue::getBoolean(bool& v) const
    {
        if (JSImmediate::isBoolean(asValue())) {
            v = JSImmediate::toBoolean(asValue());
            return true;
        }
        
        return false;
    }

    inline bool TiValue::getBoolean() const
    {
        return asValue() == jsBoolean(true);
    }

    inline bool TiValue::isCell() const
    {
        return !JSImmediate::isImmediate(asValue());
    }

    inline bool TiValue::isInt32() const
    {
        return JSImmediate::isIntegerNumber(asValue());
    }

    inline int32_t TiValue::asInt32() const
    {
        ASSERT(isInt32());
        return JSImmediate::getTruncatedInt32(asValue());
    }

    inline bool TiValue::isUInt32() const
    {
        return JSImmediate::isPositiveIntegerNumber(asValue());
    }

    inline uint32_t TiValue::asUInt32() const
    {
        ASSERT(isUInt32());
        return JSImmediate::getTruncatedUInt32(asValue());
    }

    class JSFastMath {
    public:
        static ALWAYS_INLINE bool canDoFastBitwiseOperations(TiValue v1, TiValue v2)
        {
            return JSImmediate::areBothImmediateIntegerNumbers(v1, v2);
        }

        static ALWAYS_INLINE TiValue equal(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastBitwiseOperations(v1, v2));
            return jsBoolean(v1 == v2);
        }

        static ALWAYS_INLINE TiValue notEqual(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastBitwiseOperations(v1, v2));
            return jsBoolean(v1 != v2);
        }

        static ALWAYS_INLINE TiValue andImmediateNumbers(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastBitwiseOperations(v1, v2));
            return JSImmediate::makeValue(JSImmediate::rawValue(v1) & JSImmediate::rawValue(v2));
        }

        static ALWAYS_INLINE TiValue xorImmediateNumbers(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastBitwiseOperations(v1, v2));
            return JSImmediate::makeValue((JSImmediate::rawValue(v1) ^ JSImmediate::rawValue(v2)) | JSImmediate::TagTypeNumber);
        }

        static ALWAYS_INLINE TiValue orImmediateNumbers(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastBitwiseOperations(v1, v2));
            return JSImmediate::makeValue(JSImmediate::rawValue(v1) | JSImmediate::rawValue(v2));
        }

        static ALWAYS_INLINE bool canDoFastRshift(TiValue v1, TiValue v2)
        {
            return JSImmediate::areBothImmediateIntegerNumbers(v1, v2);
        }

        static ALWAYS_INLINE bool canDoFastUrshift(TiValue v1, TiValue v2)
        {
            return JSImmediate::areBothImmediateIntegerNumbers(v1, v2) && !(JSImmediate::rawValue(v1) & JSImmediate::signBit);
        }

        static ALWAYS_INLINE TiValue rightShiftImmediateNumbers(TiValue val, TiValue shift)
        {
            ASSERT(canDoFastRshift(val, shift) || canDoFastUrshift(val, shift));
#if USE(JSVALUE64)
            return JSImmediate::makeValue(static_cast<intptr_t>(static_cast<uint32_t>(static_cast<int32_t>(JSImmediate::rawValue(val)) >> ((JSImmediate::rawValue(shift) >> JSImmediate::IntegerPayloadShift) & 0x1f))) | JSImmediate::TagTypeNumber);
#else
            return JSImmediate::makeValue((JSImmediate::rawValue(val) >> ((JSImmediate::rawValue(shift) >> JSImmediate::IntegerPayloadShift) & 0x1f)) | JSImmediate::TagTypeNumber);
#endif
        }

        static ALWAYS_INLINE bool canDoFastAdditiveOperations(TiValue v)
        {
            // Number is non-negative and an operation involving two of these can't overflow.
            // Checking for allowed negative numbers takes more time than it's worth on SunSpider.
            return (JSImmediate::rawValue(v) & (JSImmediate::TagTypeNumber + (JSImmediate::signBit | (JSImmediate::signBit >> 1)))) == JSImmediate::TagTypeNumber;
        }

        static ALWAYS_INLINE bool canDoFastAdditiveOperations(TiValue v1, TiValue v2)
        {
            // Number is non-negative and an operation involving two of these can't overflow.
            // Checking for allowed negative numbers takes more time than it's worth on SunSpider.
            return canDoFastAdditiveOperations(v1) && canDoFastAdditiveOperations(v2);
        }

        static ALWAYS_INLINE TiValue addImmediateNumbers(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastAdditiveOperations(v1, v2));
            return JSImmediate::makeValue(JSImmediate::rawValue(v1) + JSImmediate::rawValue(v2) - JSImmediate::TagTypeNumber);
        }

        static ALWAYS_INLINE TiValue subImmediateNumbers(TiValue v1, TiValue v2)
        {
            ASSERT(canDoFastAdditiveOperations(v1, v2));
            return JSImmediate::makeValue(JSImmediate::rawValue(v1) - JSImmediate::rawValue(v2) + JSImmediate::TagTypeNumber);
        }

        static ALWAYS_INLINE TiValue incImmediateNumber(TiValue v)
        {
            ASSERT(canDoFastAdditiveOperations(v));
            return JSImmediate::makeValue(JSImmediate::rawValue(v) + (1 << JSImmediate::IntegerPayloadShift));
        }

        static ALWAYS_INLINE TiValue decImmediateNumber(TiValue v)
        {
            ASSERT(canDoFastAdditiveOperations(v));
            return JSImmediate::makeValue(JSImmediate::rawValue(v) - (1 << JSImmediate::IntegerPayloadShift));
        }
    };

} // namespace TI

#endif // !USE(JSVALUE32_64)

#endif // JSImmediate_h
