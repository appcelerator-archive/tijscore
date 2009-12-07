/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "TiString.h"

#include "TiGlobalObject.h"
#include "TiObject.h"
#include "StringObject.h"
#include "StringPrototype.h"

namespace TI {

TiValue TiString::toPrimitive(TiExcState*, PreferredPrimitiveType) const
{
    return const_cast<TiString*>(this);
}

bool TiString::getPrimitiveNumber(TiExcState*, double& number, TiValue& value)
{
    value = this;
    number = m_value.toDouble();
    return false;
}

bool TiString::toBoolean(TiExcState*) const
{
    return !m_value.isEmpty();
}

double TiString::toNumber(TiExcState*) const
{
    return m_value.toDouble();
}

UString TiString::toString(TiExcState*) const
{
    return m_value;
}

UString TiString::toThisString(TiExcState*) const
{
    return m_value;
}

TiString* TiString::toThisTiString(TiExcState*)
{
    return this;
}

inline StringObject* StringObject::create(TiExcState* exec, TiString* string)
{
    return new (exec) StringObject(exec->lexicalGlobalObject()->stringObjectStructure(), string);
}

TiObject* TiString::toObject(TiExcState* exec) const
{
    return StringObject::create(exec, const_cast<TiString*>(this));
}

TiObject* TiString::toThisObject(TiExcState* exec) const
{
    return StringObject::create(exec, const_cast<TiString*>(this));
}

bool TiString::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by TiValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    if (propertyName == exec->propertyNames().underscoreProto) {
        slot.setValue(exec->lexicalGlobalObject()->stringPrototype());
        return true;
    }
    slot.setBase(this);
    TiObject* object;
    for (TiValue prototype = exec->lexicalGlobalObject()->stringPrototype(); !prototype.isNull(); prototype = object->prototype()) {
        object = asObject(prototype);
        if (object->getOwnPropertySlot(exec, propertyName, slot))
            return true;
    }
    slot.setUndefined();
    return true;
}

bool TiString::getStringPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == exec->propertyNames().length) {
        descriptor.setDescriptor(jsNumber(exec, m_value.size()), DontEnum | DontDelete | ReadOnly);
        return true;
    }
    
    bool isStrictUInt32;
    unsigned i = propertyName.toStrictUInt32(&isStrictUInt32);
    if (isStrictUInt32 && i < static_cast<unsigned>(m_value.size())) {
        descriptor.setDescriptor(jsSingleCharacterSubstring(exec, m_value, i), DontDelete | ReadOnly);
        return true;
    }
    
    return false;
}

bool TiString::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (getStringPropertyDescriptor(exec, propertyName, descriptor))
        return true;
    if (propertyName != exec->propertyNames().underscoreProto)
        return false;
    descriptor.setDescriptor(exec->lexicalGlobalObject()->stringPrototype(), DontEnum);
    return true;
}

bool TiString::getOwnPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by TiValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    return TiString::getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

} // namespace TI
