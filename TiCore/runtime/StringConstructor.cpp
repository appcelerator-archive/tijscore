/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "StringConstructor.h"

#include "Executable.h"
#include "JITCode.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "StringPrototype.h"

namespace TI {

static EncodedTiValue JSC_HOST_CALL stringFromCharCode(TiExcState*);

}

#include "StringConstructor.lut.h"

namespace TI {

const ClassInfo StringConstructor::s_info = { "Function", &InternalFunction::s_info, 0, TiExcState::stringConstructorTable };

/* Source for StringConstructor.lut.h
@begin stringConstructorTable
  fromCharCode          stringFromCharCode         DontEnum|Function 1
@end
*/

ASSERT_CLASS_FITS_IN_CELL(StringConstructor);

StringConstructor::StringConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, StringPrototype* stringPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, stringPrototype->classInfo()->className))
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, stringPrototype, ReadOnly | DontEnum | DontDelete);
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool StringConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, TiExcState::stringConstructorTable(exec), this, propertyName, slot);
}

bool StringConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<InternalFunction>(exec, TiExcState::stringConstructorTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions --------------------------------

static NEVER_INLINE TiValue stringFromCharCodeSlowCase(TiExcState* exec)
{
    unsigned length = exec->argumentCount();
    UChar* buf;
    PassRefPtr<StringImpl> impl = StringImpl::createUninitialized(length, buf);
    for (unsigned i = 0; i < length; ++i)
        buf[i] = static_cast<UChar>(exec->argument(i).toUInt32(exec));
    return jsString(exec, impl);
}

static EncodedTiValue JSC_HOST_CALL stringFromCharCode(TiExcState* exec)
{
    if (LIKELY(exec->argumentCount() == 1))
        return TiValue::encode(jsSingleCharacterString(exec, exec->argument(0).toUInt32(exec)));
    return TiValue::encode(stringFromCharCodeSlowCase(exec));
}

static EncodedTiValue JSC_HOST_CALL constructWithStringConstructor(TiExcState* exec)
{
    TiGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();
    if (!exec->argumentCount())
        return TiValue::encode(new (exec) StringObject(exec, globalObject->stringObjectStructure()));
    return TiValue::encode(new (exec) StringObject(exec, globalObject->stringObjectStructure(), exec->argument(0).toString(exec)));
}

ConstructType StringConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithStringConstructor;
    return ConstructTypeHost;
}

static EncodedTiValue JSC_HOST_CALL callStringConstructor(TiExcState* exec)
{
    if (!exec->argumentCount())
        return TiValue::encode(jsEmptyString(exec));
    return TiValue::encode(jsString(exec, exec->argument(0).toString(exec)));
}

CallType StringConstructor::getCallData(CallData& callData)
{
    callData.native.function = callStringConstructor;
    return CallTypeHost;
}

} // namespace TI
