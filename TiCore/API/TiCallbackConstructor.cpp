/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "TiCallbackConstructor.h"

#include "APIShims.h"
#include "APICast.h"
#include <runtime/TiGlobalObject.h>
#include <runtime/TiLock.h>
#include <runtime/ObjectPrototype.h>
#include <wtf/Vector.h>

namespace TI {

const ClassInfo TiCallbackConstructor::info = { "CallbackConstructor", 0, 0, 0 };

TiCallbackConstructor::TiCallbackConstructor(NonNullPassRefPtr<Structure> structure, TiClassRef jsClass, TiObjectCallAsConstructorCallback callback)
    : TiObject(structure)
    , m_class(jsClass)
    , m_callback(callback)
{
    if (m_class)
        TiClassRetain(jsClass);
}

TiCallbackConstructor::~TiCallbackConstructor()
{
    if (m_class)
        TiClassRelease(m_class);
}

static TiObject* constructTiCallback(TiExcState* exec, TiObject* constructor, const ArgList& args)
{
    TiContextRef ctx = toRef(exec);
    TiObjectRef constructorRef = toRef(constructor);

    TiObjectCallAsConstructorCallback callback = static_cast<TiCallbackConstructor*>(constructor)->callback();
    if (callback) {
        int argumentCount = static_cast<int>(args.size());
        Vector<TiValueRef, 16> arguments(argumentCount);
        for (int i = 0; i < argumentCount; i++)
            arguments[i] = toRef(exec, args.at(i));

        TiValueRef exception = 0;
        TiObjectRef result;
        {
            APICallbackShim callbackShim(exec);
            result = callback(ctx, constructorRef, argumentCount, arguments.data(), &exception);
        }
        if (exception)
            exec->setException(toJS(exec, exception));
        return toJS(result);
    }
    
    return toJS(TiObjectMake(ctx, static_cast<TiCallbackConstructor*>(constructor)->classRef(), 0));
}

ConstructType TiCallbackConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructTiCallback;
    return ConstructTypeHost;
}

} // namespace TI
