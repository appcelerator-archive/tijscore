/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
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

#ifndef JSCTiValue_h
#define JSCTiValue_h

#include <math.h>
#include <stddef.h> // for size_t
#include <stdint.h>
#include <wtf/Assertions.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashTraits.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/TriState.h>
#include "TiExportMacros.h"

namespace TI {

// This is used a lot throughout JavaScriptCore for everything from value boxing to marking
// values as being missing, so it is useful to have it abbreviated.
#define QNaN (std::numeric_limits<double>::quiet_NaN())

class AssemblyHelpers;
class ExecState;
class JSCell;
class TiValueSource;
class VM;
class JSGlobalObject;
class JSObject;
class JSString;
class PropertyName;
class PropertySlot;
class PutPropertySlot;
#if ENABLE(DFG_JIT)
namespace DFG {
class JITCompiler;
class OSRExitCompiler;
class SpeculativeJIT;
}
#endif
#if ENABLE(LLINT_C_LOOP)
namespace LLInt {
class CLoop;
}
#endif

struct ClassInfo;
struct DumpContext;
struct Instruction;
struct MethodTable;

template <class T> class WriteBarrierBase;

enum PreferredPrimitiveType { NoPreference, PreferNumber, PreferString };
enum ECMAMode { StrictMode, NotStrictMode };

typedef int64_t EncodedTiValue;
    
union EncodedValueDescriptor {
    int64_t asInt64;
#if USE(JSVALUE32_64)
    double asDouble;
#elif USE(JSVALUE64)
    JSCell* ptr;
#endif
        
#if CPU(BIG_ENDIAN)
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
};

enum WhichValueWord {
    TagWord,
    PayloadWord
};

// This implements ToInt32, defined in ECMA-262 9.5.
JS_EXPORT_PRIVATE int32_t toInt32(double);

// This implements ToUInt32, defined in ECMA-262 9.6.
inline uint32_t toUInt32(double number)
{
    // As commented in the spec, the operation of ToInt32 and ToUint32 only differ
    // in how the result is interpreted; see NOTEs in sections 9.5 and 9.6.
    return toInt32(number);
}

class TiValue {
    friend struct EncodedTiValueHashTraits;
    friend class AssemblyHelpers;
    friend class JIT;
    friend class JITSlowPathCall;
    friend class JITStubs;
    friend class JITStubCall;
    friend class JSInterfaceJIT;
    friend class TiValueSource;
    friend class SpecializedThunkJIT;
#if ENABLE(DFG_JIT)
    friend class DFG::JITCompiler;
    friend class DFG::OSRExitCompiler;
    friend class DFG::SpeculativeJIT;
#endif
#if ENABLE(LLINT_C_LOOP)
    friend class LLInt::CLoop;
#endif

public:
#if USE(JSVALUE32_64)
    enum { Int32Tag =        0xffffffff };
    enum { BooleanTag =      0xfffffffe };
    enum { NullTag =         0xfffffffd };
    enum { UndefinedTag =    0xfffffffc };
    enum { CellTag =         0xfffffffb };
    enum { EmptyValueTag =   0xfffffffa };
    enum { DeletedValueTag = 0xfffffff9 };

    enum { LowestTag =  DeletedValueTag };
#endif

    static EncodedTiValue encode(TiValue);
    static TiValue decode(EncodedTiValue);

    enum JSNullTag { JSNull };
    enum JSUndefinedTag { JSUndefined };
    enum JSTrueTag { JSTrue };
    enum JSFalseTag { JSFalse };
    enum EncodeAsDoubleTag { EncodeAsDouble };

    TiValue();
    TiValue(JSNullTag);
    TiValue(JSUndefinedTag);
    TiValue(JSTrueTag);
    TiValue(JSFalseTag);
    TiValue(JSCell* ptr);
    TiValue(const JSCell* ptr);

    // Numbers
    TiValue(EncodeAsDoubleTag, double);
    explicit TiValue(double);
    explicit TiValue(char);
    explicit TiValue(unsigned char);
    explicit TiValue(short);
    explicit TiValue(unsigned short);
    explicit TiValue(int);
    explicit TiValue(unsigned);
    explicit TiValue(long);
    explicit TiValue(unsigned long);
    explicit TiValue(long long);
    explicit TiValue(unsigned long long);

    typedef void* (TiValue::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const;
    bool operator==(const TiValue& other) const;
    bool operator!=(const TiValue& other) const;

    bool isInt32() const;
    bool isUInt32() const;
    bool isDouble() const;
    bool isTrue() const;
    bool isFalse() const;

    int32_t asInt32() const;
    uint32_t asUInt32() const;
    int64_t asMachineInt() const;
    double asDouble() const;
    bool asBoolean() const;
    double asNumber() const;

    // Querying the type.
    bool isEmpty() const;
    bool isFunction() const;
    bool isUndefined() const;
    bool isNull() const;
    bool isUndefinedOrNull() const;
    bool isBoolean() const;
    bool isMachineInt() const;
    bool isNumber() const;
    bool isString() const;
    bool isPrimitive() const;
    bool isGetterSetter() const;
    bool isObject() const;
    bool inherits(const ClassInfo*) const;
        
    // Extracting the value.
    bool getString(ExecState*, WTI::String&) const;
    WTI::String getString(ExecState*) const; // null string if not a string
    JSObject* getObject() const; // 0 if not an object

    // Extracting integer values.
    bool getUInt32(uint32_t&) const;
        
    // Basic conversions.
    TiValue toPrimitive(ExecState*, PreferredPrimitiveType = NoPreference) const;
    bool getPrimitiveNumber(ExecState*, double& number, TiValue&);

    bool toBoolean(ExecState*) const;
    TriState pureToBoolean() const;

    // toNumber conversion is expected to be side effect free if an exception has
    // been set in the ExecState already.
    double toNumber(ExecState*) const;
    JSString* toString(ExecState*) const;
    WTI::String toWTFString(ExecState*) const;
    WTI::String toWTFStringInline(ExecState*) const;
    JSObject* toObject(ExecState*) const;
    JSObject* toObject(ExecState*, JSGlobalObject*) const;

    // Integer conversions.
    JS_EXPORT_PRIVATE double toInteger(ExecState*) const;
    double toIntegerPreserveNaN(ExecState*) const;
    int32_t toInt32(ExecState*) const;
    uint32_t toUInt32(ExecState*) const;

    // Floating point conversions (this is a convenience method for webcore;
    // signle precision float is not a representation used in JS or JSC).
    float toFloat(ExecState* exec) const { return static_cast<float>(toNumber(exec)); }

    // Object operations, with the toObject operation included.
    TiValue get(ExecState*, PropertyName) const;
    TiValue get(ExecState*, PropertyName, PropertySlot&) const;
    TiValue get(ExecState*, unsigned propertyName) const;
    TiValue get(ExecState*, unsigned propertyName, PropertySlot&) const;
    void put(ExecState*, PropertyName, TiValue, PutPropertySlot&);
    void putToPrimitive(ExecState*, PropertyName, TiValue, PutPropertySlot&);
    void putToPrimitiveByIndex(ExecState*, unsigned propertyName, TiValue, bool shouldThrow);
    void putByIndex(ExecState*, unsigned propertyName, TiValue, bool shouldThrow);

    TiValue toThis(ExecState*, ECMAMode) const;

    static bool equal(ExecState*, TiValue v1, TiValue v2);
    static bool equalSlowCase(ExecState*, TiValue v1, TiValue v2);
    static bool equalSlowCaseInline(ExecState*, TiValue v1, TiValue v2);
    static bool strictEqual(ExecState*, TiValue v1, TiValue v2);
    static bool strictEqualSlowCase(ExecState*, TiValue v1, TiValue v2);
    static bool strictEqualSlowCaseInline(ExecState*, TiValue v1, TiValue v2);
    static TriState pureStrictEqual(TiValue v1, TiValue v2);

    bool isCell() const;
    JSCell* asCell() const;
    JS_EXPORT_PRIVATE bool isValidCallee();
        
    TiValue structureOrUndefined() const;

    JS_EXPORT_PRIVATE void dump(PrintStream&) const;
    void dumpInContext(PrintStream&, DumpContext*) const;

    JS_EXPORT_PRIVATE JSObject* synthesizePrototype(ExecState*) const;

    // Constants used for Int52. Int52 isn't part of TiValue right now, but TiValues may be
    // converted to Int52s and back again.
    static const unsigned numberOfInt52Bits = 52;
    static const unsigned int52ShiftAmount = 12;
    
    static ptrdiff_t offsetOfPayload() { return OBJECT_OFFSETOF(TiValue, u.asBits.payload); }
    static ptrdiff_t offsetOfTag() { return OBJECT_OFFSETOF(TiValue, u.asBits.tag); }

private:
    template <class T> TiValue(WriteBarrierBase<T>);

    enum HashTableDeletedValueTag { HashTableDeletedValue };
    TiValue(HashTableDeletedValueTag);

    inline const TiValue asValue() const { return *this; }
    JS_EXPORT_PRIVATE double toNumberSlowCase(ExecState*) const;
    JS_EXPORT_PRIVATE JSString* toStringSlowCase(ExecState*) const;
    JS_EXPORT_PRIVATE WTI::String toWTFStringSlowCase(ExecState*) const;
    JS_EXPORT_PRIVATE JSObject* toObjectSlowCase(ExecState*, JSGlobalObject*) const;
    JS_EXPORT_PRIVATE TiValue toThisSlowCase(ExecState*, ECMAMode) const;

#if USE(JSVALUE32_64)
    /*
     * On 32-bit platforms USE(JSVALUE32_64) should be defined, and we use a NaN-encoded
     * form for immediates.
     *
     * The encoding makes use of unused NaN space in the IEEE754 representation.  Any value
     * with the top 13 bits set represents a QNaN (with the sign bit set).  QNaN values
     * can encode a 51-bit payload.  Hardware produced and C-library payloads typically
     * have a payload of zero.  We assume that non-zero payloads are available to encode
     * pointer and integer values.  Since any 64-bit bit pattern where the top 15 bits are
     * all set represents a NaN with a non-zero payload, we can use this space in the NaN
     * ranges to encode other values (however there are also other ranges of NaN space that
     * could have been selected).
     *
     * For TiValues that do not contain a double value, the high 32 bits contain the tag
     * values listed in the enums below, which all correspond to NaN-space. In the case of
     * cell, integer and bool values the lower 32 bits (the 'payload') contain the pointer
     * integer or boolean value; in the case of all other tags the payload is 0.
     */
    uint32_t tag() const;
    int32_t payload() const;

#if ENABLE(LLINT_C_LOOP)
    // This should only be used by the LLInt C Loop interpreter who needs
    // synthesize TiValue from its "register"s holding tag and payload
    // values.
    explicit TiValue(int32_t tag, int32_t payload);
#endif

#elif USE(JSVALUE64)
    /*
     * On 64-bit platforms USE(JSVALUE64) should be defined, and we use a NaN-encoded
     * form for immediates.
     *
     * The encoding makes use of unused NaN space in the IEEE754 representation.  Any value
     * with the top 13 bits set represents a QNaN (with the sign bit set).  QNaN values
     * can encode a 51-bit payload.  Hardware produced and C-library payloads typically
     * have a payload of zero.  We assume that non-zero payloads are available to encode
     * pointer and integer values.  Since any 64-bit bit pattern where the top 15 bits are
     * all set represents a NaN with a non-zero payload, we can use this space in the NaN
     * ranges to encode other values (however there are also other ranges of NaN space that
     * could have been selected).
     *
     * This range of NaN space is represented by 64-bit numbers begining with the 16-bit
     * hex patterns 0xFFFE and 0xFFFF - we rely on the fact that no valid double-precision
     * numbers will begin fall in these ranges.
     *
     * The top 16-bits denote the type of the encoded TiValue:
     *
     *     Pointer {  0000:PPPP:PPPP:PPPP
     *              / 0001:****:****:****
     *     Double  {         ...
     *              \ FFFE:****:****:****
     *     Integer {  FFFF:0000:IIII:IIII
     *
     * The scheme we have implemented encodes double precision values by performing a
     * 64-bit integer addition of the value 2^48 to the number. After this manipulation
     * no encoded double-precision value will begin with the pattern 0x0000 or 0xFFFF.
     * Values must be decoded by reversing this operation before subsequent floating point
     * operations my be peformed.
     *
     * 32-bit signed integers are marked with the 16-bit tag 0xFFFF.
     *
     * The tag 0x0000 denotes a pointer, or another form of tagged immediate. Boolean,
     * null and undefined values are represented by specific, invalid pointer values:
     *
     *     False:     0x06
     *     True:      0x07
     *     Undefined: 0x0a
     *     Null:      0x02
     *
     * These values have the following properties:
     * - Bit 1 (TagBitTypeOther) is set for all four values, allowing real pointers to be
     *   quickly distinguished from all immediate values, including these invalid pointers.
     * - With bit 3 is masked out (TagBitUndefined) Undefined and Null share the
     *   same value, allowing null & undefined to be quickly detected.
     *
     * No valid TiValue will have the bit pattern 0x0, this is used to represent array
     * holes, and as a C++ 'no value' result (e.g. TiValue() has an internal value of 0).
     */

    // These values are #defines since using static const integers here is a ~1% regression!

    // This value is 2^48, used to encode doubles such that the encoded value will begin
    // with a 16-bit pattern within the range 0x0001..0xFFFE.
    #define DoubleEncodeOffset 0x1000000000000ll
    // If all bits in the mask are set, this indicates an integer number,
    // if any but not all are set this value is a double precision number.
    #define TagTypeNumber 0xffff000000000000ll

    // All non-numeric (bool, null, undefined) immediates have bit 2 set.
    #define TagBitTypeOther 0x2ll
    #define TagBitBool      0x4ll
    #define TagBitUndefined 0x8ll
    // Combined integer value for non-numeric immediates.
    #define ValueFalse     (TagBitTypeOther | TagBitBool | false)
    #define ValueTrue      (TagBitTypeOther | TagBitBool | true)
    #define ValueUndefined (TagBitTypeOther | TagBitUndefined)
    #define ValueNull      (TagBitTypeOther)

    // TagMask is used to check for all types of immediate values (either number or 'other').
    #define TagMask (TagTypeNumber | TagBitTypeOther)

    // These special values are never visible to JavaScript code; Empty is used to represent
    // Array holes, and for uninitialized TiValues. Deleted is used in hash table code.
    // These values would map to cell types in the TiValue encoding, but not valid GC cell
    // pointer should have either of these values (Empty is null, deleted is at an invalid
    // alignment for a GC cell, and in the zero page).
    #define ValueEmpty   0x0ll
    #define ValueDeleted 0x4ll
#endif

    EncodedValueDescriptor u;
};

typedef IntHash<EncodedTiValue> EncodedTiValueHash;

#if USE(JSVALUE32_64)
struct EncodedTiValueHashTraits : HashTraits<EncodedTiValue> {
    static const bool emptyValueIsZero = false;
    static EncodedTiValue emptyValue() { return TiValue::encode(TiValue()); }
    static void constructDeletedValue(EncodedTiValue& slot) { slot = TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
    static bool isDeletedValue(EncodedTiValue value) { return value == TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
};
#else
struct EncodedTiValueHashTraits : HashTraits<EncodedTiValue> {
    static void constructDeletedValue(EncodedTiValue& slot) { slot = TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
    static bool isDeletedValue(EncodedTiValue value) { return value == TiValue::encode(TiValue(TiValue::HashTableDeletedValue)); }
};
#endif

typedef HashMap<EncodedTiValue, unsigned, EncodedTiValueHash, EncodedTiValueHashTraits> TiValueMap;

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

ALWAYS_INLINE TiValue jsDoubleNumber(double d)
{
    ASSERT(TiValue(TiValue::EncodeAsDouble, d).isNumber());
    return TiValue(TiValue::EncodeAsDouble, d);
}

ALWAYS_INLINE TiValue jsNumber(double d)
{
    ASSERT(TiValue(d).isNumber());
    return TiValue(d);
}

ALWAYS_INLINE TiValue jsNumber(char i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(unsigned char i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(short i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(unsigned short i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(int i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(unsigned i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(long i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(unsigned long i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(long long i)
{
    return TiValue(i);
}

ALWAYS_INLINE TiValue jsNumber(unsigned long long i)
{
    return TiValue(i);
}

inline bool operator==(const TiValue a, const JSCell* b) { return a == TiValue(b); }
inline bool operator==(const JSCell* a, const TiValue b) { return TiValue(a) == b; }

inline bool operator!=(const TiValue a, const JSCell* b) { return a != TiValue(b); }
inline bool operator!=(const JSCell* a, const TiValue b) { return TiValue(a) != b; }

} // namespace TI

#endif // JSCTiValue_h
