/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef TiContextRefPrivate_h
#define TiContextRefPrivate_h

#include <JavaScriptCore/TiObjectRef.h>
#include <JavaScriptCore/TiValueRef.h>
#include <JavaScriptCore/WebKitAvailability.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
@function
@abstract Gets a Backtrace for the existing context
@param ctx The TiContext whose backtrace you want to get
@result A string containing the backtrace
*/
JS_EXPORT TiStringRef TiContextCreateBacktrace(TiContextRef ctx, unsigned maxStackSize) CF_AVAILABLE(10_6, 7_0);
    

/*! 
@typedef JSShouldTerminateCallback
@abstract The callback invoked when script execution has exceeded the allowed
 time limit previously specified via TiContextGroupSetExecutionTimeLimit.
@param ctx The execution context to use.
@param context User specified context data previously passed to
 TiContextGroupSetExecutionTimeLimit.
@discussion If you named your function Callback, you would declare it like this:

 bool Callback(TiContextRef ctx, void* context);

 If you return true, the timed out script will terminate.
 If you return false, the script will run for another period of the allowed
 time limit specified via TiContextGroupSetExecutionTimeLimit.

 Within this callback function, you may call TiContextGroupSetExecutionTimeLimit
 to set a new time limit, or TiContextGroupClearExecutionTimeLimit to cancel the
 timeout.
*/
typedef bool
(*JSShouldTerminateCallback) (TiContextRef ctx, void* context);

/*!
@function
@abstract Sets the script execution time limit.
@param group The JavaScript context group that this time limit applies to.
@param limit The time limit of allowed script execution time in seconds.
@param callback The callback function that will be invoked when the time limit
 has been reached. This will give you a chance to decide if you want to
 terminate the script or not. If you pass a NULL callback, the script will be
 terminated unconditionally when the time limit has been reached.
@param context User data that you can provide to be passed back to you
 in your callback.

 In order to guarantee that the execution time limit will take effect, you will
 need to call TiContextGroupSetExecutionTimeLimit before you start executing
 any scripts.
*/
JS_EXPORT void TiContextGroupSetExecutionTimeLimit(TiContextGroupRef, double limit, JSShouldTerminateCallback, void* context) CF_AVAILABLE(10_6, 7_0);

/*!
@function
@abstract Clears the script execution time limit.
@param group The JavaScript context group that the time limit is cleared on.
*/
JS_EXPORT void TiContextGroupClearExecutionTimeLimit(TiContextGroupRef) CF_AVAILABLE(10_6, 7_0);

#ifdef __cplusplus
}
#endif

#endif /* TiContextRefPrivate_h */
