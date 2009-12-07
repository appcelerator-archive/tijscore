/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
#include "DebuggerActivation.h"

#include "JSActivation.h"

namespace TI {

DebuggerActivation::DebuggerActivation(TiObject* activation)
    : TiObject(DebuggerActivation::createStructure(jsNull()))
{
    ASSERT(activation);
    ASSERT(activation->isActivationObject());
    m_activation = static_cast<JSActivation*>(activation);
}

void DebuggerActivation::markChildren(MarkStack& markStack)
{
    TiObject::markChildren(markStack);

    if (m_activation)
        markStack.append(m_activation);
}

UString DebuggerActivation::className() const
{
    return m_activation->className();
}

bool DebuggerActivation::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return m_activation->getOwnPropertySlot(exec, propertyName, slot);
}

void DebuggerActivation::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    m_activation->put(exec, propertyName, value, slot);
}

void DebuggerActivation::putWithAttributes(TiExcState* exec, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    m_activation->putWithAttributes(exec, propertyName, value, attributes);
}

bool DebuggerActivation::deleteProperty(TiExcState* exec, const Identifier& propertyName)
{
    return m_activation->deleteProperty(exec, propertyName);
}

void DebuggerActivation::getOwnPropertyNames(TiExcState* exec, PropertyNameArray& propertyNames)
{
    m_activation->getPropertyNames(exec, propertyNames);
}

bool DebuggerActivation::getPropertyAttributes(TI::TiExcState* exec, const Identifier& propertyName, unsigned& attributes) const
{
    return m_activation->getPropertyAttributes(exec, propertyName, attributes);
}

void DebuggerActivation::defineGetter(TiExcState* exec, const Identifier& propertyName, TiObject* getterFunction, unsigned attributes)
{
    m_activation->defineGetter(exec, propertyName, getterFunction, attributes);
}

void DebuggerActivation::defineSetter(TiExcState* exec, const Identifier& propertyName, TiObject* setterFunction, unsigned attributes)
{
    m_activation->defineSetter(exec, propertyName, setterFunction, attributes);
}

TiValue DebuggerActivation::lookupGetter(TiExcState* exec, const Identifier& propertyName)
{
    return m_activation->lookupGetter(exec, propertyName);
}

TiValue DebuggerActivation::lookupSetter(TiExcState* exec, const Identifier& propertyName)
{
    return m_activation->lookupSetter(exec, propertyName);
}

} // namespace TI
