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

#include "config.h"
#include "SetIteratorPrototype.h"

#include "JSCTiValueInlines.h"
#include "JSCellInlines.h"
#include "JSSetIterator.h"

namespace TI {

const ClassInfo SetIteratorPrototype::s_info = { "Set Iterator", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(SetIteratorPrototype) };

static EncodedTiValue JSC_HOST_CALL SetIteratorPrototypeFuncIterator(ExecState*);
static EncodedTiValue JSC_HOST_CALL SetIteratorPrototypeFuncNext(ExecState*);


void SetIteratorPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);

    JSC_NATIVE_FUNCTION(vm.propertyNames->iteratorPrivateName, SetIteratorPrototypeFuncIterator, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->iteratorNextPrivateName, SetIteratorPrototypeFuncNext, DontEnum, 0);
}

EncodedTiValue JSC_HOST_CALL SetIteratorPrototypeFuncIterator(CallFrame* callFrame)
{
    return TiValue::encode(callFrame->thisValue());
}

EncodedTiValue JSC_HOST_CALL SetIteratorPrototypeFuncNext(CallFrame* callFrame)
{
    TiValue result;
    JSSetIterator* iterator = jsDynamicCast<JSSetIterator*>(callFrame->thisValue());
    if (!iterator)
        return TiValue::encode(throwTypeError(callFrame, ASCIILiteral("Cannot call SetIterator.next() on a non-SetIterator object")));

    if (iterator->next(callFrame, result))
        return TiValue::encode(result);

    return TiValue::encode(callFrame->vm().iterationTerminator.get());
}


}
