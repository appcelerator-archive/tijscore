/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
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

#include "Error.h"
#include "Operations.h"
#include "RegExpMatchesArray.h"
#include "RegExpPrototype.h"

namespace TI {

static EncodedTiValue regExpConstructorInput(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorMultiline(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorLastMatch(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorLastParen(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorLeftContext(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorRightContext(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar1(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar2(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar3(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar4(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar5(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar6(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar7(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar8(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);
static EncodedTiValue regExpConstructorDollar9(ExecState*, EncodedTiValue, EncodedTiValue, PropertyName);

static void setRegExpConstructorInput(ExecState*, JSObject*, TiValue);
static void setRegExpConstructorMultiline(ExecState*, JSObject*, TiValue);

} // namespace TI

#include "RegExpConstructor.lut.h"

namespace TI {

const ClassInfo RegExpConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::regExpConstructorTable, CREATE_METHOD_TABLE(RegExpConstructor) };

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

RegExpConstructor::RegExpConstructor(VM& vm, Structure* structure, RegExpPrototype* regExpPrototype)
    : InternalFunction(vm, structure)
    , m_cachedResult(vm, this, regExpPrototype->regExp())
    , m_multiline(false)
{
}

void RegExpConstructor::finishCreation(VM& vm, RegExpPrototype* regExpPrototype)
{
    Base::finishCreation(vm, Identifier(&vm, "RegExp").string());
    ASSERT(inherits(info()));

    // ECMA 15.10.5.1 RegExp.prototype
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, regExpPrototype, DontEnum | DontDelete | ReadOnly);

    // no. of arguments for constructor
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(2), ReadOnly | DontDelete | DontEnum);
}

void RegExpConstructor::destroy(JSCell* cell)
{
    static_cast<RegExpConstructor*>(cell)->RegExpConstructor::~RegExpConstructor();
}

void RegExpConstructor::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    RegExpConstructor* thisObject = jsCast<RegExpConstructor*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    thisObject->m_cachedResult.visitChildren(visitor);
}

TiValue RegExpConstructor::getBackref(ExecState* exec, unsigned i)
{
    RegExpMatchesArray* array = m_cachedResult.lastResult(exec, this);

    if (i < array->length()) {
        TiValue result = TiValue(array).get(exec, i);
        ASSERT(result.isString() || result.isUndefined());
        if (!result.isUndefined())
            return result;
    }
    return jsEmptyString(exec);
}

TiValue RegExpConstructor::getLastParen(ExecState* exec)
{
    RegExpMatchesArray* array = m_cachedResult.lastResult(exec, this);
    unsigned length = array->length();
    if (length > 1) {
        TiValue result = TiValue(array).get(exec, length - 1);
        ASSERT(result.isString() || result.isUndefined());
        if (!result.isUndefined())
            return result;
    }
    return jsEmptyString(exec);
}

TiValue RegExpConstructor::getLeftContext(ExecState* exec)
{
    return m_cachedResult.lastResult(exec, this)->leftContext(exec);
}

TiValue RegExpConstructor::getRightContext(ExecState* exec)
{
    return m_cachedResult.lastResult(exec, this)->rightContext(exec);
}
    
bool RegExpConstructor::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<RegExpConstructor, InternalFunction>(exec, ExecState::regExpConstructorTable(exec), jsCast<RegExpConstructor*>(object), propertyName, slot);
}

static inline RegExpConstructor* asRegExpConstructor(EncodedTiValue value)
{
    return jsCast<RegExpConstructor*>(TiValue::decode(value));
}
    
EncodedTiValue regExpConstructorDollar1(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 1));
}

EncodedTiValue regExpConstructorDollar2(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 2));
}

EncodedTiValue regExpConstructorDollar3(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 3));
}

EncodedTiValue regExpConstructorDollar4(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 4));
}

EncodedTiValue regExpConstructorDollar5(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 5));
}

EncodedTiValue regExpConstructorDollar6(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 6));
}

EncodedTiValue regExpConstructorDollar7(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 7));
}

EncodedTiValue regExpConstructorDollar8(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 8));
}

EncodedTiValue regExpConstructorDollar9(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 9));
}

EncodedTiValue regExpConstructorInput(ExecState*, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->input());
}

EncodedTiValue regExpConstructorMultiline(ExecState*, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(jsBoolean(asRegExpConstructor(slotBase)->multiline()));
}

EncodedTiValue regExpConstructorLastMatch(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getBackref(exec, 0));
}

EncodedTiValue regExpConstructorLastParen(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getLastParen(exec));
}

EncodedTiValue regExpConstructorLeftContext(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getLeftContext(exec));
}

EncodedTiValue regExpConstructorRightContext(ExecState* exec, EncodedTiValue slotBase, EncodedTiValue, PropertyName)
{
    return TiValue::encode(asRegExpConstructor(slotBase)->getRightContext(exec));
}

void RegExpConstructor::put(JSCell* cell, ExecState* exec, PropertyName propertyName, TiValue value, PutPropertySlot& slot)
{
    lookupPut<RegExpConstructor, InternalFunction>(exec, propertyName, value, ExecState::regExpConstructorTable(exec), jsCast<RegExpConstructor*>(cell), slot);
}

void setRegExpConstructorInput(ExecState* exec, JSObject* baseObject, TiValue value)
{
    asRegExpConstructor(baseObject)->setInput(exec, value.toString(exec));
}

void setRegExpConstructorMultiline(ExecState* exec, JSObject* baseObject, TiValue value)
{
    asRegExpConstructor(baseObject)->setMultiline(value.toBoolean(exec));
}

// ECMA 15.10.4
JSObject* constructRegExp(ExecState* exec, JSGlobalObject* globalObject, const ArgList& args, bool callAsConstructor)
{
    TiValue arg0 = args.at(0);
    TiValue arg1 = args.at(1);

    if (arg0.inherits(RegExpObject::info())) {
        if (!arg1.isUndefined())
            return exec->vm().throwException(exec, createTypeError(exec, ASCIILiteral("Cannot supply flags when constructing one RegExp from another.")));
        // If called as a function, this just returns the first argument (see 15.10.3.1).
        if (callAsConstructor) {
            RegExp* regExp = static_cast<RegExpObject*>(asObject(arg0))->regExp();
            return RegExpObject::create(exec->vm(), globalObject->regExpStructure(), regExp);
        }
        return asObject(arg0);
    }

    String pattern = arg0.isUndefined() ? emptyString() : arg0.toString(exec)->value(exec);
    if (exec->hadException())
        return 0;

    RegExpFlags flags = NoFlags;
    if (!arg1.isUndefined()) {
        flags = regExpFlags(arg1.toString(exec)->value(exec));
        if (exec->hadException())
            return 0;
        if (flags == InvalidFlags)
            return exec->vm().throwException(exec, createSyntaxError(exec, ASCIILiteral("Invalid flags supplied to RegExp constructor.")));
    }

    VM& vm = exec->vm();
    RegExp* regExp = RegExp::create(vm, pattern, flags);
    if (!regExp->isValid())
        return vm.throwException(exec, createSyntaxError(exec, regExp->errorMessage()));
    return RegExpObject::create(vm, globalObject->regExpStructure(), regExp);
}

static EncodedTiValue JSC_HOST_CALL constructWithRegExpConstructor(ExecState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructRegExp(exec, asInternalFunction(exec->callee())->globalObject(), args, true));
}

ConstructType RegExpConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithRegExpConstructor;
    return ConstructTypeHost;
}

// ECMA 15.10.3
static EncodedTiValue JSC_HOST_CALL callRegExpConstructor(ExecState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructRegExp(exec, asInternalFunction(exec->callee())->globalObject(), args));
}

CallType RegExpConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callRegExpConstructor;
    return CallTypeHost;
}

} // namespace TI
