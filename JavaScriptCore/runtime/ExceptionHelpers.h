/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef ExceptionHelpers_h
#define ExceptionHelpers_h

#include "JSObject.h"

namespace TI {

typedef JSObject* (*ErrorFactory)(ExecState*, const String&);

JSObject* createTerminatedExecutionException(VM*);
bool isTerminatedExecutionException(JSObject*);
JS_EXPORT_PRIVATE bool isTerminatedExecutionException(TiValue);
JS_EXPORT_PRIVATE JSObject* createError(ExecState*, ErrorFactory, TiValue, const String&);
JS_EXPORT_PRIVATE JSObject* createStackOverflowError(ExecState*);
JSObject* createStackOverflowError(JSGlobalObject*);
JSObject* createOutOfMemoryError(JSGlobalObject*);
JSObject* createUndefinedVariableError(ExecState*, const Identifier&);
JSObject* createNotAnObjectError(ExecState*, TiValue);
JSObject* createInvalidParameterError(ExecState*, const char* op, TiValue);
JSObject* createNotAConstructorError(ExecState*, TiValue);
JSObject* createNotAFunctionError(ExecState*, TiValue);
JSObject* createErrorForInvalidGlobalAssignment(ExecState*, const String&);
JSString* errorDescriptionForValue(ExecState*, TiValue);

JSObject* throwOutOfMemoryError(ExecState*);
JSObject* throwStackOverflowError(ExecState*);
JSObject* throwTerminatedExecutionException(ExecState*);


class TerminatedExecutionError : public JSNonFinalObject {
private:
    TerminatedExecutionError(VM& vm)
        : JSNonFinalObject(vm, vm.terminatedExecutionErrorStructure.get())
    {
    }

    static TiValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);

public:
    typedef JSNonFinalObject Base;

    static TerminatedExecutionError* create(VM& vm)
    {
        TerminatedExecutionError* error = new (NotNull, allocateCell<TerminatedExecutionError>(vm.heap)) TerminatedExecutionError(vm);
        error->finishCreation(vm);
        return error;
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

    DECLARE_EXPORT_INFO;
};

} // namespace TI

#endif // ExceptionHelpers_h
