/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
#include "NativeErrorConstructor.h"

#include "ErrorInstance.h"
#include "TiFunction.h"
#include "TiString.h"
#include "NativeErrorPrototype.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(NativeErrorConstructor);

const ClassInfo NativeErrorConstructor::info = { "Function", &InternalFunction::info, 0, 0 };

NativeErrorConstructor::NativeErrorConstructor(TiExcState* exec, NonNullPassRefPtr<Structure> structure, NonNullPassRefPtr<Structure> prototypeStructure, const UString& nameAndMessage)
    : InternalFunction(&exec->globalData(), structure, Identifier(exec, nameAndMessage))
{
    NativeErrorPrototype* prototype = new (exec) NativeErrorPrototype(exec, prototypeStructure, nameAndMessage, this);

    putDirect(exec->propertyNames().length, jsNumber(exec, 1), DontDelete | ReadOnly | DontEnum); // ECMA 15.11.7.5
    putDirect(exec->propertyNames().prototype, prototype, DontDelete | ReadOnly | DontEnum);
    m_errorStructure = ErrorInstance::createStructure(prototype);
}


ErrorInstance* NativeErrorConstructor::construct(TiExcState* exec, const ArgList& args)
{
    ErrorInstance* object = new (exec) ErrorInstance(m_errorStructure);
    if (!args.at(0).isUndefined())
        object->putDirect(exec->propertyNames().message, jsString(exec, args.at(0).toString(exec)));
    return object;
}

static TiObject* constructWithNativeErrorConstructor(TiExcState* exec, TiObject* constructor, const ArgList& args)
{
    return static_cast<NativeErrorConstructor*>(constructor)->construct(exec, args);
}

ConstructType NativeErrorConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructWithNativeErrorConstructor;
    return ConstructTypeHost;
}
    
static TiValue JSC_HOST_CALL callNativeErrorConstructor(TiExcState* exec, TiObject* constructor, TiValue, const ArgList& args)
{
    return static_cast<NativeErrorConstructor*>(constructor)->construct(exec, args);
}

CallType NativeErrorConstructor::getCallData(CallData& callData)
{
    callData.native.function = callNativeErrorConstructor;
    return CallTypeHost;
}

} // namespace TI
