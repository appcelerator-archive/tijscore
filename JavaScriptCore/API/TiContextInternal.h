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

#ifndef TiContextInternal_h
#define TiContextInternal_h

#import <JavaScriptCore/TiCore.h>

#if JSC_OBJC_API_ENABLED

#import <JavaScriptCore/TiContext.h>

struct CallbackData {
    CallbackData *next;
    TiContext *context;
    TiValue *preservedException;
    TiValueRef thisValue;
    size_t argumentCount;
    const TiValueRef *arguments;
    NSArray *currentArguments;
};

class WeakContextRef {
public:
    WeakContextRef(TiContext * = nil);
    ~WeakContextRef();

    TiContext * get();
    void set(TiContext *);

private:
    TiContext *m_weakContext;
};

@class JSWrapperMap;

@interface TiContext(Internal)

- (id)initWithGlobalContextRef:(TiGlobalContextRef)context;

- (void)notifyException:(TiValueRef)exception;
- (TiValue *)valueFromNotifyException:(TiValueRef)exception;
- (BOOL)boolFromNotifyException:(TiValueRef)exception;

- (void)beginCallbackWithData:(CallbackData *)callbackData thisValue:(TiValueRef)thisValue argumentCount:(size_t)argumentCount arguments:(const TiValueRef *)arguments;
- (void)endCallbackWithData:(CallbackData *)callbackData;

- (TiValue *)wrapperForObjCObject:(id)object;
- (TiValue *)wrapperForJSObject:(TiValueRef)value;

@property (readonly, retain) JSWrapperMap *wrapperMap;

@end

#endif

#endif // TiContextInternal_h
