/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ExceptionHelpers.h"

#include "CodeBlock.h"
#include "CallFrame.h"
#include "ErrorInstance.h"
#include "TiGlobalObjectFunctions.h"
#include "TiObject.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "Nodes.h"
#include "UStringConcatenate.h"

namespace TI {

class InterruptedExecutionError : public JSNonFinalObject {
public:
    InterruptedExecutionError(TiGlobalData* globalData)
        : JSNonFinalObject(*globalData, globalData->interruptedExecutionErrorStructure.get())
    {
    }

    virtual ComplType exceptionType() const { return Interrupted; }

    virtual UString toString(TiExcState*) const { return "Ti execution exceeded timeout."; }
};

TiObject* createInterruptedExecutionException(TiGlobalData* globalData)
{
    return new (globalData) InterruptedExecutionError(globalData);
}

class TerminatedExecutionError : public JSNonFinalObject {
public:
    TerminatedExecutionError(TiGlobalData* globalData)
        : JSNonFinalObject(*globalData, globalData->terminatedExecutionErrorStructure.get())
    {
    }

    virtual ComplType exceptionType() const { return Terminated; }

    virtual UString toString(TiExcState*) const { return "Ti execution terminated."; }
};

TiObject* createTerminatedExecutionException(TiGlobalData* globalData)
{
    return new (globalData) TerminatedExecutionError(globalData);
}

TiObject* createStackOverflowError(TiExcState* exec)
{
    return createRangeError(exec, "Maximum call stack size exceeded.");
}

TiObject* createStackOverflowError(TiGlobalObject* globalObject)
{
    return createRangeError(globalObject, "Maximum call stack size exceeded.");
}

TiObject* createUndefinedVariableError(TiExcState* exec, const Identifier& ident)
{
    UString message(makeUString("Can't find variable: ", ident.ustring()));
    return createReferenceError(exec, message);
}
    
TiObject* createInvalidParamError(TiExcState* exec, const char* op, TiValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not a valid argument for '", op, "'");
    TiObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

TiObject* createNotAConstructorError(TiExcState* exec, TiValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not a constructor");
    TiObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

TiObject* createNotAFunctionError(TiExcState* exec, TiValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not a function");
    TiObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

TiObject* createNotAnObjectError(TiExcState* exec, TiValue value)
{
    UString errorMessage = makeUString("'", value.toString(exec), "' is not an object");
    TiObject* exception = createTypeError(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setAppendSourceToMessage();
    return exception;
}

TiObject* createErrorForInvalidGlobalAssignment(TiExcState* exec, const UString& propertyName)
{
    return createReferenceError(exec, makeUString("Strict mode forbids implicit creation of global property '", propertyName, "'"));
}

TiObject* createOutOfMemoryError(TiGlobalObject* globalObject)
{
    return createError(globalObject, "Out of memory");
}

TiObject* throwOutOfMemoryError(TiExcState* exec)
{
    return throwError(exec, createOutOfMemoryError(exec->lexicalGlobalObject()));
}

TiObject* throwStackOverflowError(TiExcState* exec)
{
    return throwError(exec, createStackOverflowError(exec));
}

} // namespace TI
