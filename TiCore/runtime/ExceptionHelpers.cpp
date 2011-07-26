/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "TiGlobalObjectFunctions.h"
#include "TiObject.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "Nodes.h"

namespace TI {

class InterruptedExecutionError : public TiObject {
public:
    InterruptedExecutionError(TiGlobalData* globalData)
        : TiObject(globalData->interruptedExecutionErrorStructure)
    {
    }

    virtual ComplType exceptionType() const { return Interrupted; }

    virtual UString toString(TiExcState*) const { return "Ti execution exceeded timeout."; }
};

TiValue createInterruptedExecutionException(TiGlobalData* globalData)
{
    return new (globalData) InterruptedExecutionError(globalData);
}

class TerminatedExecutionError : public TiObject {
public:
    TerminatedExecutionError(TiGlobalData* globalData)
        : TiObject(globalData->terminatedExecutionErrorStructure)
    {
    }

    virtual ComplType exceptionType() const { return Terminated; }

    virtual UString toString(TiExcState*) const { return "Ti execution terminated."; }
};

TiValue createTerminatedExecutionException(TiGlobalData* globalData)
{
    return new (globalData) TerminatedExecutionError(globalData);
}

static TiValue createError(TiExcState* exec, ErrorType e, const char* msg)
{
    return Error::create(exec, e, msg, -1, -1, UString());
}

TiValue createStackOverflowError(TiExcState* exec)
{
    return createError(exec, RangeError, "Maximum call stack size exceeded.");
}

TiValue createTypeError(TiExcState* exec, const char* message)
{
    return createError(exec, TypeError, message);
}

TiValue createUndefinedVariableError(TiExcState* exec, const Identifier& ident, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    TiObject* exception = Error::create(exec, ReferenceError, makeString("Can't find variable: ", ident.ustring()), line, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}
    
static UString createErrorMessage(TiExcState* exec, CodeBlock* codeBlock, int, int expressionStart, int expressionStop, TiValue value, UString error)
{
    if (!expressionStop || expressionStart > codeBlock->source()->length())
        return makeString(value.toString(exec), " is ", error);
    if (expressionStart < expressionStop)
        return makeString("Result of expression '", codeBlock->source()->getRange(expressionStart, expressionStop), "' [", value.toString(exec), "] is ", error, ".");

    // No range information, so give a few characters of context
    const UChar* data = codeBlock->source()->data();
    int dataLength = codeBlock->source()->length();
    int start = expressionStart;
    int stop = expressionStart;
    // Get up to 20 characters of context to the left and right of the divot, clamping to the line.
    // then strip whitespace.
    while (start > 0 && (expressionStart - start < 20) && data[start - 1] != '\n')
        start--;
    while (start < (expressionStart - 1) && isStrWhiteSpace(data[start]))
        start++;
    while (stop < dataLength && (stop - expressionStart < 20) && data[stop] != '\n')
        stop++;
    while (stop > expressionStart && isStrWhiteSpace(data[stop]))
        stop--;
    return makeString("Result of expression near '...", codeBlock->source()->getRange(start, stop), "...' [", value.toString(exec), "] is ", error, ".");
}

TiObject* createInvalidParamError(TiExcState* exec, const char* op, TiValue value, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString errorMessage = createErrorMessage(exec, codeBlock, line, divotPoint, divotPoint + endOffset, value, makeString("not a valid argument for '", op, "'"));
    TiObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

TiObject* createNotAConstructorError(TiExcState* exec, TiValue value, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);

    // We're in a "new" expression, so we need to skip over the "new.." part
    int startPoint = divotPoint - (startOffset ? startOffset - 4 : 0); // -4 for "new "
    const UChar* data = codeBlock->source()->data();
    while (startPoint < divotPoint && isStrWhiteSpace(data[startPoint]))
        startPoint++;
    
    UString errorMessage = createErrorMessage(exec, codeBlock, line, startPoint, divotPoint, value, "not a constructor");
    TiObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

TiValue createNotAFunctionError(TiExcState* exec, TiValue value, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString errorMessage = createErrorMessage(exec, codeBlock, line, divotPoint - startOffset, divotPoint, value, "not a function");
    TiObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->sourceURL());    
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

JSNotAnObjectErrorStub* createNotAnObjectErrorStub(TiExcState* exec, bool isNull)
{
    return new (exec) JSNotAnObjectErrorStub(exec, isNull);
}

TiObject* createNotAnObjectError(TiExcState* exec, JSNotAnObjectErrorStub* error, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    // Both op_construct and op_instanceof require a use of op_get_by_id to get
    // the prototype property from an object. The exception messages for exceptions
    // thrown by these instances op_get_by_id need to reflect this.
    OpcodeID followingOpcodeID;
    if (codeBlock->getByIdExceptionInfoForBytecodeOffset(exec, bytecodeOffset, followingOpcodeID)) {
        ASSERT(followingOpcodeID == op_construct || followingOpcodeID == op_instanceof);
        if (followingOpcodeID == op_construct)
            return createNotAConstructorError(exec, error->isNull() ? jsNull() : jsUndefined(), bytecodeOffset, codeBlock);
        return createInvalidParamError(exec, "instanceof", error->isNull() ? jsNull() : jsUndefined(), bytecodeOffset, codeBlock);
    }

    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString errorMessage = createErrorMessage(exec, codeBlock, line, divotPoint - startOffset, divotPoint, error->isNull() ? jsNull() : jsUndefined(), "not an object");
    TiObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

TiValue throwOutOfMemoryError(TiExcState* exec)
{
    return throwError(exec, GeneralError, "Out of memory");
}

} // namespace TI
