/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "FunctionPrototype.h"

#include "Arguments.h"
#include "TiArray.h"
#include "TiFunction.h"
#include "TiString.h"
#include "TiStringBuilder.h"
#include "Interpreter.h"
#include "Lexer.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(FunctionPrototype);

static EncodedTiValue JSC_HOST_CALL functionProtoFuncToString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL functionProtoFuncApply(TiExcState*);
static EncodedTiValue JSC_HOST_CALL functionProtoFuncCall(TiExcState*);

FunctionPrototype::FunctionPrototype(TiExcState* exec, TiGlobalObject* globalObject, Structure* structure)
    : InternalFunction(&exec->globalData(), globalObject, structure, exec->propertyNames().nullIdentifier)
{
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(0), DontDelete | ReadOnly | DontEnum);
}

void FunctionPrototype::addFunctionProperties(TiExcState* exec, TiGlobalObject* globalObject, Structure* functionStructure, TiFunction** callFunction, TiFunction** applyFunction)
{
    putDirectFunctionWithoutTransition(exec, new (exec) TiFunction(exec, globalObject, functionStructure, 0, exec->propertyNames().toString, functionProtoFuncToString), DontEnum);
    *applyFunction = new (exec) TiFunction(exec, globalObject, functionStructure, 2, exec->propertyNames().apply, functionProtoFuncApply);
    putDirectFunctionWithoutTransition(exec, *applyFunction, DontEnum);
    *callFunction = new (exec) TiFunction(exec, globalObject, functionStructure, 1, exec->propertyNames().call, functionProtoFuncCall);
    putDirectFunctionWithoutTransition(exec, *callFunction, DontEnum);
}

static EncodedTiValue JSC_HOST_CALL callFunctionPrototype(TiExcState*)
{
    return TiValue::encode(jsUndefined());
}

// ECMA 15.3.4
CallType FunctionPrototype::getCallData(CallData& callData)
{
    callData.native.function = callFunctionPrototype;
    return CallTypeHost;
}

// Functions

// Compatibility hack for the Optimost Ti library. (See <rdar://problem/6595040>.)
static inline void insertSemicolonIfNeeded(UString& functionBody)
{
    ASSERT(functionBody[0] == '{');
    ASSERT(functionBody[functionBody.length() - 1] == '}');

    for (size_t i = functionBody.length() - 2; i > 0; --i) {
        UChar ch = functionBody[i];
        if (!Lexer::isWhiteSpace(ch) && !Lexer::isLineTerminator(ch)) {
            if (ch != ';' && ch != '}')
                functionBody = makeUString(functionBody.substringSharingImpl(0, i + 1), ";", functionBody.substringSharingImpl(i + 1, functionBody.length() - (i + 1)));
            return;
        }
    }
}

EncodedTiValue JSC_HOST_CALL functionProtoFuncToString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    if (thisValue.inherits(&TiFunction::s_info)) {
        TiFunction* function = asFunction(thisValue);
        if (function->isHostFunction())
            return TiValue::encode(jsMakeNontrivialString(exec, "function ", function->name(exec), "() {\n    [native code]\n}"));
        FunctionExecutable* executable = function->jsExecutable();
        UString sourceString = executable->source().toString();
        insertSemicolonIfNeeded(sourceString);
        return TiValue::encode(jsMakeNontrivialString(exec, "function ", function->name(exec), "(", executable->paramString(), ") ", sourceString));
    }

    if (thisValue.inherits(&InternalFunction::s_info)) {
        InternalFunction* function = asInternalFunction(thisValue);
        return TiValue::encode(jsMakeNontrivialString(exec, "function ", function->name(exec), "() {\n    [native code]\n}"));
    }

    return throwVMTypeError(exec);
}

EncodedTiValue JSC_HOST_CALL functionProtoFuncApply(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    CallData callData;
    CallType callType = getCallData(thisValue, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    TiValue array = exec->argument(1);

    MarkedArgumentBuffer applyArgs;
    if (!array.isUndefinedOrNull()) {
        if (!array.isObject())
            return throwVMTypeError(exec);
        if (asObject(array)->classInfo() == &Arguments::s_info)
            asArguments(array)->fillArgList(exec, applyArgs);
        else if (isTiArray(&exec->globalData(), array))
            asArray(array)->fillArgList(exec, applyArgs);
        else if (asObject(array)->inherits(&TiArray::s_info)) {
            unsigned length = asArray(array)->get(exec, exec->propertyNames().length).toUInt32(exec);
            for (unsigned i = 0; i < length; ++i)
                applyArgs.append(asArray(array)->get(exec, i));
        } else
            return throwVMTypeError(exec);
    }

    return TiValue::encode(call(exec, thisValue, callType, callData, exec->argument(0), applyArgs));
}

EncodedTiValue JSC_HOST_CALL functionProtoFuncCall(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    CallData callData;
    CallType callType = getCallData(thisValue, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    ArgList args(exec);
    ArgList callArgs;
    args.getSlice(1, callArgs);
    return TiValue::encode(call(exec, thisValue, callType, callData, exec->argument(0), callArgs));
}

} // namespace TI
