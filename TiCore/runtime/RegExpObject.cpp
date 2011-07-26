/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
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
#include "RegExpObject.h"

#include "Error.h"
#include "TiArray.h"
#include "TiGlobalObject.h"
#include "TiString.h"
#include "Lookup.h"
#include "RegExpConstructor.h"
#include "RegExpPrototype.h"

namespace TI {

static TiValue regExpObjectGlobal(TiExcState*, TiValue, const Identifier&);
static TiValue regExpObjectIgnoreCase(TiExcState*, TiValue, const Identifier&);
static TiValue regExpObjectMultiline(TiExcState*, TiValue, const Identifier&);
static TiValue regExpObjectSource(TiExcState*, TiValue, const Identifier&);
static TiValue regExpObjectLastIndex(TiExcState*, TiValue, const Identifier&);
static void setRegExpObjectLastIndex(TiExcState*, TiObject*, TiValue);

} // namespace TI

#include "RegExpObject.lut.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(RegExpObject);

const ClassInfo RegExpObject::info = { "RegExp", 0, 0, TiExcState::regExpTable };

/* Source for RegExpObject.lut.h
@begin regExpTable
    global        regExpObjectGlobal       DontDelete|ReadOnly|DontEnum
    ignoreCase    regExpObjectIgnoreCase   DontDelete|ReadOnly|DontEnum
    multiline     regExpObjectMultiline    DontDelete|ReadOnly|DontEnum
    source        regExpObjectSource       DontDelete|ReadOnly|DontEnum
    lastIndex     regExpObjectLastIndex    DontDelete|DontEnum
@end
*/

RegExpObject::RegExpObject(NonNullPassRefPtr<Structure> structure, NonNullPassRefPtr<RegExp> regExp)
    : TiObject(structure)
    , d(new RegExpObjectData(regExp, 0))
{
}

RegExpObject::~RegExpObject()
{
}

bool RegExpObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<RegExpObject, TiObject>(exec, TiExcState::regExpTable(exec), this, propertyName, slot);
}

bool RegExpObject::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<RegExpObject, TiObject>(exec, TiExcState::regExpTable(exec), this, propertyName, descriptor);
}

TiValue regExpObjectGlobal(TiExcState*, TiValue slotBase, const Identifier&)
{
    return jsBoolean(asRegExpObject(slotBase)->regExp()->global());
}

TiValue regExpObjectIgnoreCase(TiExcState*, TiValue slotBase, const Identifier&)
{
    return jsBoolean(asRegExpObject(slotBase)->regExp()->ignoreCase());
}
 
TiValue regExpObjectMultiline(TiExcState*, TiValue slotBase, const Identifier&)
{            
    return jsBoolean(asRegExpObject(slotBase)->regExp()->multiline());
}

TiValue regExpObjectSource(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return jsString(exec, asRegExpObject(slotBase)->regExp()->pattern());
}

TiValue regExpObjectLastIndex(TiExcState* exec, TiValue slotBase, const Identifier&)
{
    return jsNumber(exec, asRegExpObject(slotBase)->lastIndex());
}

void RegExpObject::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    lookupPut<RegExpObject, TiObject>(exec, propertyName, value, TiExcState::regExpTable(exec), this, slot);
}

void setRegExpObjectLastIndex(TiExcState* exec, TiObject* baseObject, TiValue value)
{
    asRegExpObject(baseObject)->setLastIndex(value.toInteger(exec));
}

TiValue RegExpObject::test(TiExcState* exec, const ArgList& args)
{
    return jsBoolean(match(exec, args));
}

TiValue RegExpObject::exec(TiExcState* exec, const ArgList& args)
{
    if (match(exec, args))
        return exec->lexicalGlobalObject()->regExpConstructor()->arrayOfMatches(exec);
    return jsNull();
}

static TiValue JSC_HOST_CALL callRegExpObject(TiExcState* exec, TiObject* function, TiValue, const ArgList& args)
{
    return asRegExpObject(function)->exec(exec, args);
}

CallType RegExpObject::getCallData(CallData& callData)
{
    callData.native.function = callRegExpObject;
    return CallTypeHost;
}

// Shared implementation used by test and exec.
bool RegExpObject::match(TiExcState* exec, const ArgList& args)
{
    RegExpConstructor* regExpConstructor = exec->lexicalGlobalObject()->regExpConstructor();

    UString input = args.isEmpty() ? regExpConstructor->input() : args.at(0).toString(exec);
    if (input.isNull()) {
        throwError(exec, GeneralError, makeString("No input to ", toString(exec), "."));
        return false;
    }

    if (!regExp()->global()) {
        int position;
        int length;
        regExpConstructor->performMatch(d->regExp.get(), input, 0, position, length);
        return position >= 0;
    }

    if (d->lastIndex < 0 || d->lastIndex > input.size()) {
        d->lastIndex = 0;
        return false;
    }

    int position;
    int length = 0;
    regExpConstructor->performMatch(d->regExp.get(), input, static_cast<int>(d->lastIndex), position, length);
    if (position < 0) {
        d->lastIndex = 0;
        return false;
    }

    d->lastIndex = position + length;
    return true;
}

} // namespace TI
