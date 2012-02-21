/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "JSONObject.h"

#include "BooleanObject.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "TiArray.h"
#include "TiGlobalObject.h"
#include "LiteralParser.h"
#include "Local.h"
#include "LocalScope.h"
#include "Lookup.h"
#include "PropertyNameArray.h"
#include "UStringBuilder.h"
#include "UStringConcatenate.h"
#include <wtf/MathExtras.h>

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(JSONObject);

static EncodedTiValue JSC_HOST_CALL JSONProtoFuncParse(TiExcState*);
static EncodedTiValue JSC_HOST_CALL JSONProtoFuncStringify(TiExcState*);

}

#include "JSONObject.lut.h"

namespace TI {

JSONObject::JSONObject(TiGlobalObject* globalObject, Structure* structure)
    : TiObjectWithGlobalObject(globalObject, structure)
{
    ASSERT(inherits(&s_info));
}

// PropertyNameForFunctionCall objects must be on the stack, since the TiValue that they create is not marked.
class PropertyNameForFunctionCall {
public:
    PropertyNameForFunctionCall(const Identifier&);
    PropertyNameForFunctionCall(unsigned);

    TiValue value(TiExcState*) const;

private:
    const Identifier* m_identifier;
    unsigned m_number;
    mutable TiValue m_value;
};

class Stringifier {
    WTF_MAKE_NONCOPYABLE(Stringifier);
public:
    Stringifier(TiExcState*, const Local<Unknown>& replacer, const Local<Unknown>& space);
    Local<Unknown> stringify(Handle<Unknown>);

    void visitAggregate(SlotVisitor&);

private:
    class Holder {
    public:
        Holder(TiGlobalData&, TiObject*);

        TiObject* object() const { return m_object.get(); }

        bool appendNextProperty(Stringifier&, UStringBuilder&);

    private:
        Local<TiObject> m_object;
        const bool m_isArray;
        bool m_isTiArray;
        unsigned m_index;
        unsigned m_size;
        RefPtr<PropertyNameArrayData> m_propertyNames;
    };

    friend class Holder;

    static void appendQuotedString(UStringBuilder&, const UString&);

    TiValue toJSON(TiValue, const PropertyNameForFunctionCall&);

    enum StringifyResult { StringifyFailed, StringifySucceeded, StringifyFailedDueToUndefinedValue };
    StringifyResult appendStringifiedValue(UStringBuilder&, TiValue, TiObject* holder, const PropertyNameForFunctionCall&);

    bool willIndent() const;
    void indent();
    void unindent();
    void startNewLine(UStringBuilder&) const;

    TiExcState* const m_exec;
    const Local<Unknown> m_replacer;
    bool m_usingArrayReplacer;
    PropertyNameArray m_arrayReplacerPropertyNames;
    CallType m_replacerCallType;
    CallData m_replacerCallData;
    const UString m_gap;

    Vector<Holder, 16> m_holderStack;
    UString m_repeatedGap;
    UString m_indent;
};

// ------------------------------ helper functions --------------------------------

static inline TiValue unwrapBoxedPrimitive(TiExcState* exec, TiValue value)
{
    if (!value.isObject())
        return value;
    TiObject* object = asObject(value);
    if (object->inherits(&NumberObject::s_info))
        return jsNumber(object->toNumber(exec));
    if (object->inherits(&StringObject::s_info))
        return jsString(exec, object->toString(exec));
    if (object->inherits(&BooleanObject::s_info))
        return object->toPrimitive(exec);
    return value;
}

static inline UString gap(TiExcState* exec, TiValue space)
{
    const unsigned maxGapLength = 10;
    space = unwrapBoxedPrimitive(exec, space);

    // If the space value is a number, create a gap string with that number of spaces.
    double spaceCount;
    if (space.getNumber(spaceCount)) {
        int count;
        if (spaceCount > maxGapLength)
            count = maxGapLength;
        else if (!(spaceCount > 0))
            count = 0;
        else
            count = static_cast<int>(spaceCount);
        UChar spaces[maxGapLength];
        for (int i = 0; i < count; ++i)
            spaces[i] = ' ';
        return UString(spaces, count);
    }

    // If the space value is a string, use it as the gap string, otherwise use no gap string.
    UString spaces = space.getString(exec);
    if (spaces.length() > maxGapLength) {
        spaces = spaces.substringSharingImpl(0, maxGapLength);
    }
    return spaces;
}

// ------------------------------ PropertyNameForFunctionCall --------------------------------

inline PropertyNameForFunctionCall::PropertyNameForFunctionCall(const Identifier& identifier)
    : m_identifier(&identifier)
{
}

inline PropertyNameForFunctionCall::PropertyNameForFunctionCall(unsigned number)
    : m_identifier(0)
    , m_number(number)
{
}

TiValue PropertyNameForFunctionCall::value(TiExcState* exec) const
{
    if (!m_value) {
        if (m_identifier)
            m_value = jsString(exec, m_identifier->ustring());
        else
            m_value = jsNumber(m_number);
    }
    return m_value;
}

// ------------------------------ Stringifier --------------------------------

Stringifier::Stringifier(TiExcState* exec, const Local<Unknown>& replacer, const Local<Unknown>& space)
    : m_exec(exec)
    , m_replacer(replacer)
    , m_usingArrayReplacer(false)
    , m_arrayReplacerPropertyNames(exec)
    , m_replacerCallType(CallTypeNone)
    , m_gap(gap(exec, space.get()))
{
    if (!m_replacer.isObject())
        return;

    if (m_replacer.asObject()->inherits(&TiArray::s_info)) {
        m_usingArrayReplacer = true;
        Handle<TiObject> array = m_replacer.asObject();
        unsigned length = array->get(exec, exec->globalData().propertyNames->length).toUInt32(exec);
        for (unsigned i = 0; i < length; ++i) {
            TiValue name = array->get(exec, i);
            if (exec->hadException())
                break;

            UString propertyName;
            if (name.getString(exec, propertyName)) {
                m_arrayReplacerPropertyNames.add(Identifier(exec, propertyName));
                continue;
            }

            double value = 0;
            if (name.getNumber(value)) {
                m_arrayReplacerPropertyNames.add(Identifier::from(exec, value));
                continue;
            }

            if (name.isObject()) {
                if (!asObject(name)->inherits(&NumberObject::s_info) && !asObject(name)->inherits(&StringObject::s_info))
                    continue;
                propertyName = name.toString(exec);
                if (exec->hadException())
                    break;
                m_arrayReplacerPropertyNames.add(Identifier(exec, propertyName));
            }
        }
        return;
    }

    m_replacerCallType = m_replacer.asObject()->getCallData(m_replacerCallData);
}

Local<Unknown> Stringifier::stringify(Handle<Unknown> value)
{
    TiObject* object = constructEmptyObject(m_exec);
    if (m_exec->hadException())
        return Local<Unknown>(m_exec->globalData(), jsNull());

    PropertyNameForFunctionCall emptyPropertyName(m_exec->globalData().propertyNames->emptyIdentifier);
    object->putDirect(m_exec->globalData(), m_exec->globalData().propertyNames->emptyIdentifier, value.get());

    UStringBuilder result;
    if (appendStringifiedValue(result, value.get(), object, emptyPropertyName) != StringifySucceeded)
        return Local<Unknown>(m_exec->globalData(), jsUndefined());
    if (m_exec->hadException())
        return Local<Unknown>(m_exec->globalData(), jsNull());

    return Local<Unknown>(m_exec->globalData(), jsString(m_exec, result.toUString()));
}

void Stringifier::appendQuotedString(UStringBuilder& builder, const UString& value)
{
    int length = value.length();

    builder.append('"');

    const UChar* data = value.characters();
    for (int i = 0; i < length; ++i) {
        int start = i;
        while (i < length && (data[i] > 0x1F && data[i] != '"' && data[i] != '\\'))
            ++i;
        builder.append(data + start, i - start);
        if (i >= length)
            break;
        switch (data[i]) {
            case '\t':
                builder.append('\\');
                builder.append('t');
                break;
            case '\r':
                builder.append('\\');
                builder.append('r');
                break;
            case '\n':
                builder.append('\\');
                builder.append('n');
                break;
            case '\f':
                builder.append('\\');
                builder.append('f');
                break;
            case '\b':
                builder.append('\\');
                builder.append('b');
                break;
            case '"':
                builder.append('\\');
                builder.append('"');
                break;
            case '\\':
                builder.append('\\');
                builder.append('\\');
                break;
            default:
                static const char hexDigits[] = "0123456789abcdef";
                UChar ch = data[i];
                UChar hex[] = { '\\', 'u', hexDigits[(ch >> 12) & 0xF], hexDigits[(ch >> 8) & 0xF], hexDigits[(ch >> 4) & 0xF], hexDigits[ch & 0xF] };
                builder.append(hex, WTF_ARRAY_LENGTH(hex));
                break;
        }
    }

    builder.append('"');
}

inline TiValue Stringifier::toJSON(TiValue value, const PropertyNameForFunctionCall& propertyName)
{
    ASSERT(!m_exec->hadException());
    if (!value.isObject() || !asObject(value)->hasProperty(m_exec, m_exec->globalData().propertyNames->toJSON))
        return value;

    TiValue toJSONFunction = asObject(value)->get(m_exec, m_exec->globalData().propertyNames->toJSON);
    if (m_exec->hadException())
        return jsNull();

    if (!toJSONFunction.isObject())
        return value;

    TiObject* object = asObject(toJSONFunction);
    CallData callData;
    CallType callType = object->getCallData(callData);
    if (callType == CallTypeNone)
        return value;

    TiValue list[] = { propertyName.value(m_exec) };
    ArgList args(list, WTF_ARRAY_LENGTH(list));
    return call(m_exec, object, callType, callData, value, args);
}

Stringifier::StringifyResult Stringifier::appendStringifiedValue(UStringBuilder& builder, TiValue value, TiObject* holder, const PropertyNameForFunctionCall& propertyName)
{
    // Call the toJSON function.
    value = toJSON(value, propertyName);
    if (m_exec->hadException())
        return StringifyFailed;

    // Call the replacer function.
    if (m_replacerCallType != CallTypeNone) {
        TiValue list[] = { propertyName.value(m_exec), value };
        ArgList args(list, WTF_ARRAY_LENGTH(list));
        value = call(m_exec, m_replacer.get(), m_replacerCallType, m_replacerCallData, holder, args);
        if (m_exec->hadException())
            return StringifyFailed;
    }

    if (value.isUndefined() && !holder->inherits(&TiArray::s_info))
        return StringifyFailedDueToUndefinedValue;

    if (value.isNull()) {
        builder.append("null");
        return StringifySucceeded;
    }

    value = unwrapBoxedPrimitive(m_exec, value);

    if (m_exec->hadException())
        return StringifyFailed;

    if (value.isBoolean()) {
        builder.append(value.getBoolean() ? "true" : "false");
        return StringifySucceeded;
    }

    UString stringValue;
    if (value.getString(m_exec, stringValue)) {
        appendQuotedString(builder, stringValue);
        return StringifySucceeded;
    }

    double numericValue;
    if (value.getNumber(numericValue)) {
        if (!isfinite(numericValue))
            builder.append("null");
        else
            builder.append(UString::number(numericValue));
        return StringifySucceeded;
    }

    if (!value.isObject())
        return StringifyFailed;

    TiObject* object = asObject(value);

    CallData callData;
    if (object->getCallData(callData) != CallTypeNone) {
        if (holder->inherits(&TiArray::s_info)) {
            builder.append("null");
            return StringifySucceeded;
        }
        return StringifyFailedDueToUndefinedValue;
    }

    // Handle cycle detection, and put the holder on the stack.
    for (unsigned i = 0; i < m_holderStack.size(); i++) {
        if (m_holderStack[i].object() == object) {
            throwError(m_exec, createTypeError(m_exec, "JSON.stringify cannot serialize cyclic structures."));
            return StringifyFailed;
        }
    }
    bool holderStackWasEmpty = m_holderStack.isEmpty();
    m_holderStack.append(Holder(m_exec->globalData(), object));
    if (!holderStackWasEmpty)
        return StringifySucceeded;

    // If this is the outermost call, then loop to handle everything on the holder stack.
    TimeoutChecker localTimeoutChecker(m_exec->globalData().timeoutChecker);
    localTimeoutChecker.reset();
    unsigned tickCount = localTimeoutChecker.ticksUntilNextCheck();
    do {
        while (m_holderStack.last().appendNextProperty(*this, builder)) {
            if (m_exec->hadException())
                return StringifyFailed;
            if (!--tickCount) {
                if (localTimeoutChecker.didTimeOut(m_exec)) {
                    throwError(m_exec, createInterruptedExecutionException(&m_exec->globalData()));
                    return StringifyFailed;
                }
                tickCount = localTimeoutChecker.ticksUntilNextCheck();
            }
        }
        m_holderStack.removeLast();
    } while (!m_holderStack.isEmpty());
    return StringifySucceeded;
}

inline bool Stringifier::willIndent() const
{
    return !m_gap.isEmpty();
}

inline void Stringifier::indent()
{
    // Use a single shared string, m_repeatedGap, so we don't keep allocating new ones as we indent and unindent.
    unsigned newSize = m_indent.length() + m_gap.length();
    if (newSize > m_repeatedGap.length())
        m_repeatedGap = makeUString(m_repeatedGap, m_gap);
    ASSERT(newSize <= m_repeatedGap.length());
    m_indent = m_repeatedGap.substringSharingImpl(0, newSize);
}

inline void Stringifier::unindent()
{
    ASSERT(m_indent.length() >= m_gap.length());
    m_indent = m_repeatedGap.substringSharingImpl(0, m_indent.length() - m_gap.length());
}

inline void Stringifier::startNewLine(UStringBuilder& builder) const
{
    if (m_gap.isEmpty())
        return;
    builder.append('\n');
    builder.append(m_indent);
}

inline Stringifier::Holder::Holder(TiGlobalData& globalData, TiObject* object)
    : m_object(globalData, object)
    , m_isArray(object->inherits(&TiArray::s_info))
    , m_index(0)
{
}

bool Stringifier::Holder::appendNextProperty(Stringifier& stringifier, UStringBuilder& builder)
{
    ASSERT(m_index <= m_size);

    TiExcState* exec = stringifier.m_exec;

    // First time through, initialize.
    if (!m_index) {
        if (m_isArray) {
            m_isTiArray = isTiArray(&exec->globalData(), m_object.get());
            m_size = m_object->get(exec, exec->globalData().propertyNames->length).toUInt32(exec);
            builder.append('[');
        } else {
            if (stringifier.m_usingArrayReplacer)
                m_propertyNames = stringifier.m_arrayReplacerPropertyNames.data();
            else {
                PropertyNameArray objectPropertyNames(exec);
                m_object->getOwnPropertyNames(exec, objectPropertyNames);
                m_propertyNames = objectPropertyNames.releaseData();
            }
            m_size = m_propertyNames->propertyNameVector().size();
            builder.append('{');
        }
        stringifier.indent();
    }

    // Last time through, finish up and return false.
    if (m_index == m_size) {
        stringifier.unindent();
        if (m_size && builder[builder.length() - 1] != '{')
            stringifier.startNewLine(builder);
        builder.append(m_isArray ? ']' : '}');
        return false;
    }

    // Handle a single element of the array or object.
    unsigned index = m_index++;
    unsigned rollBackPoint = 0;
    StringifyResult stringifyResult;
    if (m_isArray) {
        // Get the value.
        TiValue value;
        if (m_isTiArray && asArray(m_object.get())->canGetIndex(index))
            value = asArray(m_object.get())->getIndex(index);
        else {
            PropertySlot slot(m_object.get());
            if (!m_object->getOwnPropertySlot(exec, index, slot))
                slot.setUndefined();
            if (exec->hadException())
                return false;
            value = slot.getValue(exec, index);
        }

        // Append the separator string.
        if (index)
            builder.append(',');
        stringifier.startNewLine(builder);

        // Append the stringified value.
        stringifyResult = stringifier.appendStringifiedValue(builder, value, m_object.get(), index);
    } else {
        // Get the value.
        PropertySlot slot(m_object.get());
        Identifier& propertyName = m_propertyNames->propertyNameVector()[index];
        if (!m_object->getOwnPropertySlot(exec, propertyName, slot))
            return true;
        TiValue value = slot.getValue(exec, propertyName);
        if (exec->hadException())
            return false;

        rollBackPoint = builder.length();

        // Append the separator string.
        if (builder[rollBackPoint - 1] != '{')
            builder.append(',');
        stringifier.startNewLine(builder);

        // Append the property name.
        appendQuotedString(builder, propertyName.ustring());
        builder.append(':');
        if (stringifier.willIndent())
            builder.append(' ');

        // Append the stringified value.
        stringifyResult = stringifier.appendStringifiedValue(builder, value, m_object.get(), propertyName);
    }

    // From this point on, no access to the this pointer or to any members, because the
    // Holder object may have moved if the call to stringify pushed a new Holder onto
    // m_holderStack.

    switch (stringifyResult) {
        case StringifyFailed:
            builder.append("null");
            break;
        case StringifySucceeded:
            break;
        case StringifyFailedDueToUndefinedValue:
            // This only occurs when get an undefined value for an object property.
            // In this case we don't want the separator and property name that we
            // already appended, so roll back.
            builder.resize(rollBackPoint);
            break;
    }

    return true;
}

// ------------------------------ JSONObject --------------------------------

const ClassInfo JSONObject::s_info = { "JSON", &TiObjectWithGlobalObject::s_info, 0, TiExcState::jsonTable };

/* Source for JSONObject.lut.h
@begin jsonTable
  parse         JSONProtoFuncParse             DontEnum|Function 2
  stringify     JSONProtoFuncStringify         DontEnum|Function 3
@end
*/

// ECMA 15.8

bool JSONObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<TiObject>(exec, TiExcState::jsonTable(exec), this, propertyName, slot);
}

bool JSONObject::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<TiObject>(exec, TiExcState::jsonTable(exec), this, propertyName, descriptor);
}

class Walker {
public:
    Walker(TiExcState* exec, Handle<TiObject> function, CallType callType, CallData callData)
        : m_exec(exec)
        , m_function(exec->globalData(), function)
        , m_callType(callType)
        , m_callData(callData)
    {
    }
    TiValue walk(TiValue unfiltered);
private:
    TiValue callReviver(TiObject* thisObj, TiValue property, TiValue unfiltered)
    {
        TiValue args[] = { property, unfiltered };
        ArgList argList(args, 2);
        return call(m_exec, m_function.get(), m_callType, m_callData, thisObj, argList);
    }

    friend class Holder;

    TiExcState* m_exec;
    Local<TiObject> m_function;
    CallType m_callType;
    CallData m_callData;
};

// We clamp recursion well beyond anything reasonable, but we also have a timeout check
// to guard against "infinite" execution by inserting arbitrarily large objects.
static const unsigned maximumFilterRecursion = 40000;
enum WalkerState { StateUnknown, ArrayStartState, ArrayStartVisitMember, ArrayEndVisitMember, 
                                 ObjectStartState, ObjectStartVisitMember, ObjectEndVisitMember };
NEVER_INLINE TiValue Walker::walk(TiValue unfiltered)
{
    Vector<PropertyNameArray, 16> propertyStack;
    Vector<uint32_t, 16> indexStack;
    LocalStack<TiObject, 16> objectStack(m_exec->globalData());
    LocalStack<TiArray, 16> arrayStack(m_exec->globalData());
    
    Vector<WalkerState, 16> stateStack;
    WalkerState state = StateUnknown;
    TiValue inValue = unfiltered;
    TiValue outValue = jsNull();
    
    TimeoutChecker localTimeoutChecker(m_exec->globalData().timeoutChecker);
    localTimeoutChecker.reset();
    unsigned tickCount = localTimeoutChecker.ticksUntilNextCheck();
    while (1) {
        switch (state) {
            arrayStartState:
            case ArrayStartState: {
                ASSERT(inValue.isObject());
                ASSERT(isTiArray(&m_exec->globalData(), asObject(inValue)) || asObject(inValue)->inherits(&TiArray::s_info));
                if (objectStack.size() + arrayStack.size() > maximumFilterRecursion)
                    return throwError(m_exec, createStackOverflowError(m_exec));

                TiArray* array = asArray(inValue);
                arrayStack.push(array);
                indexStack.append(0);
                // fallthrough
            }
            arrayStartVisitMember:
            case ArrayStartVisitMember: {
                if (!--tickCount) {
                    if (localTimeoutChecker.didTimeOut(m_exec))
                        return throwError(m_exec, createInterruptedExecutionException(&m_exec->globalData()));
                    tickCount = localTimeoutChecker.ticksUntilNextCheck();
                }

                TiArray* array = arrayStack.peek();
                uint32_t index = indexStack.last();
                if (index == array->length()) {
                    outValue = array;
                    arrayStack.pop();
                    indexStack.removeLast();
                    break;
                }
                if (isTiArray(&m_exec->globalData(), array) && array->canGetIndex(index))
                    inValue = array->getIndex(index);
                else {
                    PropertySlot slot;
                    if (array->getOwnPropertySlot(m_exec, index, slot))
                        inValue = slot.getValue(m_exec, index);
                    else
                        inValue = jsUndefined();
                }
                    
                if (inValue.isObject()) {
                    stateStack.append(ArrayEndVisitMember);
                    goto stateUnknown;
                } else
                    outValue = inValue;
                // fallthrough
            }
            case ArrayEndVisitMember: {
                TiArray* array = arrayStack.peek();
                TiValue filteredValue = callReviver(array, jsString(m_exec, UString::number(indexStack.last())), outValue);
                if (filteredValue.isUndefined())
                    array->deleteProperty(m_exec, indexStack.last());
                else {
                    if (isTiArray(&m_exec->globalData(), array) && array->canSetIndex(indexStack.last()))
                        array->setIndex(m_exec->globalData(), indexStack.last(), filteredValue);
                    else
                        array->put(m_exec, indexStack.last(), filteredValue);
                }
                if (m_exec->hadException())
                    return jsNull();
                indexStack.last()++;
                goto arrayStartVisitMember;
            }
            objectStartState:
            case ObjectStartState: {
                ASSERT(inValue.isObject());
                ASSERT(!isTiArray(&m_exec->globalData(), asObject(inValue)) && !asObject(inValue)->inherits(&TiArray::s_info));
                if (objectStack.size() + arrayStack.size() > maximumFilterRecursion)
                    return throwError(m_exec, createStackOverflowError(m_exec));

                TiObject* object = asObject(inValue);
                objectStack.push(object);
                indexStack.append(0);
                propertyStack.append(PropertyNameArray(m_exec));
                object->getOwnPropertyNames(m_exec, propertyStack.last());
                // fallthrough
            }
            objectStartVisitMember:
            case ObjectStartVisitMember: {
                if (!--tickCount) {
                    if (localTimeoutChecker.didTimeOut(m_exec))
                        return throwError(m_exec, createInterruptedExecutionException(&m_exec->globalData()));
                    tickCount = localTimeoutChecker.ticksUntilNextCheck();
                }

                TiObject* object = objectStack.peek();
                uint32_t index = indexStack.last();
                PropertyNameArray& properties = propertyStack.last();
                if (index == properties.size()) {
                    outValue = object;
                    objectStack.pop();
                    indexStack.removeLast();
                    propertyStack.removeLast();
                    break;
                }
                PropertySlot slot;
                if (object->getOwnPropertySlot(m_exec, properties[index], slot))
                    inValue = slot.getValue(m_exec, properties[index]);
                else
                    inValue = jsUndefined();

                // The holder may be modified by the reviver function so any lookup may throw
                if (m_exec->hadException())
                    return jsNull();

                if (inValue.isObject()) {
                    stateStack.append(ObjectEndVisitMember);
                    goto stateUnknown;
                } else
                    outValue = inValue;
                // fallthrough
            }
            case ObjectEndVisitMember: {
                TiObject* object = objectStack.peek();
                Identifier prop = propertyStack.last()[indexStack.last()];
                PutPropertySlot slot;
                TiValue filteredValue = callReviver(object, jsString(m_exec, prop.ustring()), outValue);
                if (filteredValue.isUndefined())
                    object->deleteProperty(m_exec, prop);
                else
                    object->put(m_exec, prop, filteredValue, slot);
                if (m_exec->hadException())
                    return jsNull();
                indexStack.last()++;
                goto objectStartVisitMember;
            }
            stateUnknown:
            case StateUnknown:
                if (!inValue.isObject()) {
                    outValue = inValue;
                    break;
                }
                TiObject* object = asObject(inValue);
                if (isTiArray(&m_exec->globalData(), object) || object->inherits(&TiArray::s_info))
                    goto arrayStartState;
                goto objectStartState;
        }
        if (stateStack.isEmpty())
            break;

        state = stateStack.last();
        stateStack.removeLast();

        if (!--tickCount) {
            if (localTimeoutChecker.didTimeOut(m_exec))
                return throwError(m_exec, createInterruptedExecutionException(&m_exec->globalData()));
            tickCount = localTimeoutChecker.ticksUntilNextCheck();
        }
    }
    TiObject* finalHolder = constructEmptyObject(m_exec);
    PutPropertySlot slot;
    finalHolder->put(m_exec, m_exec->globalData().propertyNames->emptyIdentifier, outValue, slot);
    return callReviver(finalHolder, jsEmptyString(m_exec), outValue);
}

// ECMA-262 v5 15.12.2
EncodedTiValue JSC_HOST_CALL JSONProtoFuncParse(TiExcState* exec)
{
    if (!exec->argumentCount())
        return throwVMError(exec, createError(exec, "JSON.parse requires at least one parameter"));
    TiValue value = exec->argument(0);
    UString source = value.toString(exec);
    if (exec->hadException())
        return TiValue::encode(jsNull());

    LocalScope scope(exec->globalData());
    LiteralParser jsonParser(exec, source.characters(), source.length(), LiteralParser::StrictJSON);
    TiValue unfiltered = jsonParser.tryLiteralParse();
    if (!unfiltered)
        return throwVMError(exec, createSyntaxError(exec, "Unable to parse JSON string"));
    
    if (exec->argumentCount() < 2)
        return TiValue::encode(unfiltered);
    
    TiValue function = exec->argument(1);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return TiValue::encode(unfiltered);
    return TiValue::encode(Walker(exec, Local<TiObject>(exec->globalData(), asObject(function)), callType, callData).walk(unfiltered));
}

// ECMA-262 v5 15.12.3
EncodedTiValue JSC_HOST_CALL JSONProtoFuncStringify(TiExcState* exec)
{
    if (!exec->argumentCount())
        return throwVMError(exec, createError(exec, "No input to stringify"));
    LocalScope scope(exec->globalData());
    Local<Unknown> value(exec->globalData(), exec->argument(0));
    Local<Unknown> replacer(exec->globalData(), exec->argument(1));
    Local<Unknown> space(exec->globalData(), exec->argument(2));
    return TiValue::encode(Stringifier(exec, replacer, space).stringify(value).get());
}

UString JSONStringify(TiExcState* exec, TiValue value, unsigned indent)
{
    LocalScope scope(exec->globalData());
    Local<Unknown> result = Stringifier(exec, Local<Unknown>(exec->globalData(), jsNull()), Local<Unknown>(exec->globalData(), jsNumber(indent))).stringify(Local<Unknown>(exec->globalData(), value));
    if (result.isUndefinedOrNull())
        return UString();
    return result.getString(exec);
}

} // namespace TI
