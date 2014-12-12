/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef JSCallbackConstructor_h
#define JSCallbackConstructor_h

#include "TiObjectRef.h"
#include "runtime/JSDestructibleObject.h"

namespace TI {

class JSCallbackConstructor : public JSDestructibleObject {
public:
    typedef JSDestructibleObject Base;

    static JSCallbackConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, TiClassRef classRef, TiObjectCallAsConstructorCallback callback) 
    {
        JSCallbackConstructor* constructor = new (NotNull, allocateCell<JSCallbackConstructor>(*exec->heap())) JSCallbackConstructor(globalObject, structure, classRef, callback);
        constructor->finishCreation(globalObject, classRef);
        return constructor;
    }
    
    ~JSCallbackConstructor();
    static void destroy(JSCell*);
    TiClassRef classRef() const { return m_class; }
    TiObjectCallAsConstructorCallback callback() const { return m_callback; }
    DECLARE_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue proto) 
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(ObjectType, StructureFlags), info());
    }

protected:
    JSCallbackConstructor(JSGlobalObject*, Structure*, TiClassRef, TiObjectCallAsConstructorCallback);
    void finishCreation(JSGlobalObject*, TiClassRef);
    static const unsigned StructureFlags = ImplementsHasInstance | JSObject::StructureFlags;

private:
    friend struct APICallbackFunction;

    static ConstructType getConstructData(JSCell*, ConstructData&);

    TiObjectCallAsConstructorCallback constructCallback() { return m_callback; }

    TiClassRef m_class;
    TiObjectCallAsConstructorCallback m_callback;
};

} // namespace TI

#endif // JSCallbackConstructor_h
