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

#include "config.h"
#include "JSAPIWrapperObject.h"

#include "DelayedReleaseScope.h"
#include "JSCTiValueInlines.h"
#include "JSCallbackObject.h"
#include "JSCellInlines.h"
#include "TiVirtualMachineInternal.h"
#include "SlotVisitorInlines.h"
#include "Structure.h"
#include "StructureInlines.h"

#if JSC_OBJC_API_ENABLED

class JSAPIWrapperObjectHandleOwner : public TI::WeakHandleOwner {
public:
    virtual void finalize(TI::Handle<TI::Unknown>, void*) OVERRIDE;
    virtual bool isReachableFromOpaqueRoots(TI::Handle<TI::Unknown>, void* context, TI::SlotVisitor&) OVERRIDE;
};

static JSAPIWrapperObjectHandleOwner* jsAPIWrapperObjectHandleOwner()
{
    DEFINE_STATIC_LOCAL(JSAPIWrapperObjectHandleOwner, jsWrapperObjectHandleOwner, ());
    return &jsWrapperObjectHandleOwner;
}

void JSAPIWrapperObjectHandleOwner::finalize(TI::Handle<TI::Unknown> handle, void*)
{
    TI::JSAPIWrapperObject* wrapperObject = TI::jsCast<TI::JSAPIWrapperObject*>(handle.get().asCell());
    if (!wrapperObject->wrappedObject())
        return;

    TI::Heap::heap(wrapperObject)->releaseSoon(adoptNS(static_cast<id>(wrapperObject->wrappedObject())));
    TI::WeakSet::deallocate(TI::WeakImpl::asWeakImpl(handle.slot()));
}

bool JSAPIWrapperObjectHandleOwner::isReachableFromOpaqueRoots(TI::Handle<TI::Unknown> handle, void*, TI::SlotVisitor& visitor)
{
    TI::JSAPIWrapperObject* wrapperObject = TI::jsCast<TI::JSAPIWrapperObject*>(handle.get().asCell());
    // We use the JSGlobalObject when processing weak handles to prevent the situation where using
    // the same Objective-C object in multiple global objects keeps all of the global objects alive.
    if (!wrapperObject->wrappedObject())
        return false;
    return TI::Heap::isMarked(wrapperObject->structure()->globalObject()) && visitor.containsOpaqueRoot(wrapperObject->wrappedObject());
}

namespace TI {
    
template <> const ClassInfo JSCallbackObject<JSAPIWrapperObject>::s_info = { "JSAPIWrapperObject", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSCallbackObject) };

template<> const bool JSCallbackObject<JSAPIWrapperObject>::needsDestruction = true;

template <>
Structure* JSCallbackObject<JSAPIWrapperObject>::createStructure(VM& vm, JSGlobalObject* globalObject, TiValue proto)
{
    return Structure::create(vm, globalObject, proto, TypeInfo(ObjectType, StructureFlags), &s_info);
}

JSAPIWrapperObject::JSAPIWrapperObject(VM& vm, Structure* structure)
    : Base(vm, structure)
    , m_wrappedObject(0)
{
}

void JSAPIWrapperObject::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    WeakSet::allocate(this, jsAPIWrapperObjectHandleOwner(), 0); // Balanced in JSAPIWrapperObjectHandleOwner::finalize.
}
    
void JSAPIWrapperObject::setWrappedObject(void* wrappedObject)
{
    ASSERT(!m_wrappedObject);
    m_wrappedObject = [static_cast<id>(wrappedObject) retain];
}

void JSAPIWrapperObject::visitChildren(JSCell* cell, TI::SlotVisitor& visitor)
{
    JSAPIWrapperObject* thisObject = TI::jsCast<JSAPIWrapperObject*>(cell);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    Base::visitChildren(cell, visitor);

    if (thisObject->wrappedObject())
        scanExternalObjectGraph(cell->structure()->globalObject()->vm(), visitor, thisObject->wrappedObject());
}

} // namespace TI

#endif // JSC_OBJC_API_ENABLED
