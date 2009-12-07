/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "StringObject.h"

#include "PropertyNameArray.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(StringObject);

const ClassInfo StringObject::info = { "String", 0, 0, 0 };

StringObject::StringObject(TiExcState* exec, NonNullPassRefPtr<Structure> structure)
    : JSWrapperObject(structure)
{
    setInternalValue(jsEmptyString(exec));
}

StringObject::StringObject(NonNullPassRefPtr<Structure> structure, TiString* string)
    : JSWrapperObject(structure)
{
    setInternalValue(string);
}

StringObject::StringObject(TiExcState* exec, NonNullPassRefPtr<Structure> structure, const UString& string)
    : JSWrapperObject(structure)
{
    setInternalValue(jsString(exec, string));
}

bool StringObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (internalValue()->getStringPropertySlot(exec, propertyName, slot))
        return true;
    return TiObject::getOwnPropertySlot(exec, propertyName, slot);
}
    
bool StringObject::getOwnPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (internalValue()->getStringPropertySlot(exec, propertyName, slot))
        return true;    
    return TiObject::getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

bool StringObject::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (internalValue()->getStringPropertyDescriptor(exec, propertyName, descriptor))
        return true;    
    return TiObject::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void StringObject::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    if (propertyName == exec->propertyNames().length)
        return;
    TiObject::put(exec, propertyName, value, slot);
}

bool StringObject::deleteProperty(TiExcState* exec, const Identifier& propertyName)
{
    if (propertyName == exec->propertyNames().length)
        return false;
    return TiObject::deleteProperty(exec, propertyName);
}

void StringObject::getOwnPropertyNames(TiExcState* exec, PropertyNameArray& propertyNames)
{
    int size = internalValue()->value().size();
    for (int i = 0; i < size; ++i)
        propertyNames.add(Identifier(exec, UString::from(i)));
    return TiObject::getOwnPropertyNames(exec, propertyNames);
}

} // namespace TI
