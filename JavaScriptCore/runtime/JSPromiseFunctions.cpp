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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSPromiseFunctions.h"

#if ENABLE(PROMISES)

#include "Error.h"
#include "JSCTiValueInlines.h"
#include "JSCellInlines.h"
#include "JSPromise.h"
#include "JSPromiseConstructor.h"
#include "JSPromiseDeferred.h"

namespace TI {

// Deferred Construction Functions
static EncodedTiValue JSC_HOST_CALL deferredConstructionFunction(ExecState* exec)
{
    JSObject* F = exec->callee();

    VM& vm = exec->vm();

    // 1. Set F's [[Resolve]] internal slot to resolve.
    F->putDirect(vm, vm.propertyNames->resolvePrivateName, exec->argument(0));

    // 2. Set F's [[Reject]] internal slot to reject.
    F->putDirect(vm, vm.propertyNames->rejectPrivateName, exec->argument(1));

    // 3. Return.
    return TiValue::encode(jsUndefined());
}

JSFunction* createDeferredConstructionFunction(VM& vm, JSGlobalObject* globalObject)
{
    return JSFunction::create(vm, globalObject, 2, ASCIILiteral("DeferredConstructionFunction"), deferredConstructionFunction);
}

// Identity Functions

static EncodedTiValue JSC_HOST_CALL identifyFunction(ExecState* exec)
{
    return TiValue::encode(exec->argument(0));
}

JSFunction* createIdentifyFunction(VM& vm, JSGlobalObject* globalObject)
{
    return JSFunction::create(vm, globalObject, 1, ASCIILiteral("IdentityFunction"), identifyFunction);
}

// Promise Resolution Handler Functions

static EncodedTiValue JSC_HOST_CALL promiseResolutionHandlerFunction(ExecState* exec)
{
    TiValue x = exec->argument(0);
    VM& vm = exec->vm();
    JSObject* F = exec->callee();

    // 1. Let 'promise' be the value of F's [[Promise]] internal slot
    JSPromise* promise = jsCast<JSPromise*>(F->get(exec, vm.propertyNames->promisePrivateName));

    // 2. Let 'fulfillmentHandler' be the value of F's [[FulfillmentHandler]] internal slot.
    TiValue fulfillmentHandler = F->get(exec, vm.propertyNames->fulfillmentHandlerPrivateName);
    
    // 3. Let 'rejectionHandler' be the value of F's [[RejectionHandler]] internal slot.
    TiValue rejectionHandler = F->get(exec, vm.propertyNames->rejectionHandlerPrivateName);
    
    // 4. If SameValue(x, promise) is true,
    if (sameValue(exec, x, promise)) {
        // i. Let 'selfResolutionError' be a newly-created TypeError object.
        JSObject* selfResolutionError = createTypeError(exec, ASCIILiteral("Resolve a promise with itself"));
        // ii. Return the result of calling the [[Call]] internal method of rejectionHandler with
        //     undefined as thisArgument and a List containing selfResolutionError as argumentsList.
        CallData rejectCallData;
        CallType rejectCallType = getCallData(rejectionHandler, rejectCallData);
        ASSERT(rejectCallType != CallTypeNone);

        MarkedArgumentBuffer rejectArguments;
        rejectArguments.append(selfResolutionError);

        return TiValue::encode(call(exec, rejectionHandler, rejectCallType, rejectCallData, jsUndefined(), rejectArguments));
    }

    // 5. Let 'C' be the value of promise's [[PromiseConstructor]] internal slot.
    TiValue C = promise->constructor();

    // 6. Let 'deferred' be the result of calling GetDeferred(C)
    TiValue deferredValue = createJSPromiseDeferredFromConstructor(exec, C);

    // 7. ReturnIfAbrupt(deferred).
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    JSPromiseDeferred* deferred = jsCast<JSPromiseDeferred*>(deferredValue);

    // 8. Let 'updateResult' be the result of calling UpdateDeferredFromPotentialThenable(x, deferred).
    ThenableStatus updateResult = updateDeferredFromPotentialThenable(exec, x, deferred);

    // 9. ReturnIfAbrupt(updateResult).
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    // 10. If 'updateResult' is not "not a thenable", return the result of calling
    //     Invoke(deferred.[[Promise]], "then", (fulfillmentHandler, rejectionHandler)).
    // NOTE: Invoke does not seem to be defined anywhere, so I am guessing here.
    if (updateResult != NotAThenable) {
        JSObject* deferredPromise = deferred->promise();
    
        TiValue thenValue = deferredPromise->get(exec, exec->vm().propertyNames->then);
        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        CallData thenCallData;
        CallType thenCallType = getCallData(thenValue, thenCallData);
        if (thenCallType == CallTypeNone)
            return TiValue::encode(throwTypeError(exec));

        MarkedArgumentBuffer arguments;
        arguments.append(fulfillmentHandler);
        arguments.append(rejectionHandler);

        return TiValue::encode(call(exec, thenValue, thenCallType, thenCallData, deferredPromise, arguments));
    }
    
    // 11. Return the result of calling the [[Call]] internal method of fulfillmentHandler
    //     with undefined as thisArgument and a List containing x as argumentsList.
    CallData fulfillmentHandlerCallData;
    CallType fulfillmentHandlerCallType = getCallData(fulfillmentHandler, fulfillmentHandlerCallData);
    ASSERT(fulfillmentHandlerCallType != CallTypeNone);
    
    MarkedArgumentBuffer fulfillmentHandlerArguments;
    fulfillmentHandlerArguments.append(x);

    return TiValue::encode(call(exec, fulfillmentHandler, fulfillmentHandlerCallType, fulfillmentHandlerCallData, jsUndefined(), fulfillmentHandlerArguments));
}

JSFunction* createPromiseResolutionHandlerFunction(VM& vm, JSGlobalObject* globalObject)
{
    return JSFunction::create(vm, globalObject, 1, ASCIILiteral("PromiseResolutionHandlerFunction"), promiseResolutionHandlerFunction);
}

// Reject Promise Functions

static EncodedTiValue JSC_HOST_CALL rejectPromiseFunction(ExecState* exec)
{
    TiValue reason = exec->argument(0);
    JSObject* F = exec->callee();
    VM& vm = exec->vm();

    // 1. Let 'promise' be the value of F's [[Promise]] internal slot.
    JSPromise* promise = jsCast<JSPromise*>(F->get(exec, exec->vm().propertyNames->promisePrivateName));

    // 2. Return the result of calling PromiseReject(promise, reason);
    promise->reject(vm, reason);

    return TiValue::encode(jsUndefined());
}

JSFunction* createRejectPromiseFunction(VM& vm, JSGlobalObject* globalObject)
{
    return JSFunction::create(vm, globalObject, 1, ASCIILiteral("RejectPromiseFunction"), rejectPromiseFunction);
}

// Resolve Promise Functions

static EncodedTiValue JSC_HOST_CALL resolvePromiseFunction(ExecState* exec)
{
    TiValue resolution = exec->argument(0);
    JSObject* F = exec->callee();
    VM& vm = exec->vm();

    // 1. Let 'promise' be the value of F's [[Promise]] internal slot.
    JSPromise* promise = jsCast<JSPromise*>(F->get(exec, vm.propertyNames->promisePrivateName));

    // 2. Return the result of calling PromiseResolve(promise, resolution);
    promise->resolve(vm, resolution);

    return TiValue::encode(jsUndefined());
}

JSFunction* createResolvePromiseFunction(VM& vm, JSGlobalObject* globalObject)
{
    return JSFunction::create(vm, globalObject, 1, ASCIILiteral("ResolvePromiseFunction"), resolvePromiseFunction);
}

// Thrower Functions

static EncodedTiValue JSC_HOST_CALL throwerFunction(ExecState* exec)
{
    return TiValue::encode(exec->vm().throwException(exec, exec->argument(0)));
}

JSFunction* createThrowerFunction(VM& vm, JSGlobalObject* globalObject)
{
    return JSFunction::create(vm, globalObject, 1, ASCIILiteral("ThrowerFunction"), throwerFunction);
}


} // namespace TI

#endif // ENABLE(PROMISES)
