/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple, Inc. All rights reserved.
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

#ifndef ArgumentsIteratorConstructor_h
#define ArgumentsIteratorConstructor_h

#include "InternalFunction.h"

namespace TI {

class ArgumentsIteratorPrototype;

class ArgumentsIteratorConstructor : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    static ArgumentsIteratorConstructor* create(VM& vm, Structure* structure, ArgumentsIteratorPrototype* prototype)
    {
        ArgumentsIteratorConstructor* constructor = new (NotNull, allocateCell<ArgumentsIteratorConstructor>(vm.heap)) ArgumentsIteratorConstructor(vm, structure);
        constructor->finishCreation(vm, prototype);
        return constructor;
    }

    DECLARE_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

private:
    ArgumentsIteratorConstructor(VM& vm, Structure* structure)
        : Base(vm, structure)
    {
    }
    void finishCreation(VM&, ArgumentsIteratorPrototype*);
};

}

#endif // !defined(ArgumentsIteratorConstructor_h)
