/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Torch Mobile, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "StringPrototype.h"

#include "CachedCall.h"
#include "Error.h"
#include "Executable.h"
#include "TiGlobalObjectFunctions.h"
#include "TiArray.h"
#include "TiFunction.h"
#include "TiStringBuilder.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "PropertyNameArray.h"
#include "RegExpCache.h"
#include "RegExpConstructor.h"
#include "RegExpObject.h"
#include <wtf/ASCIICType.h>
#include <wtf/MathExtras.h>
#include <wtf/unicode/Collator.h>

using namespace WTI;

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(StringPrototype);

static EncodedTiValue JSC_HOST_CALL stringProtoFuncToString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncCharAt(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncCharCodeAt(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncConcat(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncIndexOf(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncLastIndexOf(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncMatch(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncReplace(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSearch(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSlice(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSplit(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSubstr(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSubstring(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncToLowerCase(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncToUpperCase(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncLocaleCompare(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncBig(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSmall(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncBlink(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncBold(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncFixed(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncItalics(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncStrike(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSub(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncSup(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncFontcolor(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncFontsize(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncAnchor(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncLink(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncTrim(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncTrimLeft(TiExcState*);
static EncodedTiValue JSC_HOST_CALL stringProtoFuncTrimRight(TiExcState*);

}

#include "StringPrototype.lut.h"

namespace TI {

const ClassInfo StringPrototype::s_info = { "String", &StringObject::s_info, 0, TiExcState::stringTable };

/* Source for StringPrototype.lut.h
@begin stringTable 26
    toString              stringProtoFuncToString          DontEnum|Function       0
    valueOf               stringProtoFuncToString          DontEnum|Function       0
    charAt                stringProtoFuncCharAt            DontEnum|Function       1
    charCodeAt            stringProtoFuncCharCodeAt        DontEnum|Function       1
    concat                stringProtoFuncConcat            DontEnum|Function       1
    indexOf               stringProtoFuncIndexOf           DontEnum|Function       1
    lastIndexOf           stringProtoFuncLastIndexOf       DontEnum|Function       1
    match                 stringProtoFuncMatch             DontEnum|Function       1
    replace               stringProtoFuncReplace           DontEnum|Function       2
    search                stringProtoFuncSearch            DontEnum|Function       1
    slice                 stringProtoFuncSlice             DontEnum|Function       2
    split                 stringProtoFuncSplit             DontEnum|Function       2
    substr                stringProtoFuncSubstr            DontEnum|Function       2
    substring             stringProtoFuncSubstring         DontEnum|Function       2
    toLowerCase           stringProtoFuncToLowerCase       DontEnum|Function       0
    toUpperCase           stringProtoFuncToUpperCase       DontEnum|Function       0
    localeCompare         stringProtoFuncLocaleCompare     DontEnum|Function       1

    # toLocaleLowerCase and toLocaleUpperCase are currently identical to toLowerCase and toUpperCase
    toLocaleLowerCase     stringProtoFuncToLowerCase       DontEnum|Function       0
    toLocaleUpperCase     stringProtoFuncToUpperCase       DontEnum|Function       0

    big                   stringProtoFuncBig               DontEnum|Function       0
    small                 stringProtoFuncSmall             DontEnum|Function       0
    blink                 stringProtoFuncBlink             DontEnum|Function       0
    bold                  stringProtoFuncBold              DontEnum|Function       0
    fixed                 stringProtoFuncFixed             DontEnum|Function       0
    italics               stringProtoFuncItalics           DontEnum|Function       0
    strike                stringProtoFuncStrike            DontEnum|Function       0
    sub                   stringProtoFuncSub               DontEnum|Function       0
    sup                   stringProtoFuncSup               DontEnum|Function       0
    fontcolor             stringProtoFuncFontcolor         DontEnum|Function       1
    fontsize              stringProtoFuncFontsize          DontEnum|Function       1
    anchor                stringProtoFuncAnchor            DontEnum|Function       1
    link                  stringProtoFuncLink              DontEnum|Function       1
    trim                  stringProtoFuncTrim              DontEnum|Function       0
    trimLeft              stringProtoFuncTrimLeft          DontEnum|Function       0
    trimRight             stringProtoFuncTrimRight         DontEnum|Function       0
@end
*/

// ECMA 15.5.4
StringPrototype::StringPrototype(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure)
    : StringObject(exec, structure)
{
    ASSERT(inherits(&s_info));

    putAnonymousValue(exec->globalData(), 0, globalObject);
    // The constructor will be added later, after StringConstructor has been built
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(0), DontDelete | ReadOnly | DontEnum);
}

bool StringPrototype::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<StringObject>(exec, TiExcState::stringTable(exec), this, propertyName, slot);
}

bool StringPrototype::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<StringObject>(exec, TiExcState::stringTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions --------------------------

static NEVER_INLINE UString substituteBackreferencesSlow(const UString& replacement, const UString& source, const int* ovector, RegExp* reg, size_t i)
{
    Vector<UChar> substitutedReplacement;
    int offset = 0;
    do {
        if (i + 1 == replacement.length())
            break;

        UChar ref = replacement[i + 1];
        if (ref == '$') {
            // "$$" -> "$"
            ++i;
            substitutedReplacement.append(replacement.characters() + offset, i - offset);
            offset = i + 1;
            continue;
        }

        int backrefStart;
        int backrefLength;
        int advance = 0;
        if (ref == '&') {
            backrefStart = ovector[0];
            backrefLength = ovector[1] - backrefStart;
        } else if (ref == '`') {
            backrefStart = 0;
            backrefLength = ovector[0];
        } else if (ref == '\'') {
            backrefStart = ovector[1];
            backrefLength = source.length() - backrefStart;
        } else if (reg && ref >= '0' && ref <= '9') {
            // 1- and 2-digit back references are allowed
            unsigned backrefIndex = ref - '0';
            if (backrefIndex > reg->numSubpatterns())
                continue;
            if (replacement.length() > i + 2) {
                ref = replacement[i + 2];
                if (ref >= '0' && ref <= '9') {
                    backrefIndex = 10 * backrefIndex + ref - '0';
                    if (backrefIndex > reg->numSubpatterns())
                        backrefIndex = backrefIndex / 10;   // Fall back to the 1-digit reference
                    else
                        advance = 1;
                }
            }
            if (!backrefIndex)
                continue;
            backrefStart = ovector[2 * backrefIndex];
            backrefLength = ovector[2 * backrefIndex + 1] - backrefStart;
        } else
            continue;

        if (i - offset)
            substitutedReplacement.append(replacement.characters() + offset, i - offset);
        i += 1 + advance;
        offset = i + 1;
        if (backrefStart >= 0)
            substitutedReplacement.append(source.characters() + backrefStart, backrefLength);
    } while ((i = replacement.find('$', i + 1)) != notFound);

    if (replacement.length() - offset)
        substitutedReplacement.append(replacement.characters() + offset, replacement.length() - offset);

    substitutedReplacement.shrinkToFit();
    return UString::adopt(substitutedReplacement);
}

static inline UString substituteBackreferences(const UString& replacement, const UString& source, const int* ovector, RegExp* reg)
{
    size_t i = replacement.find('$', 0);
    if (UNLIKELY(i != notFound))
        return substituteBackreferencesSlow(replacement, source, ovector, reg, i);
    return replacement;
}

static inline int localeCompare(const UString& a, const UString& b)
{
    return Collator::userDefault()->collate(reinterpret_cast<const ::UChar*>(a.characters()), a.length(), reinterpret_cast<const ::UChar*>(b.characters()), b.length());
}

struct StringRange {
public:
    StringRange(int pos, int len)
        : position(pos)
        , length(len)
    {
    }

    StringRange()
    {
    }

    int position;
    int length;
};

static ALWAYS_INLINE TiValue jsSpliceSubstringsWithSeparators(TiExcState* exec, TiString* sourceVal, const UString& source, const StringRange* substringRanges, int rangeCount, const UString* separators, int separatorCount)
{
    if (rangeCount == 1 && separatorCount == 0) {
        int sourceSize = source.length();
        int position = substringRanges[0].position;
        int length = substringRanges[0].length;
        if (position <= 0 && length >= sourceSize)
            return sourceVal;
        // We could call UString::substr, but this would result in redundant checks
        return jsString(exec, StringImpl::create(source.impl(), max(0, position), min(sourceSize, length)));
    }

    int totalLength = 0;
    for (int i = 0; i < rangeCount; i++)
        totalLength += substringRanges[i].length;
    for (int i = 0; i < separatorCount; i++)
        totalLength += separators[i].length();

    if (totalLength == 0)
        return jsString(exec, "");

    UChar* buffer;
    PassRefPtr<StringImpl> impl = StringImpl::tryCreateUninitialized(totalLength, buffer);
    if (!impl)
        return throwOutOfMemoryError(exec);

    int maxCount = max(rangeCount, separatorCount);
    int bufferPos = 0;
    for (int i = 0; i < maxCount; i++) {
        if (i < rangeCount) {
            if (int srcLen = substringRanges[i].length) {
                StringImpl::copyChars(buffer + bufferPos, source.characters() + substringRanges[i].position, srcLen);
                bufferPos += srcLen;
            }
        }
        if (i < separatorCount) {
            if (int sepLen = separators[i].length()) {
                StringImpl::copyChars(buffer + bufferPos, separators[i].characters(), sepLen);
                bufferPos += sepLen;
            }
        }
    }

    return jsString(exec, impl);
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncReplace(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    TiString* sourceVal = thisValue.toThisTiString(exec);
    TiValue pattern = exec->argument(0);
    TiValue replacement = exec->argument(1);
    TiGlobalData* globalData = &exec->globalData();

    UString replacementString;
    CallData callData;
    CallType callType = getCallData(replacement, callData);
    if (callType == CallTypeNone)
        replacementString = replacement.toString(exec);

    if (pattern.inherits(&RegExpObject::s_info)) {
        const UString& source = sourceVal->value(exec);
        unsigned sourceLen = source.length();
        if (exec->hadException())
            return TiValue::encode(TiValue());
        RegExp* reg = asRegExpObject(pattern)->regExp();
        bool global = reg->global();

        RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();

        int lastIndex = 0;
        unsigned startPosition = 0;

        Vector<StringRange, 16> sourceRanges;
        Vector<UString, 16> replacements;

        // This is either a loop (if global is set) or a one-way (if not).
        if (global && callType == CallTypeJS) {
            // reg->numSubpatterns() + 1 for pattern args, + 2 for match start and sourceValue
            int argCount = reg->numSubpatterns() + 1 + 2;
            TiFunction* func = asFunction(replacement);
            CachedCall cachedCall(exec, func, argCount);
            if (exec->hadException())
                return TiValue::encode(jsNull());
            while (true) {
                int matchIndex;
                int matchLen = 0;
                int* ovector;
                regExpConstructor->performMatch(*globalData, reg, source, startPosition, matchIndex, matchLen, &ovector);
                if (matchIndex < 0)
                    break;

                sourceRanges.append(StringRange(lastIndex, matchIndex - lastIndex));

                int completeMatchStart = ovector[0];
                unsigned i = 0;
                for (; i < reg->numSubpatterns() + 1; ++i) {
                    int matchStart = ovector[i * 2];
                    int matchLen = ovector[i * 2 + 1] - matchStart;

                    if (matchStart < 0)
                        cachedCall.setArgument(i, jsUndefined());
                    else
                        cachedCall.setArgument(i, jsSubstring(exec, source, matchStart, matchLen));
                }

                cachedCall.setArgument(i++, jsNumber(completeMatchStart));
                cachedCall.setArgument(i++, sourceVal);

                cachedCall.setThis(exec->globalThisValue());
                TiValue result = cachedCall.call();
                if (LIKELY(result.isString()))
                    replacements.append(asString(result)->value(exec));
                else
                    replacements.append(result.toString(cachedCall.newCallFrame(exec)));
                if (exec->hadException())
                    break;

                lastIndex = matchIndex + matchLen;
                startPosition = lastIndex;

                // special case of empty match
                if (matchLen == 0) {
                    startPosition++;
                    if (startPosition > sourceLen)
                        break;
                }
            }
        } else {
            do {
                int matchIndex;
                int matchLen = 0;
                int* ovector;
                regExpConstructor->performMatch(*globalData, reg, source, startPosition, matchIndex, matchLen, &ovector);
                if (matchIndex < 0)
                    break;

                if (callType != CallTypeNone) {
                    sourceRanges.append(StringRange(lastIndex, matchIndex - lastIndex));

                    int completeMatchStart = ovector[0];
                    MarkedArgumentBuffer args;

                    for (unsigned i = 0; i < reg->numSubpatterns() + 1; ++i) {
                        int matchStart = ovector[i * 2];
                        int matchLen = ovector[i * 2 + 1] - matchStart;
 
                        if (matchStart < 0)
                            args.append(jsUndefined());
                        else
                            args.append(jsSubstring(exec, source, matchStart, matchLen));
                    }

                    args.append(jsNumber(completeMatchStart));
                    args.append(sourceVal);

                    replacements.append(call(exec, replacement, callType, callData, exec->globalThisValue(), args).toString(exec));
                    if (exec->hadException())
                        break;
                } else {
                    int replLen = replacementString.length();
                    if (lastIndex < matchIndex || replLen) {
                        sourceRanges.append(StringRange(lastIndex, matchIndex - lastIndex));
 
                        if (replLen)
                            replacements.append(substituteBackreferences(replacementString, source, ovector, reg));
                        else
                            replacements.append(UString());
                    }
                }

                lastIndex = matchIndex + matchLen;
                startPosition = lastIndex;

                // special case of empty match
                if (matchLen == 0) {
                    startPosition++;
                    if (startPosition > sourceLen)
                        break;
                }
            } while (global);
        }

        if (!lastIndex && replacements.isEmpty())
            return TiValue::encode(sourceVal);

        if (static_cast<unsigned>(lastIndex) < sourceLen)
            sourceRanges.append(StringRange(lastIndex, sourceLen - lastIndex));

        return TiValue::encode(jsSpliceSubstringsWithSeparators(exec, sourceVal, source, sourceRanges.data(), sourceRanges.size(), replacements.data(), replacements.size()));
    }

    // Not a regular expression, so treat the pattern as a string.

    UString patternString = pattern.toString(exec);
    // Special case for single character patterns without back reference replacement
    if (patternString.length() == 1 && callType == CallTypeNone && replacementString.find('$', 0) == notFound)
        return TiValue::encode(sourceVal->replaceCharacter(exec, patternString[0], replacementString));

    const UString& source = sourceVal->value(exec);
    size_t matchPos = source.find(patternString);

    if (matchPos == notFound)
        return TiValue::encode(sourceVal);

    int matchLen = patternString.length();
    if (callType != CallTypeNone) {
        MarkedArgumentBuffer args;
        args.append(jsSubstring(exec, source, matchPos, matchLen));
        args.append(jsNumber(matchPos));
        args.append(sourceVal);

        replacementString = call(exec, replacement, callType, callData, exec->globalThisValue(), args).toString(exec);
    }
    
    size_t matchEnd = matchPos + matchLen;
    int ovector[2] = { matchPos, matchEnd };
    return TiValue::encode(jsString(exec, source.substringSharingImpl(0, matchPos), substituteBackreferences(replacementString, source, ovector, 0), source.substringSharingImpl(matchEnd)));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncToString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    // Also used for valueOf.

    if (thisValue.isString())
        return TiValue::encode(thisValue);

    if (thisValue.inherits(&StringObject::s_info))
        return TiValue::encode(asStringObject(thisValue)->internalValue());

    return throwVMTypeError(exec);
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncCharAt(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    unsigned len = s.length();
    TiValue a0 = exec->argument(0);
    if (a0.isUInt32()) {
        uint32_t i = a0.asUInt32();
        if (i < len)
            return TiValue::encode(jsSingleCharacterSubstring(exec, s, i));
        return TiValue::encode(jsEmptyString(exec));
    }
    double dpos = a0.toInteger(exec);
    if (dpos >= 0 && dpos < len)
        return TiValue::encode(jsSingleCharacterSubstring(exec, s, static_cast<unsigned>(dpos)));
    return TiValue::encode(jsEmptyString(exec));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncCharCodeAt(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    unsigned len = s.length();
    TiValue a0 = exec->argument(0);
    if (a0.isUInt32()) {
        uint32_t i = a0.asUInt32();
        if (i < len)
            return TiValue::encode(jsNumber(s.characters()[i]));
        return TiValue::encode(jsNaN());
    }
    double dpos = a0.toInteger(exec);
    if (dpos >= 0 && dpos < len)
        return TiValue::encode(jsNumber(s[static_cast<int>(dpos)]));
    return TiValue::encode(jsNaN());
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncConcat(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isString() && (exec->argumentCount() == 1)) {
        TiValue v = exec->argument(0);
        return TiValue::encode(v.isString()
            ? jsString(exec, asString(thisValue), asString(v))
            : jsString(exec, asString(thisValue), v.toString(exec)));
    }
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    return TiValue::encode(jsString(exec, thisValue));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncIndexOf(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    int len = s.length();

    TiValue a0 = exec->argument(0);
    TiValue a1 = exec->argument(1);
    UString u2 = a0.toString(exec);
    int pos;
    if (a1.isUndefined())
        pos = 0;
    else if (a1.isUInt32())
        pos = min<uint32_t>(a1.asUInt32(), len);
    else {
        double dpos = a1.toInteger(exec);
        if (dpos < 0)
            dpos = 0;
        else if (dpos > len)
            dpos = len;
        pos = static_cast<int>(dpos);
    }

    size_t result = s.find(u2, pos);
    if (result == notFound)
        return TiValue::encode(jsNumber(-1));
    return TiValue::encode(jsNumber(result));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncLastIndexOf(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    int len = s.length();

    TiValue a0 = exec->argument(0);
    TiValue a1 = exec->argument(1);

    UString u2 = a0.toString(exec);
    double dpos = a1.toIntegerPreserveNaN(exec);
    if (dpos < 0)
        dpos = 0;
    else if (!(dpos <= len)) // true for NaN
        dpos = len;
#if OS(SYMBIAN)
    // Work around for broken NaN compare operator
    else if (isnan(dpos))
        dpos = len;
#endif

    size_t result = s.reverseFind(u2, static_cast<unsigned>(dpos));
    if (result == notFound)
        return TiValue::encode(jsNumber(-1));
    return TiValue::encode(jsNumber(result));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncMatch(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    TiGlobalData* globalData = &exec->globalData();

    TiValue a0 = exec->argument(0);

    RegExp* reg;
    if (a0.inherits(&RegExpObject::s_info))
        reg = asRegExpObject(a0)->regExp();
    else {
        /*
         *  ECMA 15.5.4.12 String.prototype.search (regexp)
         *  If regexp is not an object whose [[Class]] property is "RegExp", it is
         *  replaced with the result of the expression new RegExp(regexp).
         */
        reg = RegExp::create(&exec->globalData(), a0.toString(exec), NoFlags);
    }
    RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();
    int pos;
    int matchLength = 0;
    regExpConstructor->performMatch(*globalData, reg, s, 0, pos, matchLength);
    if (!(reg->global())) {
        // case without 'g' flag is handled like RegExp.prototype.exec
        if (pos < 0)
            return TiValue::encode(jsNull());
        return TiValue::encode(regExpConstructor->arrayOfMatches(exec));
    }

    // return array of matches
    MarkedArgumentBuffer list;
    while (pos >= 0) {
        list.append(jsSubstring(exec, s, pos, matchLength));
        pos += matchLength == 0 ? 1 : matchLength;
        regExpConstructor->performMatch(*globalData, reg, s, pos, pos, matchLength);
    }
    if (list.isEmpty()) {
        // if there are no matches at all, it's important to return
        // Null instead of an empty array, because this matches
        // other browsers and because Null is a false value.
        return TiValue::encode(jsNull());
    }

    return TiValue::encode(constructArray(exec, list));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSearch(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    TiGlobalData* globalData = &exec->globalData();

    TiValue a0 = exec->argument(0);

    RegExp* reg;
    if (a0.inherits(&RegExpObject::s_info))
        reg = asRegExpObject(a0)->regExp();
    else { 
        /*
         *  ECMA 15.5.4.12 String.prototype.search (regexp)
         *  If regexp is not an object whose [[Class]] property is "RegExp", it is
         *  replaced with the result of the expression new RegExp(regexp).
         */
        reg = RegExp::create(&exec->globalData(), a0.toString(exec), NoFlags);
    }
    RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();
    int pos;
    int matchLength = 0;
    regExpConstructor->performMatch(*globalData, reg, s, 0, pos, matchLength);
    return TiValue::encode(jsNumber(pos));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSlice(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    int len = s.length();

    TiValue a0 = exec->argument(0);
    TiValue a1 = exec->argument(1);

    // The arg processing is very much like ArrayProtoFunc::Slice
    double start = a0.toInteger(exec);
    double end = a1.isUndefined() ? len : a1.toInteger(exec);
    double from = start < 0 ? len + start : start;
    double to = end < 0 ? len + end : end;
    if (to > from && to > 0 && from < len) {
        if (from < 0)
            from = 0;
        if (to > len)
            to = len;
        return TiValue::encode(jsSubstring(exec, s, static_cast<unsigned>(from), static_cast<unsigned>(to) - static_cast<unsigned>(from)));
    }

    return TiValue::encode(jsEmptyString(exec));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSplit(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    UString s = thisValue.toThisString(exec);
    TiGlobalData* globalData = &exec->globalData();

    TiValue a0 = exec->argument(0);
    TiValue a1 = exec->argument(1);

    TiArray* result = constructEmptyArray(exec);
    unsigned i = 0;
    unsigned p0 = 0;
    unsigned limit = a1.isUndefined() ? 0xFFFFFFFFU : a1.toUInt32(exec);
    if (a0.inherits(&RegExpObject::s_info)) {
        RegExp* reg = asRegExpObject(a0)->regExp();
        if (s.isEmpty() && reg->match(*globalData, s, 0) >= 0) {
            // empty string matched by regexp -> empty array
            return TiValue::encode(result);
        }
        unsigned pos = 0;
        while (i != limit && pos < s.length()) {
            Vector<int, 32> ovector;
            int mpos = reg->match(*globalData, s, pos, &ovector);
            if (mpos < 0)
                break;
            int mlen = ovector[1] - ovector[0];
            pos = mpos + (mlen == 0 ? 1 : mlen);
            if (static_cast<unsigned>(mpos) != p0 || mlen) {
                result->put(exec, i++, jsSubstring(exec, s, p0, mpos - p0));
                p0 = mpos + mlen;
            }
            for (unsigned si = 1; si <= reg->numSubpatterns(); ++si) {
                int spos = ovector[si * 2];
                if (spos < 0)
                    result->put(exec, i++, jsUndefined());
                else
                    result->put(exec, i++, jsSubstring(exec, s, spos, ovector[si * 2 + 1] - spos));
            }
        }
    } else {
        UString u2 = a0.toString(exec);
        if (u2.isEmpty()) {
            if (s.isEmpty()) {
                // empty separator matches empty string -> empty array
                return TiValue::encode(result);
            }
            while (i != limit && p0 < s.length() - 1)
                result->put(exec, i++, jsSingleCharacterSubstring(exec, s, p0++));
        } else {
            size_t pos;
            while (i != limit && (pos = s.find(u2, p0)) != notFound) {
                result->put(exec, i++, jsSubstring(exec, s, p0, pos - p0));
                p0 = pos + u2.length();
            }
        }
    }

    // add remaining string
    if (i != limit)
        result->put(exec, i++, jsSubstring(exec, s, p0, s.length() - p0));

    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSubstr(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    unsigned len;
    TiString* jsString = 0;
    UString uString;
    if (thisValue.isString()) {
        jsString = static_cast<TiString*>(thisValue.asCell());
        len = jsString->length();
    } else {
        uString = thisValue.toThisObject(exec)->toString(exec);
        len = uString.length();
    }

    TiValue a0 = exec->argument(0);
    TiValue a1 = exec->argument(1);

    double start = a0.toInteger(exec);
    double length = a1.isUndefined() ? len : a1.toInteger(exec);
    if (start >= len || length <= 0)
        return TiValue::encode(jsEmptyString(exec));
    if (start < 0) {
        start += len;
        if (start < 0)
            start = 0;
    }
    if (start + length > len)
        length = len - start;
    unsigned substringStart = static_cast<unsigned>(start);
    unsigned substringLength = static_cast<unsigned>(length);
    if (jsString)
        return TiValue::encode(jsSubstring(exec, jsString, substringStart, substringLength));
    return TiValue::encode(jsSubstring(exec, uString, substringStart, substringLength));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSubstring(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    int len;
    TiString* jsString = 0;
    UString uString;
    if (thisValue.isString()) {
        jsString = static_cast<TiString*>(thisValue.asCell());
        len = jsString->length();
    } else {
        uString = thisValue.toThisObject(exec)->toString(exec);
        len = uString.length();
    }

    TiValue a0 = exec->argument(0);
    TiValue a1 = exec->argument(1);

    double start = a0.toNumber(exec);
    double end;
    if (!(start >= 0)) // check for negative values or NaN
        start = 0;
    else if (start > len)
        start = len;
    if (a1.isUndefined())
        end = len;
    else { 
        end = a1.toNumber(exec);
        if (!(end >= 0)) // check for negative values or NaN
            end = 0;
        else if (end > len)
            end = len;
    }
    if (start > end) {
        double temp = end;
        end = start;
        start = temp;
    }
    unsigned substringStart = static_cast<unsigned>(start);
    unsigned substringLength = static_cast<unsigned>(end) - substringStart;
    if (jsString)
        return TiValue::encode(jsSubstring(exec, jsString, substringStart, substringLength));
    return TiValue::encode(jsSubstring(exec, uString, substringStart, substringLength));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncToLowerCase(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    TiString* sVal = thisValue.toThisTiString(exec);
    const UString& s = sVal->value(exec);

    int sSize = s.length();
    if (!sSize)
        return TiValue::encode(sVal);

    const UChar* sData = s.characters();
    Vector<UChar> buffer(sSize);

    UChar ored = 0;
    for (int i = 0; i < sSize; i++) {
        UChar c = sData[i];
        ored |= c;
        buffer[i] = toASCIILower(c);
    }
    if (!(ored & ~0x7f))
        return TiValue::encode(jsString(exec, UString::adopt(buffer)));

    bool error;
    int length = Unicode::toLower(buffer.data(), sSize, sData, sSize, &error);
    if (error) {
        buffer.resize(length);
        length = Unicode::toLower(buffer.data(), length, sData, sSize, &error);
        if (error)
            return TiValue::encode(sVal);
    }
    if (length == sSize) {
        if (memcmp(buffer.data(), sData, length * sizeof(UChar)) == 0)
            return TiValue::encode(sVal);
    } else
        buffer.resize(length);
    return TiValue::encode(jsString(exec, UString::adopt(buffer)));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncToUpperCase(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);
    TiString* sVal = thisValue.toThisTiString(exec);
    const UString& s = sVal->value(exec);

    int sSize = s.length();
    if (!sSize)
        return TiValue::encode(sVal);

    const UChar* sData = s.characters();
    Vector<UChar> buffer(sSize);

    UChar ored = 0;
    for (int i = 0; i < sSize; i++) {
        UChar c = sData[i];
        ored |= c;
        buffer[i] = toASCIIUpper(c);
    }
    if (!(ored & ~0x7f))
        return TiValue::encode(jsString(exec, UString::adopt(buffer)));

    bool error;
    int length = Unicode::toUpper(buffer.data(), sSize, sData, sSize, &error);
    if (error) {
        buffer.resize(length);
        length = Unicode::toUpper(buffer.data(), length, sData, sSize, &error);
        if (error)
            return TiValue::encode(sVal);
    }
    if (length == sSize) {
        if (memcmp(buffer.data(), sData, length * sizeof(UChar)) == 0)
            return TiValue::encode(sVal);
    } else
        buffer.resize(length);
    return TiValue::encode(jsString(exec, UString::adopt(buffer)));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncLocaleCompare(TiExcState* exec)
{
    if (exec->argumentCount() < 1)
      return TiValue::encode(jsNumber(0));

    TiValue thisValue = exec->hostThisValue();
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwVMTypeError(exec);

    UString s = thisValue.toThisString(exec);
    TiValue a0 = exec->argument(0);
    return TiValue::encode(jsNumber(localeCompare(s, a0.toString(exec))));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncBig(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<big>", s, "</big>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSmall(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<small>", s, "</small>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncBlink(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<blink>", s, "</blink>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncBold(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<b>", s, "</b>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncFixed(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<tt>", s, "</tt>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncItalics(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<i>", s, "</i>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncStrike(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<strike>", s, "</strike>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSub(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<sub>", s, "</sub>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncSup(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    return TiValue::encode(jsMakeNontrivialString(exec, "<sup>", s, "</sup>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncFontcolor(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    TiValue a0 = exec->argument(0);
    return TiValue::encode(jsMakeNontrivialString(exec, "<font color=\"", a0.toString(exec), "\">", s, "</font>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncFontsize(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    TiValue a0 = exec->argument(0);

    uint32_t smallInteger;
    if (a0.getUInt32(smallInteger) && smallInteger <= 9) {
        unsigned stringSize = s.length();
        unsigned bufferSize = 22 + stringSize;
        UChar* buffer;
        PassRefPtr<StringImpl> impl = StringImpl::tryCreateUninitialized(bufferSize, buffer);
        if (!impl)
            return TiValue::encode(jsUndefined());
        buffer[0] = '<';
        buffer[1] = 'f';
        buffer[2] = 'o';
        buffer[3] = 'n';
        buffer[4] = 't';
        buffer[5] = ' ';
        buffer[6] = 's';
        buffer[7] = 'i';
        buffer[8] = 'z';
        buffer[9] = 'e';
        buffer[10] = '=';
        buffer[11] = '"';
        buffer[12] = '0' + smallInteger;
        buffer[13] = '"';
        buffer[14] = '>';
        memcpy(&buffer[15], s.characters(), stringSize * sizeof(UChar));
        buffer[15 + stringSize] = '<';
        buffer[16 + stringSize] = '/';
        buffer[17 + stringSize] = 'f';
        buffer[18 + stringSize] = 'o';
        buffer[19 + stringSize] = 'n';
        buffer[20 + stringSize] = 't';
        buffer[21 + stringSize] = '>';
        return TiValue::encode(jsNontrivialString(exec, impl));
    }

    return TiValue::encode(jsMakeNontrivialString(exec, "<font size=\"", a0.toString(exec), "\">", s, "</font>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncAnchor(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    TiValue a0 = exec->argument(0);
    return TiValue::encode(jsMakeNontrivialString(exec, "<a name=\"", a0.toString(exec), "\">", s, "</a>"));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncLink(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    UString s = thisValue.toThisString(exec);
    TiValue a0 = exec->argument(0);
    UString linkText = a0.toString(exec);

    unsigned linkTextSize = linkText.length();
    unsigned stringSize = s.length();
    unsigned bufferSize = 15 + linkTextSize + stringSize;
    UChar* buffer;
    PassRefPtr<StringImpl> impl = StringImpl::tryCreateUninitialized(bufferSize, buffer);
    if (!impl)
        return TiValue::encode(jsUndefined());
    buffer[0] = '<';
    buffer[1] = 'a';
    buffer[2] = ' ';
    buffer[3] = 'h';
    buffer[4] = 'r';
    buffer[5] = 'e';
    buffer[6] = 'f';
    buffer[7] = '=';
    buffer[8] = '"';
    memcpy(&buffer[9], linkText.characters(), linkTextSize * sizeof(UChar));
    buffer[9 + linkTextSize] = '"';
    buffer[10 + linkTextSize] = '>';
    memcpy(&buffer[11 + linkTextSize], s.characters(), stringSize * sizeof(UChar));
    buffer[11 + linkTextSize + stringSize] = '<';
    buffer[12 + linkTextSize + stringSize] = '/';
    buffer[13 + linkTextSize + stringSize] = 'a';
    buffer[14 + linkTextSize + stringSize] = '>';
    return TiValue::encode(jsNontrivialString(exec, impl));
}

enum {
    TrimLeft = 1,
    TrimRight = 2
};

static inline bool isTrimWhitespace(UChar c)
{
    return isStrWhiteSpace(c) || c == 0x200b;
}

static inline TiValue trimString(TiExcState* exec, TiValue thisValue, int trimKind)
{
    if (thisValue.isUndefinedOrNull()) // CheckObjectCoercible
        return throwTypeError(exec);
    UString str = thisValue.toThisString(exec);
    unsigned left = 0;
    if (trimKind & TrimLeft) {
        while (left < str.length() && isTrimWhitespace(str[left]))
            left++;
    }
    unsigned right = str.length();
    if (trimKind & TrimRight) {
        while (right > left && isTrimWhitespace(str[right - 1]))
            right--;
    }

    // Don't gc allocate a new string if we don't have to.
    if (left == 0 && right == str.length() && thisValue.isString())
        return thisValue;

    return jsString(exec, str.substringSharingImpl(left, right - left));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncTrim(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(trimString(exec, thisValue, TrimLeft | TrimRight));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncTrimLeft(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(trimString(exec, thisValue, TrimLeft));
}

EncodedTiValue JSC_HOST_CALL stringProtoFuncTrimRight(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    return TiValue::encode(trimString(exec, thisValue, TrimRight));
}
    
    
} // namespace TI
