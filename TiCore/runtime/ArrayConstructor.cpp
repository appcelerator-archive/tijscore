/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2011 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "ArrayConstructor.h"

#include "ArrayPrototype.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "TiArray.h"
#include "TiFunction.h"
#include "Lookup.h"

namespace TI {

static EncodedTiValue JSC_HOST_CALL arrayConstructorIsArray(TiExcState*);

}

#include "ArrayConstructor.lut.h"

namespace TI {

const ClassInfo ArrayConstructor::s_info = { "Function", &InternalFunction::s_info, 0, TiExcState::arrayConstructorTable };

/* Source for ArrayConstructor.lut.h
@begin arrayConstructorTable
  isArray   arrayConstructorIsArray     DontEnum|Function 1
@end
*/

ASSERT_CLASS_FITS_IN_CELL(ArrayConstructor);

ArrayConstructor::ArrayConstructor(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, ArrayPrototype* arrayPrototype)
    : InternalFunction(&exec->globalData(), globalObject, structure, Identifier(exec, arrayPrototype->classInfo()->className))
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, arrayPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool ArrayConstructor::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, TiExcState::arrayConstructorTable(exec), this, propertyName, slot);
}

bool ArrayConstructor::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<InternalFunction>(exec, TiExcState::arrayConstructorTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

static inline TiObject* constructArrayWithSizeQuirk(TiExcState* exec, const ArgList& args)
{
    TiGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();

    // a single numeric argument denotes the array size (!)
    if (args.size() == 1 && args.at(0).isNumber()) {
        uint32_t n = args.at(0).toUInt32(exec);
        if (n != args.at(0).toNumber(exec))
            return throwError(exec, createRangeError(exec, "Array size is not a small enough positive integer."));
        return new (exec) TiArray(exec->globalData(), globalObject->arrayStructure(), n, CreateInitialized);
    }

    // otherwise the array is constructed with the arguments in it
    return new (exec) TiArray(exec->globalData(), globalObject->arrayStructure(), args);
}

static EncodedTiValue JSC_HOST_CALL constructWithArrayConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructArrayWithSizeQuirk(exec, args));
}

ConstructType ArrayConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithArrayConstructor;
    return ConstructTypeHost;
}

static EncodedTiValue JSC_HOST_CALL callArrayConstructor(TiExcState* exec)
{
    ArgList args(exec);
    return TiValue::encode(constructArrayWithSizeQuirk(exec, args));
}

CallType ArrayConstructor::getCallData(CallData& callData)
{
    // equivalent to 'new Array(....)'
    callData.native.function = callArrayConstructor;
    return CallTypeHost;
}

EncodedTiValue JSC_HOST_CALL arrayConstructorIsArray(TiExcState* exec)
{
    return TiValue::encode(jsBoolean(exec->argument(0).inherits(&TiArray::s_info)));
}

} // namespace TI
