/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#include "TiStaticScopeObject.h"

#include "Error.h"

namespace TI {
ASSERT_CLASS_FITS_IN_CELL(TiStaticScopeObject);

void TiStaticScopeObject::visitChildren(SlotVisitor& visitor)
{
    ASSERT_GC_OBJECT_INHERITS(this, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(structure()->typeInfo().overridesVisitChildren());
    JSVariableObject::visitChildren(visitor);
    visitor.append(&m_registerStore);
}

TiObject* TiStaticScopeObject::toThisObject(TiExcState* exec) const
{
    return exec->globalThisValue();
}

TiValue TiStaticScopeObject::toStrictThisObject(TiExcState*) const
{
    return jsNull();
}

void TiStaticScopeObject::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    if (slot.isStrictMode()) {
        // Double lookup in strict mode, but this only occurs when
        // a) indirectly writing to an exception slot
        // b) writing to a function expression name
        // (a) is unlikely, and (b) is an error.
        // Also with a single entry the symbol table lookup should simply be
        // a pointer compare.
        PropertySlot slot;
        bool isWritable = true;
        symbolTableGet(propertyName, slot, isWritable);
        if (!isWritable) {
            throwError(exec, createTypeError(exec, StrictModeReadonlyPropertyWriteError));
            return;
        }
    }
    if (symbolTablePut(exec->globalData(), propertyName, value))
        return;
    
    ASSERT_NOT_REACHED();
}

void TiStaticScopeObject::putWithAttributes(TiExcState* exec, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    if (symbolTablePutWithAttributes(exec->globalData(), propertyName, value, attributes))
        return;
    
    ASSERT_NOT_REACHED();
}

bool TiStaticScopeObject::isDynamicScope(bool&) const
{
    return false;
}

bool TiStaticScopeObject::getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot& slot)
{
    return symbolTableGet(propertyName, slot);
}

}
