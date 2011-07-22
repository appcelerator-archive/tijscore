/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef TiObjectRefPrivate_h
#define TiObjectRefPrivate_h

#include <TiCore/TiObjectRef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 @function
 @abstract Sets a private property on an object.  This private property cannot be accessed from within Ti.
 @param ctx The execution context to use.
 @param object The TiObject whose private property you want to set.
 @param propertyName A TiString containing the property's name.
 @param value A TiValue to use as the property's value.  This may be NULL.
 @result true if object can store private data, otherwise false.
 @discussion This API allows you to store JS values directly an object in a way that will be ensure that they are kept alive without exposing them to Ti code and without introducing the reference cycles that may occur when using TiValueProtect.

 The default object class does not allocate storage for private data. Only objects created with a non-NULL TiClass can store private properties.
 */
JS_EXPORT bool TiObjectSetPrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName, TiValueRef value);

/*!
 @function
 @abstract Gets a private property from an object.
 @param ctx The execution context to use.
 @param object The TiObject whose private property you want to get.
 @param propertyName A TiString containing the property's name.
 @result The property's value if object has the property, otherwise NULL.
 */
JS_EXPORT TiValueRef TiObjectGetPrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName);

/*!
 @function
 @abstract Deletes a private property from an object.
 @param ctx The execution context to use.
 @param object The TiObject whose private property you want to delete.
 @param propertyName A TiString containing the property's name.
 @result true if object can store private data, otherwise false.
 @discussion The default object class does not allocate storage for private data. Only objects created with a non-NULL TiClass can store private data.
 */
JS_EXPORT bool TiObjectDeletePrivateProperty(TiContextRef ctx, TiObjectRef object, TiStringRef propertyName);

#ifdef __cplusplus
}
#endif

#endif // TiObjectRefPrivate_h
