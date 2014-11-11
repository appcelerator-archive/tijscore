/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef TiBase_h
#define TiBase_h

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

/* JavaScript engine interface */

/*! @typedef TiContextGroupRef A group that associates JavaScript contexts with one another. Contexts in the same group may share and exchange JavaScript objects. */
typedef const struct OpaqueTiContextGroup* TiContextGroupRef;

/*! @typedef TiContextRef A JavaScript execution context. Holds the global object and other execution state. */
typedef const struct OpaqueTiContext* TiContextRef;

/*! @typedef TiGlobalContextRef A global JavaScript execution context. A TiGlobalContext is a TiContext. */
typedef struct OpaqueTiContext* TiGlobalContextRef;

/*! @typedef TiStringRef A UTF16 character buffer. The fundamental string representation in JavaScript. */
typedef struct OpaqueTiString* TiStringRef;

/*! @typedef TiClassRef A JavaScript class. Used with TiObjectMake to construct objects with custom behavior. */
typedef struct OpaqueTiClass* TiClassRef;

/*! @typedef TiPropertyNameArrayRef An array of JavaScript property names. */
typedef struct OpaqueTiPropertyNameArray* TiPropertyNameArrayRef;

/*! @typedef TiPropertyNameAccumulatorRef An ordered set used to collect the names of a JavaScript object's properties. */
typedef struct OpaqueTiPropertyNameAccumulator* TiPropertyNameAccumulatorRef;


/* JavaScript data types */

/*! @typedef TiValueRef A JavaScript value. The base type for all JavaScript values, and polymorphic functions on them. */
typedef const struct OpaqueTiValue* TiValueRef;

/*! @typedef TiObjectRef A JavaScript object. A JSObject is a TiValue. */
typedef struct OpaqueTiValue* TiObjectRef;

/* JavaScript symbol exports */
/* These rules should stay the same as in WebKit2/Shared/API/c/WKBase.h */

#undef JS_EXPORT
#if defined(JS_NO_EXPORT)
#define JS_EXPORT
#elif defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC__)
#define JS_EXPORT __attribute__((visibility("default")))
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_WCE) || defined(__CC_ARM) || defined(__ARMCC__)
#if defined(BUILDING_JavaScriptCore) || defined(STATICALLY_LINKED_WITH_JavaScriptCore)
#define JS_EXPORT __declspec(dllexport)
#else
#define JS_EXPORT __declspec(dllimport)
#endif
#else /* !defined(JS_NO_EXPORT) */
#define JS_EXPORT
#endif /* defined(JS_NO_EXPORT) */

/* JS tests uses WTF but has no config.h, so we need to set the export defines here. */
#ifndef WTF_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE JS_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Script Evaluation */

/*!
@function TiEvalScript
@abstract Evaluates a string of JavaScript.
@param ctx The execution context to use.
@param script A JSString containing the script to evaluate.
@param thisObject The object to use as "this," or NULL to use the global object as "this."
@param sourceURL A JSString containing a URL for the script's source file. This is only used when reporting exceptions. Pass NULL if you do not care to include source file information in exceptions.
@param startingLineNumber An integer value specifying the script's starting line number in the file located at sourceURL. This is only used when reporting exceptions.
@param exception A pointer to a TiValueRef in which to store an exception, if any. Pass NULL if you do not care to store an exception.
@result The TiValue that results from evaluating script, or NULL if an exception is thrown.
*/
JS_EXPORT TiValueRef TiEvalScript(TiContextRef ctx, TiStringRef script, TiObjectRef thisObject, TiStringRef sourceURL, int startingLineNumber, TiValueRef* exception);

/*!
@function TiCheckScriptSyntax
@abstract Checks for syntax errors in a string of JavaScript.
@param ctx The execution context to use.
@param script A JSString containing the script to check for syntax errors.
@param sourceURL A JSString containing a URL for the script's source file. This is only used when reporting exceptions. Pass NULL if you do not care to include source file information in exceptions.
@param startingLineNumber An integer value specifying the script's starting line number in the file located at sourceURL. This is only used when reporting exceptions.
@param exception A pointer to a TiValueRef in which to store a syntax error exception, if any. Pass NULL if you do not care to store a syntax error exception.
@result true if the script is syntactically correct, otherwise false.
*/
JS_EXPORT bool TiCheckScriptSyntax(TiContextRef ctx, TiStringRef script, TiStringRef sourceURL, int startingLineNumber, TiValueRef* exception);

/*!
@function TiGarbageCollect
@abstract Performs a JavaScript garbage collection. 
@param ctx The execution context to use.
@discussion JavaScript values that are on the machine stack, in a register, 
 protected by TiValueProtect, set as the global object of an execution context, 
 or reachable from any such value will not be collected.

 During JavaScript execution, you are not required to call this function; the 
 JavaScript engine will garbage collect as needed. JavaScript values created
 within a context group are automatically destroyed when the last reference
 to the context group is released.
*/
JS_EXPORT void TiGarbageCollect(TiContextRef ctx);

#ifdef __cplusplus
}
#endif

/* Enable the Objective-C API for platforms with a modern runtime. */
#if !defined(JSC_OBJC_API_ENABLED)
#ifndef JSC_OBJC_API_AVAILABLE_MAC_OS_X_1080
#define JSC_OBJC_API_ENABLED (defined(__clang__) && defined(__APPLE__) && ((defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090 && !defined(__i386__)) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)))
#else
#define JSC_OBJC_API_ENABLED (defined(__clang__) && defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080 && !defined(__i386__))
#endif
#endif

#endif /* TiBase_h */
