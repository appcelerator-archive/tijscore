/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef TiValueInlines_h
#define TiValueInlines_h

#include "InternalFunction.h"
#include "JSCTiValue.h"
#include "JSCellInlines.h"
#include "JSFunction.h"

namespace TI {

ALWAYS_INLINE int32_t TiValue::toInt32(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    return TI::toInt32(toNumber(exec));
}

inline uint32_t TiValue::toUInt32(ExecState* exec) const
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

inline double TiValue::asNumber() const
{
    ASSERT(isNumber());
    return isInt32() ? asInt32() : asDouble();
}

inline TiValue jsNaN()
{
    return TiValue(QNaN);
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
    if (asInt32 != d || (!asInt32 && std::signbit(d))) { // true for -0.0
        *this = TiValue(EncodeAsDouble, d);
        return;
    }
    *this = TiValue(static_cast<int32_t>(d));
}

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

#if USE(JSVALUE32_64)
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

inline TiValue::TiValue(JSCell* ptr)
{
    if (ptr)
        u.asBits.tag = CellTag;
    else
        u.asBits.tag = EmptyValueTag;
    u.asBits.payload = reinterpret_cast<int32_t>(ptr);
}

inline TiValue::TiValue(const JSCell* ptr)
{
    if (ptr)
        u.asBits.tag = CellTag;
    else
        u.asBits.tag = EmptyValueTag;
    u.asBits.payload = reinterpret_cast<int32_t>(const_cast<JSCell*>(ptr));
}

inline TiValue::operator UnspecifiedBoolType*() const
{
    ASSERT(tag() != DeletedValueTag);
    return tag() != EmptyValueTag ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0;
}

inline bool TiValue::operator==(const TiValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

inline bool TiValue::operator!=(const TiValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline bool TiValue::isEmpty() const
{
    return tag() == EmptyValueTag;
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
    
ALWAYS_INLINE JSCell* TiValue::asCell() const
{
    ASSERT(isCell());
    return reinterpret_cast<JSCell*>(u.asBits.payload);
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

#if ENABLE(LLINT_C_LOOP)
inline TiValue::TiValue(int32_t tag, int32_t payload)
{
    u.asBits.tag = tag;
    u.asBits.payload = payload;
}
#endif

inline bool TiValue::isNumber() const
{
    return isInt32() || isDouble();
}

inline bool TiValue::isBoolean() const
{
    return isTrue() || isFalse();
}

inline bool TiValue::asBoolean() const
{
    ASSERT(isBoolean());
    return payload();
}

#else // !USE(JSVALUE32_64) i.e. USE(JSVALUE64)

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

inline TiValue::TiValue(JSCell* ptr)
{
    u.asInt64 = reinterpret_cast<uintptr_t>(ptr);
}

inline TiValue::TiValue(const JSCell* ptr)
{
    u.asInt64 = reinterpret_cast<uintptr_t>(const_cast<JSCell*>(ptr));
}

inline TiValue::operator UnspecifiedBoolType*() const
{
    return u.asInt64 ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0;
}

inline bool TiValue::operator==(const TiValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

inline bool TiValue::operator!=(const TiValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline bool TiValue::isEmpty() const
{
    return u.asInt64 == ValueEmpty;
}

inline bool TiValue::isUndefined() const
{
    return asValue() == TiValue(JSUndefined);
}

inline bool TiValue::isNull() const
{
    return asValue() == TiValue(JSNull);
}

inline bool TiValue::isTrue() const
{
    return asValue() == TiValue(JSTrue);
}

inline bool TiValue::isFalse() const
{
    return asValue() == TiValue(JSFalse);
}

inline bool TiValue::asBoolean() const
{
    ASSERT(isBoolean());
    return asValue() == TiValue(JSTrue);
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

inline int64_t reinterpretDoubleToInt64(double value)
{
    return bitwise_cast<int64_t>(value);
}
inline double reinterpretInt64ToDouble(int64_t value)
{
    return bitwise_cast<double>(value);
}

ALWAYS_INLINE TiValue::TiValue(EncodeAsDoubleTag, double d)
{
    u.asInt64 = reinterpretDoubleToInt64(d) + DoubleEncodeOffset;
}

inline TiValue::TiValue(int i)
{
    u.asInt64 = TagTypeNumber | static_cast<uint32_t>(i);
}

inline double TiValue::asDouble() const
{
    ASSERT(isDouble());
    return reinterpretInt64ToDouble(u.asInt64 - DoubleEncodeOffset);
}

inline bool TiValue::isNumber() const
{
    return u.asInt64 & TagTypeNumber;
}

ALWAYS_INLINE JSCell* TiValue::asCell() const
{
    ASSERT(isCell());
    return u.ptr;
}

#endif // USE(JSVALUE64)

inline bool TiValue::isMachineInt() const
{
    if (isInt32())
        return true;
    if (!isNumber())
        return false;
    double number = asDouble();
    if (number != number)
        return false;
    int64_t asInt64 = static_cast<int64_t>(number);
    if (asInt64 != number)
        return false;
    if (!asInt64 && std::signbit(number))
        return false;
    if (asInt64 >= (static_cast<int64_t>(1) << (numberOfInt52Bits - 1)))
        return false;
    if (asInt64 < -(static_cast<int64_t>(1) << (numberOfInt52Bits - 1)))
        return false;
    return true;
}

inline int64_t TiValue::asMachineInt() const
{
    ASSERT(isMachineInt());
    if (isInt32())
        return asInt32();
    return static_cast<int64_t>(asDouble());
}

inline bool TiValue::isString() const
{
    return isCell() && asCell()->isString();
}

inline bool TiValue::isPrimitive() const
{
    return !isCell() || asCell()->isString();
}

inline bool TiValue::isGetterSetter() const
{
    return isCell() && asCell()->isGetterSetter();
}

inline bool TiValue::isObject() const
{
    return isCell() && asCell()->isObject();
}

inline bool TiValue::getString(ExecState* exec, String& s) const
{
    return isCell() && asCell()->getString(exec, s);
}

inline String TiValue::getString(ExecState* exec) const
{
    return isCell() ? asCell()->getString(exec) : String();
}

template <typename Base> String HandleConverter<Base, Unknown>::getString(ExecState* exec) const
{
    return jsValue().getString(exec);
}

inline JSObject* TiValue::getObject() const
{
    return isCell() ? asCell()->getObject() : 0;
}

ALWAYS_INLINE bool TiValue::getUInt32(uint32_t& v) const
{
    if (isInt32()) {
        int32_t i = asInt32();
        v = static_cast<uint32_t>(i);
        return i >= 0;
    }
    if (isDouble()) {
        double d = asDouble();
        v = static_cast<uint32_t>(d);
        return v == d;
    }
    return false;
}

inline TiValue TiValue::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    return isCell() ? asCell()->toPrimitive(exec, preferredType) : asValue();
}

inline bool TiValue::getPrimitiveNumber(ExecState* exec, double& number, TiValue& value)
{
    if (isInt32()) {
        number = asInt32();
        value = *this;
        return true;
    }
    if (isDouble()) {
        number = asDouble();
        value = *this;
        return true;
    }
    if (isCell())
        return asCell()->getPrimitiveNumber(exec, number, value);
    if (isTrue()) {
        number = 1.0;
        value = *this;
        return true;
    }
    if (isFalse() || isNull()) {
        number = 0.0;
        value = *this;
        return true;
    }
    ASSERT(isUndefined());
    number = QNaN;
    value = *this;
    return true;
}

ALWAYS_INLINE double TiValue::toNumber(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    if (isDouble())
        return asDouble();
    return toNumberSlowCase(exec);
}

inline JSObject* TiValue::toObject(ExecState* exec) const
{
    return isCell() ? asCell()->toObject(exec, exec->lexicalGlobalObject()) : toObjectSlowCase(exec, exec->lexicalGlobalObject());
}

inline JSObject* TiValue::toObject(ExecState* exec, JSGlobalObject* globalObject) const
{
    return isCell() ? asCell()->toObject(exec, globalObject) : toObjectSlowCase(exec, globalObject);
}

inline bool TiValue::isFunction() const
{
    return isCell() && (asCell()->inherits(JSFunction::info()) || asCell()->inherits(InternalFunction::info()));
}

// this method is here to be after the inline declaration of JSCell::inherits
inline bool TiValue::inherits(const ClassInfo* classInfo) const
{
    return isCell() && asCell()->inherits(classInfo);
}

inline TiValue TiValue::toThis(ExecState* exec, ECMAMode ecmaMode) const
{
    return isCell() ? asCell()->methodTable()->toThis(asCell(), exec, ecmaMode) : toThisSlowCase(exec, ecmaMode);
}

inline TiValue TiValue::get(ExecState* exec, PropertyName propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline TiValue TiValue::get(ExecState* exec, PropertyName propertyName, PropertySlot& slot) const
{
    // If this is a primitive, we'll need to synthesize the prototype -
    // and if it's a string there are special properties to check first.
    JSObject* object;
    if (UNLIKELY(!isObject())) {
        if (isCell() && asString(*this)->getStringPropertySlot(exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        object = synthesizePrototype(exec);
    } else
        object = asObject(asCell());
    
    if (object->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);
    return jsUndefined();
}

inline TiValue TiValue::get(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline TiValue TiValue::get(ExecState* exec, unsigned propertyName, PropertySlot& slot) const
{
    // If this is a primitive, we'll need to synthesize the prototype -
    // and if it's a string there are special properties to check first.
    JSObject* object;
    if (UNLIKELY(!isObject())) {
        if (isCell() && asString(*this)->getStringPropertySlot(exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        object = synthesizePrototype(exec);
    } else
        object = asObject(asCell());
    
    if (object->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);
    return jsUndefined();
}

inline void TiValue::put(ExecState* exec, PropertyName propertyName, TiValue value, PutPropertySlot& slot)
{
    if (UNLIKELY(!isCell())) {
        putToPrimitive(exec, propertyName, value, slot);
        return;
    }
    asCell()->methodTable()->put(asCell(), exec, propertyName, value, slot);
}

inline void TiValue::putByIndex(ExecState* exec, unsigned propertyName, TiValue value, bool shouldThrow)
{
    if (UNLIKELY(!isCell())) {
        putToPrimitiveByIndex(exec, propertyName, value, shouldThrow);
        return;
    }
    asCell()->methodTable()->putByIndex(asCell(), exec, propertyName, value, shouldThrow);
}

inline TiValue TiValue::structureOrUndefined() const
{
    if (isCell())
        return TiValue(asCell()->structure());
    return jsUndefined();
}

// ECMA 11.9.3
inline bool TiValue::equal(ExecState* exec, TiValue v1, TiValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return v1 == v2;

    return equalSlowCase(exec, v1, v2);
}

ALWAYS_INLINE bool TiValue::equalSlowCaseInline(ExecState* exec, TiValue v1, TiValue v2)
{
    do {
        if (v1.isNumber() && v2.isNumber())
            return v1.asNumber() == v2.asNumber();

        bool s1 = v1.isString();
        bool s2 = v2.isString();
        if (s1 && s2)
            return asString(v1)->value(exec) == asString(v2)->value(exec);

        if (v1.isUndefinedOrNull()) {
            if (v2.isUndefinedOrNull())
                return true;
            if (!v2.isCell())
                return false;
            return v2.asCell()->structure()->masqueradesAsUndefined(exec->lexicalGlobalObject());
        }

        if (v2.isUndefinedOrNull()) {
            if (!v1.isCell())
                return false;
            return v1.asCell()->structure()->masqueradesAsUndefined(exec->lexicalGlobalObject());
        }

        if (v1.isObject()) {
            if (v2.isObject())
                return v1 == v2;
            TiValue p1 = v1.toPrimitive(exec);
            if (exec->hadException())
                return false;
            v1 = p1;
            if (v1.isInt32() && v2.isInt32())
                return v1 == v2;
            continue;
        }

        if (v2.isObject()) {
            TiValue p2 = v2.toPrimitive(exec);
            if (exec->hadException())
                return false;
            v2 = p2;
            if (v1.isInt32() && v2.isInt32())
                return v1 == v2;
            continue;
        }

        if (s1 || s2) {
            double d1 = v1.toNumber(exec);
            double d2 = v2.toNumber(exec);
            return d1 == d2;
        }

        if (v1.isBoolean()) {
            if (v2.isNumber())
                return static_cast<double>(v1.asBoolean()) == v2.asNumber();
        } else if (v2.isBoolean()) {
            if (v1.isNumber())
                return v1.asNumber() == static_cast<double>(v2.asBoolean());
        }

        return v1 == v2;
    } while (true);
}

// ECMA 11.9.3
ALWAYS_INLINE bool TiValue::strictEqualSlowCaseInline(ExecState* exec, TiValue v1, TiValue v2)
{
    ASSERT(v1.isCell() && v2.isCell());

    if (v1.asCell()->isString() && v2.asCell()->isString())
        return asString(v1)->value(exec) == asString(v2)->value(exec);

    return v1 == v2;
}

inline bool TiValue::strictEqual(ExecState* exec, TiValue v1, TiValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return v1 == v2;

    if (v1.isNumber() && v2.isNumber())
        return v1.asNumber() == v2.asNumber();

    if (!v1.isCell() || !v2.isCell())
        return v1 == v2;

    return strictEqualSlowCaseInline(exec, v1, v2);
}

inline TriState TiValue::pureStrictEqual(TiValue v1, TiValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return triState(v1 == v2);

    if (v1.isNumber() && v2.isNumber())
        return triState(v1.asNumber() == v2.asNumber());

    if (!v1.isCell() || !v2.isCell())
        return triState(v1 == v2);
    
    if (v1.asCell()->isString() && v2.asCell()->isString()) {
        const StringImpl* v1String = asString(v1)->tryGetValueImpl();
        const StringImpl* v2String = asString(v2)->tryGetValueImpl();
        if (!v1String || !v2String)
            return MixedTriState;
        return triState(WTI::equal(v1String, v2String));
    }
    
    return triState(v1 == v2);
}

inline TriState TiValue::pureToBoolean() const
{
    if (isInt32())
        return asInt32() ? TrueTriState : FalseTriState;
    if (isDouble())
        return isNotZeroAndOrdered(asDouble()) ? TrueTriState : FalseTriState; // false for NaN
    if (isCell())
        return asCell()->pureToBoolean();
    return isTrue() ? TrueTriState : FalseTriState;
}

} // namespace TI

#endif // TiValueInlines_h

