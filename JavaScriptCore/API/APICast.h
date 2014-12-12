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

#ifndef APICast_h
#define APICast_h

#include "JSAPIValueWrapper.h"
#include "JSCTiValue.h"
#include "JSCTiValueInlines.h"
#include "JSGlobalObject.h"

namespace TI {
    class ExecState;
    class PropertyNameArray;
    class VM;
    class JSObject;
    class TiValue;
}

typedef const struct OpaqueTiContextGroup* TiContextGroupRef;
typedef const struct OpaqueTiContext* TiContextRef;
typedef struct OpaqueTiContext* TiGlobalContextRef;
typedef struct OpaqueTiPropertyNameAccumulator* TiPropertyNameAccumulatorRef;
typedef const struct OpaqueTiValue* TiValueRef;
typedef struct OpaqueTiValue* TiObjectRef;

/* Opaque typing convenience methods */

inline TI::ExecState* toJS(TiContextRef c)
{
    ASSERT(c);
    return reinterpret_cast<TI::ExecState*>(const_cast<OpaqueTiContext*>(c));
}

inline TI::ExecState* toJS(TiGlobalContextRef c)
{
    ASSERT(c);
    return reinterpret_cast<TI::ExecState*>(c);
}

inline TI::TiValue toJS(TI::ExecState* exec, TiValueRef v)
{
    ASSERT_UNUSED(exec, exec);
#if USE(JSVALUE32_64)
    TI::JSCell* jsCell = reinterpret_cast<TI::JSCell*>(const_cast<OpaqueTiValue*>(v));
    if (!jsCell)
        return TI::jsNull();
    TI::TiValue result;
    if (jsCell->isAPIValueWrapper())
        result = TI::jsCast<TI::JSAPIValueWrapper*>(jsCell)->value();
    else
        result = jsCell;
#else
    TI::TiValue result = TI::TiValue::decode(reinterpret_cast<TI::EncodedTiValue>(const_cast<OpaqueTiValue*>(v)));
#endif
    if (!result)
        return TI::jsNull();
    if (result.isCell())
        RELEASE_ASSERT(result.asCell()->methodTable());
    return result;
}

inline TI::TiValue toJSForGC(TI::ExecState* exec, TiValueRef v)
{
    ASSERT_UNUSED(exec, exec);
#if USE(JSVALUE32_64)
    TI::JSCell* jsCell = reinterpret_cast<TI::JSCell*>(const_cast<OpaqueTiValue*>(v));
    if (!jsCell)
        return TI::TiValue();
    TI::TiValue result = jsCell;
#else
    TI::TiValue result = TI::TiValue::decode(reinterpret_cast<TI::EncodedTiValue>(const_cast<OpaqueTiValue*>(v)));
#endif
    if (result && result.isCell())
        RELEASE_ASSERT(result.asCell()->methodTable());
    return result;
}

// Used in TiObjectGetPrivate as that may be called during finalization
inline TI::JSObject* uncheckedToJS(TiObjectRef o)
{
    return reinterpret_cast<TI::JSObject*>(o);
}

inline TI::JSObject* toJS(TiObjectRef o)
{
    TI::JSObject* object = uncheckedToJS(o);
    if (object)
        RELEASE_ASSERT(object->methodTable());
    return object;
}

inline TI::PropertyNameArray* toJS(TiPropertyNameAccumulatorRef a)
{
    return reinterpret_cast<TI::PropertyNameArray*>(a);
}

inline TI::VM* toJS(TiContextGroupRef g)
{
    return reinterpret_cast<TI::VM*>(const_cast<OpaqueTiContextGroup*>(g));
}

inline TiValueRef toRef(TI::ExecState* exec, TI::TiValue v)
{
#if USE(JSVALUE32_64)
    if (!v)
        return 0;
    if (!v.isCell())
        return reinterpret_cast<TiValueRef>(TI::jsAPIValueWrapper(exec, v).asCell());
    return reinterpret_cast<TiValueRef>(v.asCell());
#else
    UNUSED_PARAM(exec);
    return reinterpret_cast<TiValueRef>(TI::TiValue::encode(v));
#endif
}

inline TiObjectRef toRef(TI::JSObject* o)
{
    return reinterpret_cast<TiObjectRef>(o);
}

inline TiObjectRef toRef(const TI::JSObject* o)
{
    return reinterpret_cast<TiObjectRef>(const_cast<TI::JSObject*>(o));
}

inline TiContextRef toRef(TI::ExecState* e)
{
    return reinterpret_cast<TiContextRef>(e);
}

inline TiGlobalContextRef toGlobalRef(TI::ExecState* e)
{
    ASSERT(e == e->lexicalGlobalObject()->globalExec());
    return reinterpret_cast<TiGlobalContextRef>(e);
}

inline TiPropertyNameAccumulatorRef toRef(TI::PropertyNameArray* l)
{
    return reinterpret_cast<TiPropertyNameAccumulatorRef>(l);
}

inline TiContextGroupRef toRef(TI::VM* g)
{
    return reinterpret_cast<TiContextGroupRef>(g);
}

#endif // APICast_h
