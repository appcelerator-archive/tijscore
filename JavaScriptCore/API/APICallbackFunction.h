/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef APICallbackFunction_h
#define APICallbackFunction_h

#include "APICast.h"
#include "APIShims.h"
#include "Error.h"
#include "JSCallbackConstructor.h"
#include <wtf/Vector.h>

namespace TI {

struct APICallbackFunction {

template <typename T> static EncodedTiValue JSC_HOST_CALL call(ExecState*);
template <typename T> static EncodedTiValue JSC_HOST_CALL construct(ExecState*);

};

template <typename T>
EncodedTiValue JSC_HOST_CALL APICallbackFunction::call(ExecState* exec)
{
    TiContextRef execRef = toRef(exec);
    TiObjectRef functionRef = toRef(exec->callee());
    TiObjectRef thisObjRef = toRef(jsCast<JSObject*>(exec->hostThisValue().toThis(exec, NotStrictMode)));

    int argumentCount = static_cast<int>(exec->argumentCount());
    Vector<TiValueRef, 16> arguments;
    arguments.reserveInitialCapacity(argumentCount);
    for (int i = 0; i < argumentCount; i++)
        arguments.uncheckedAppend(toRef(exec, exec->uncheckedArgument(i)));

    TiValueRef exception = 0;
    TiValueRef result;
    {
        APICallbackShim callbackShim(exec);
        result = jsCast<T*>(toJS(functionRef))->functionCallback()(execRef, functionRef, thisObjRef, argumentCount, arguments.data(), &exception);
    }
    if (exception)
        exec->vm().throwException(exec, toJS(exec, exception));

    // result must be a valid TiValue.
    if (!result)
        return TiValue::encode(jsUndefined());

    return TiValue::encode(toJS(exec, result));
}

template <typename T>
EncodedTiValue JSC_HOST_CALL APICallbackFunction::construct(ExecState* exec)
{
    JSObject* constructor = exec->callee();
    TiContextRef ctx = toRef(exec);
    TiObjectRef constructorRef = toRef(constructor);

    TiObjectCallAsConstructorCallback callback = jsCast<T*>(constructor)->constructCallback();
    if (callback) {
        size_t argumentCount = exec->argumentCount();
        Vector<TiValueRef, 16> arguments;
        arguments.reserveInitialCapacity(argumentCount);
        for (size_t i = 0; i < argumentCount; ++i)
            arguments.uncheckedAppend(toRef(exec, exec->uncheckedArgument(i)));

        TiValueRef exception = 0;
        TiObjectRef result;
        {
            APICallbackShim callbackShim(exec);
            result = callback(ctx, constructorRef, argumentCount, arguments.data(), &exception);
        }
        if (exception) {
            exec->vm().throwException(exec, toJS(exec, exception));
            return TiValue::encode(toJS(exec, exception));
        }
        // result must be a valid TiValue.
        if (!result)
            return throwVMTypeError(exec);
        return TiValue::encode(toJS(result));
    }
    
    return TiValue::encode(toJS(TiObjectMake(ctx, jsCast<JSCallbackConstructor*>(constructor)->classRef(), 0)));
}

} // namespace TI

#endif // APICallbackFunction_h
