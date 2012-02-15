/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel (eric@webkit.org)
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

#include "config.h"
#include "Error.h"

#include "ConstructData.h"
#include "ErrorConstructor.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "TiObject.h"
#include "TiString.h"
#include "NativeErrorConstructor.h"
#include "SourceCode.h"

namespace TI {

static const char* linePropertyName = "line";
static const char* sourceIdPropertyName = "sourceId";
static const char* sourceURLPropertyName = "sourceURL";

TiObject* createError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->errorStructure(), message);
}

TiObject* createEvalError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->evalErrorConstructor()->errorStructure(), message);
}

TiObject* createRangeError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->rangeErrorConstructor()->errorStructure(), message);
}

TiObject* createReferenceError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->referenceErrorConstructor()->errorStructure(), message);
}

TiObject* createSyntaxError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->syntaxErrorConstructor()->errorStructure(), message);
}

TiObject* createTypeError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->typeErrorConstructor()->errorStructure(), message);
}

TiObject* createURIError(TiGlobalObject* globalObject, const UString& message)
{
    ASSERT(!message.isEmpty());
    return ErrorInstance::create(&globalObject->globalData(), globalObject->URIErrorConstructor()->errorStructure(), message);
}

TiObject* createError(TiExcState* exec, const UString& message)
{
    return createError(exec->lexicalGlobalObject(), message);
}

TiObject* createEvalError(TiExcState* exec, const UString& message)
{
    return createEvalError(exec->lexicalGlobalObject(), message);
}

TiObject* createRangeError(TiExcState* exec, const UString& message)
{
    return createRangeError(exec->lexicalGlobalObject(), message);
}

TiObject* createReferenceError(TiExcState* exec, const UString& message)
{
    return createReferenceError(exec->lexicalGlobalObject(), message);
}

TiObject* createSyntaxError(TiExcState* exec, const UString& message)
{
    return createSyntaxError(exec->lexicalGlobalObject(), message);
}

TiObject* createTypeError(TiExcState* exec, const UString& message)
{
    return createTypeError(exec->lexicalGlobalObject(), message);
}

TiObject* createURIError(TiExcState* exec, const UString& message)
{
    return createURIError(exec->lexicalGlobalObject(), message);
}

TiObject* addErrorInfo(TiGlobalData* globalData, TiObject* error, int line, const SourceCode& source)
{
    intptr_t sourceID = source.provider()->asID();
    const UString& sourceURL = source.provider()->url();

    if (line != -1)
        error->putWithAttributes(globalData, Identifier(globalData, linePropertyName), jsNumber(line), ReadOnly | DontDelete);
    if (sourceID != -1)
        error->putWithAttributes(globalData, Identifier(globalData, sourceIdPropertyName), jsNumber((double)sourceID), ReadOnly | DontDelete);
    if (!sourceURL.isNull())
        error->putWithAttributes(globalData, Identifier(globalData, sourceURLPropertyName), jsString(globalData, sourceURL), ReadOnly | DontDelete);

    return error;
}

TiObject* addErrorInfo(TiExcState* exec, TiObject* error, int line, const SourceCode& source)
{
    return addErrorInfo(&exec->globalData(), error, line, source);
}

bool hasErrorInfo(TiExcState* exec, TiObject* error)
{
    return error->hasProperty(exec, Identifier(exec, linePropertyName))
        || error->hasProperty(exec, Identifier(exec, sourceIdPropertyName))
        || error->hasProperty(exec, Identifier(exec, sourceURLPropertyName));
}

TiValue throwError(TiExcState* exec, TiValue error)
{
    exec->globalData().exception = error;
    return error;
}

TiObject* throwError(TiExcState* exec, TiObject* error)
{
    exec->globalData().exception = error;
    return error;
}

TiObject* throwTypeError(TiExcState* exec)
{
    return throwError(exec, createTypeError(exec, "Type error"));
}

TiObject* throwSyntaxError(TiExcState* exec)
{
    return throwError(exec, createSyntaxError(exec, "Syntax error"));
}

class StrictModeTypeErrorFunction : public InternalFunction {
public:
    StrictModeTypeErrorFunction(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure, const UString& message)
        : InternalFunction(&exec->globalData(), globalObject, structure, exec->globalData().propertyNames->emptyIdentifier)
        , m_message(message)
    {
    }
    
    static EncodedTiValue JSC_HOST_CALL constructThrowTypeError(TiExcState* exec)
    {
        throwTypeError(exec, static_cast<StrictModeTypeErrorFunction*>(exec->callee())->m_message);
        return TiValue::encode(jsNull());
    }
    
    ConstructType getConstructData(ConstructData& constructData)
    {
        constructData.native.function = constructThrowTypeError;
        return ConstructTypeHost;
    }
    
    static EncodedTiValue JSC_HOST_CALL callThrowTypeError(TiExcState* exec)
    {
        throwTypeError(exec, static_cast<StrictModeTypeErrorFunction*>(exec->callee())->m_message);
        return TiValue::encode(jsNull());
    }

    CallType getCallData(CallData& callData)
    {
        callData.native.function = callThrowTypeError;
        return CallTypeHost;
    }

private:
    UString m_message;
};

ASSERT_CLASS_FITS_IN_CELL(StrictModeTypeErrorFunction);

TiValue createTypeErrorFunction(TiExcState* exec, const UString& message)
{
    return new (exec) StrictModeTypeErrorFunction(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->internalFunctionStructure(), message);
}

} // namespace TI
