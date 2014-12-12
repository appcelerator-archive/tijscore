/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef JSBoundFunction_h
#define JSBoundFunction_h

#include "JSFunction.h"

namespace TI {

EncodedTiValue JSC_HOST_CALL boundFunctionCall(ExecState*);
EncodedTiValue JSC_HOST_CALL boundFunctionConstruct(ExecState*);

class JSBoundFunction : public JSFunction {
public:
    typedef JSFunction Base;

    static JSBoundFunction* create(VM&, JSGlobalObject*, JSObject* targetFunction, TiValue boundThis, TiValue boundArgs, int, const String&);
    
    static void destroy(JSCell*);

    static bool customHasInstance(JSObject*, ExecState*, TiValue);

    JSObject* targetFunction() { return m_targetFunction.get(); }
    TiValue boundThis() { return m_boundThis.get(); }
    TiValue boundArgs() { return m_boundArgs.get(); }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype) 
    {
        ASSERT(globalObject);
        return Structure::create(vm, globalObject, prototype, TypeInfo(JSFunctionType, StructureFlags), info()); 
    }

    DECLARE_INFO;

protected:
    const static unsigned StructureFlags = OverridesHasInstance | OverridesVisitChildren | Base::StructureFlags;

    static void visitChildren(JSCell*, SlotVisitor&);

private:
    JSBoundFunction(VM&, JSGlobalObject*, Structure*, JSObject* targetFunction, TiValue boundThis, TiValue boundArgs);
    
    void finishCreation(VM&, NativeExecutable*, int, const String&);

    WriteBarrier<JSObject> m_targetFunction;
    WriteBarrier<Unknown> m_boundThis;
    WriteBarrier<Unknown> m_boundArgs;
};

} // namespace TI

#endif // JSFunction_h
