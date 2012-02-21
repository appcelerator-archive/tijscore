/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
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
#include "RegExpConstructor.h"

#include "ArrayPrototype.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "TiArray.h"
#include "TiFunction.h"
#include "TiString.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "RegExpMatchesArray.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "RegExp.h"
#include "RegExpCache.h"
#include "UStringConcatenate.h"
#include <wtf/PassOwnPtr.h>

#include <wtf/PassOwnPtr.h>

namespace TI {

static TiValue regExpConstructorInput(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorMultiline(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorLastMatch(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorLastParen(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorLeftContext(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorRightContext(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar1(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar2(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar3(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar4(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar5(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar6(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar7(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar8(TiExcState*, TiValue, const Identifier&);
static TiValue regExpConstructorDollar9(TiExcState*, TiValue, const Identifier&);

static void setRegExpConstructorInput(TiExcState*, TiObject*, TiValue);
static void setRegExpConstructorMultiline(TiExcState*, TiObject*, TiValue);

} // namespace TI

#include "RegExpConstructor.lut.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(RegExpConstructor);

const ClassInfo RegExpConstructor::s_info = { "Function", &InternalFunction::s_info, 0, TiExcState::regExpConstructorTable };

/* Source for RegExpConstructor.lut.h
@begin regExpConstructorTable
    input           regExpConstructorInput          None
    $_              regExpConstructorInput          DontEnum
    multiline       regExpConstructorMultiline      None
    $*              regExpConstructorMultiline      DontEnum
    lastMatch       regExpConstructorLastMatch      DontDelete|ReadOnly
    $&              regExpConstructorLastMatch      DontDelete|ReadOnly|DontEnum
    lastParen       regExpConstructorLastParen      DontDelete|ReadOnly
    $+              regExpConstructorLastParen      DontDelete|ReadOnly|DontEnum
    leftContext     regExpConstructorLeftContext    DontDelete|ReadOnly
    $`              regExpConstructorLeftContext    DontDelete|ReadOnly|DontEnum
    rightContext    regExpConstructorRightContext   DontDelete|ReadOnly
    $'              regExpConstructorRightContext   DontDelete|ReadOnly|DontEnum
    $1              regExpConstructorDollar1        DontDelete|ReadOnly
    $2              regExpConstructorDollar2        DontDelete|ReadOnly
    $3              regExpConstructorDollar3        DontDelete|ReadOnly
    $4              regExpConstructorDollar4        DontDelete|ReadOnly
    $5              regExpConstructorDollar5        DontDelete|ReadOnly
    $6              regExpConstructorDollar6        DontDelete|ReadOnly
    $7              regExpConstructorDollar7        DontDelete|ReadOnly
    $8              regExpConstructorDollar8        DontDelete|ReadOnly
    $9              regExpConstructorDollar9        DontDelete|ReadOnly
@end
*/

RegExpConstructor::RegExpConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, RegExpPrototype* regExpPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, "RegExp"))
    , d(adoptPtr(new RegExpConstructorPrivate))
{
    ASSERT(inherits(&s_info));

    // ECMA 15.10.5.1 RegExp.prototype
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, regExpPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(2), ReadOnly | DontDelete | DontEnum);
}

RegExpMatchesArray::RegExpMatchesArray(TiExcState* exec, RegExpConstructorPrivate* data)
    : TiArray(exec->globalData(), exec->lexicalGlobalObject()->regExpMatchesArrayStructure(), data->lastNumSubPatterns + 1, CreateInitialized)
{
    RegExpConstructorPrivate* d = new RegExpConstructorPrivate;
    d->input = data->lastInput;
    d->lastInput = data->lastInput;
    d->lastNumSubPatterns = data->lastNumSubPatterns;
    unsigned offsetVectorSize = (data->lastNumSubPatterns + 1) * 2; // only copying the result part of the vector
    d->lastOvector().resize(offsetVectorSize);
    memcpy(d->lastOvector().data(), data->lastOvector().data(), offsetVectorSize * sizeof(int));
    // d->multiline is not needed, and remains uninitialized

    setSubclassData(d);
}

RegExpMatchesArray::~RegExpMatchesArray()
{
    delete static_cast<RegExpConstructorPrivate*>(subclassData());
}

void RegExpMatchesArray::fillArrayInstance(TiExcState* exec)
{
    RegExpConstructorPrivate* d = static_cast<RegExpConstructorPrivate*>(subclassData());
    ASSERT(d);

    unsigned lastNumSubpatterns = d->lastNumSubPatterns;

    for (unsigned i = 0; i <= lastNumSubpatterns; ++i) {
        int start = d->lastOvector()[2 * i];
        if (start >= 0)
            TiArray::put(exec, i, jsSubstring(exec, d->lastInput, start, d->lastOvector()[2 * i + 1] - start));
        else
            TiArray::put(exec, i, jsUndefined());
    }

    PutPropertySlot slot;
    TiArray::put(exec, exec->propertyNames().index, jsNumber(d->lastOvector()[0]), slot);
    TiArray::put(exec, exec->propertyNames().input, jsString(exec, d->input), slot);

    delete d;
    setSubclassData(0);
}

TiObject* RegExpConstructor::arrayOfMatches(TiExcState* exec) const
{
    return new (exec) RegExpMatchesArray(exec, d.get());
}

TiValue RegExpConstructor::getBackref(TiExcState* exec, unsigned i) const
{
    if (!d->lastOvector().isEmpty() && i <= d->lastNumSubPatterns) {
        int start = d->lastOvector()[2 * i];
        if (start >= 0)
            return jsSubstring(exec, d->lastInput, start, d->lastOvector()[2 * i + 1] - start);
    }
    return jsEmptyString(exec);
}

TiValue RegExpConstructor::getLastParen(TiExcState* exec) const
{
    unsigned i = d->lastNumSubPatterns;
    if (i > 0) {
        ASSERT(!d->lastOvector().isEmpty());
        int start = d->lastOvector()[2 * i];
        if (start >= 0)
            return jsSubstring(exec, d->lastInput, start, d->lastOvector()[2 * i + 1] - start);
    }
    return jsEmptyString(exec);
}

TiValue RegExpConstructor::getLeftContext(TiExcState* exec) const
{
    if (!d->lastOvector().isEmpty())
        return jsSubstring(exec, d->lastInput, 0, d->lastOvector()[0]);
    return jsEmptyString(exec);
}

TiValue RegExpConstructor::getRightContext(TiExcState* exec) const
{
    if (!d->lastOvector().isEmpty())
        return jsSubstring(exec, d->lastInput, d->lastOvector()[1], d->lastInput.length() - d->lastOvector()[1]);
    return jsEmptyString(exec);
}
    
bool RegExpConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<RegExpConstructor, InternalFunction>(exec, TiExcState::regExpConstructorTable(exec), this, propertyName, slot);
}

bool RegExpConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<RegExpConstructor, InternalFunction>(exec, TiExcState::regExpConstructorTable(exec), this, propertyName, descriptor);
}

TiValue regExpConstructorDollar1(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 1);
}

TiValue regExpConstructorDollar2(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 2);
}

TiValue regExpConstructorDollar3(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 3);
}

TiValue regExpConstructorDollar4(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 4);
}

TiValue regExpConstructorDollar5(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 5);
}

TiValue regExpConstructorDollar6(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 6);
}

TiValue regExpConstructorDollar7(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 7);
}

TiValue regExpConstructorDollar8(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 8);
}

TiValue regExpConstructorDollar9(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 9);
}

TiValue regExpConstructorInput(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return jsString(exec, asRegExpConstructor(slotBase)->input());
}

TiValue regExpConstructorMultiline(TiExcState*, TiValue slotBase, const Identifier&)
{
    return jsBoolean(asRegExpConstructor(slotBase)->multiline());
}

TiValue regExpConstructorLastMatch(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getBackref(exec, 0);
}

TiValue regExpConstructorLastParen(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getLastParen(exec);
}

TiValue regExpConstructorLeftContext(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getLeftContext(exec);
}

TiValue regExpConstructorRightContext(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return asRegExpConstructor(slotBase)->getRightContext(exec);
}

void RegExpConstructor::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    lookupPut<RegExpConstructor, InternalFunction>(exec, propertyName, value, TiExcState::regExpConstructorTable(exec), this, slot);
}

void setRegExpConstructorInput(TiExcState* exec, TiObject* baseObject, TiValue value)
{
    asRegExpConstructor(baseObject)->setInput(value.toString(exec));
}

void setRegExpConstructorMultiline(TiExcState* exec, TiObject* baseObject, TiValue value)
{
    asRegExpConstructor(baseObject)->setMultiline(value.toBoolean(exec));
}

// ECMA 15.10.4
TiObject* constructRegExp(TiExcState* exec, TiGlobalObject* globalObject, const ArgList& args)
{
    TiValue arg0 = args.at(0);
    TiValue arg1 = args.at(1);

    if (arg0.inherits(&RegExpObject::s_info)) {
        if (!arg1.isUndefined())
            return throwError(exec, createTypeError(exec, "Cannot supply flags when constructing one RegExp from another."));
        return asObject(arg0);
    }

    UString pattern = arg0.isUndefined() ? UString("") : arg0.toString(exec);
    if (exec->hadException())
        return 0;

    RegExpFlags flags = NoFlags;
    if (!arg1.isUndefined()) {
        flags = regExpFlags(arg1.toString(exec));
        if (exec->hadException())
            return 0;
        if (flags == InvalidFlags)
            return throwError(exec, createSyntaxError(exec, "Invalid flags supplied to RegExp constructor."));
    }

    RegExp* regExp = RegExp::create(&exec->globalData(), pattern, flags);
    if (!regExp->isValid())
        return throwError(exec, createSyntaxError(exec, regExp->errorMessage()));
    return new (exec) RegExpObject(exec->lexicalGlobalObject(), globalObject->regExpStructure(), regExp);
}

static EncodedTiValue JSC_HOST_CALL constructWithRegExpConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructRegExp(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

ConstructType RegExpConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithRegExpConstructor;
    return ConstructTypeHost;
}

// ECMA 15.10.3
static EncodedTiValue JSC_HOST_CALL callRegExpConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructRegExp(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

CallType RegExpConstructor::getCallData(CallData& callData)
{
    callData.native.function = callRegExpConstructor;
    return CallTypeHost;
}

void RegExpConstructor::setInput(const UString& input)
{
    d->input = input;
}

const UString& RegExpConstructor::input() const
{
    // Can detect a distinct initial state that is invisible to Ti, by checking for null
    // state (since jsString turns null strings to empty strings).
    return d->input;
}

void RegExpConstructor::setMultiline(bool multiline)
{
    d->multiline = multiline;
}

bool RegExpConstructor::multiline() const
{
    return d->multiline;
}

} // namespace TI
