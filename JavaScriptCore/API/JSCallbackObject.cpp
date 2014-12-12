/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "JSCallbackObject.h"

#include "Heap.h"
#include "Operations.h"
#include <wtf/text/StringHash.h>

namespace TI {

// Define the two types of JSCallbackObjects we support.
template <> const ClassInfo JSCallbackObject<JSDestructibleObject>::s_info = { "CallbackObject", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSCallbackObject) };
template <> const ClassInfo JSCallbackObject<JSGlobalObject>::s_info = { "CallbackGlobalObject", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSCallbackObject) };

template<> const bool JSCallbackObject<JSDestructibleObject>::needsDestruction = true;
template<> const bool JSCallbackObject<JSGlobalObject>::needsDestruction = false;

template<>
JSCallbackObject<JSGlobalObject>* JSCallbackObject<JSGlobalObject>::create(VM& vm, TiClassRef classRef, Structure* structure)
{
    JSCallbackObject<JSGlobalObject>* callbackObject = new (NotNull, allocateCell<JSCallbackObject<JSGlobalObject>>(vm.heap)) JSCallbackObject(vm, classRef, structure);
    callbackObject->finishCreation(vm);
    vm.heap.addFinalizer(callbackObject, destroy);
    return callbackObject;
}

template <>
Structure* JSCallbackObject<JSDestructibleObject>::createStructure(VM& vm, JSGlobalObject* globalObject, TiValue proto)
{ 
    return Structure::create(vm, globalObject, proto, TypeInfo(ObjectType, StructureFlags), info()); 
}
    
template <>
Structure* JSCallbackObject<JSGlobalObject>::createStructure(VM& vm, JSGlobalObject* globalObject, TiValue proto)
{ 
    return Structure::create(vm, globalObject, proto, TypeInfo(GlobalObjectType, StructureFlags), info()); 
}

void JSCallbackObjectData::finalize(Handle<Unknown> handle, void* context)
{
    TiClassRef jsClass = static_cast<TiClassRef>(context);
    TiObjectRef thisRef = toRef(static_cast<JSObject*>(handle.get().asCell()));
    
    for (; jsClass; jsClass = jsClass->parentClass)
        if (TiObjectFinalizeCallback finalize = jsClass->finalize)
            finalize(thisRef);
    WeakSet::deallocate(WeakImpl::asWeakImpl(handle.slot()));
}
    
} // namespace TI
