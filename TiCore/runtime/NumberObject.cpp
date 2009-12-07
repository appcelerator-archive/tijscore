/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "NumberObject.h"

#include "TiGlobalObject.h"
#include "NumberPrototype.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(NumberObject);

const ClassInfo NumberObject::info = { "Number", 0, 0, 0 };

NumberObject::NumberObject(NonNullPassRefPtr<Structure> structure)
    : JSWrapperObject(structure)
{
}

TiValue NumberObject::getJSNumber()
{
    return internalValue();
}

NumberObject* constructNumber(TiExcState* exec, TiValue number)
{
    NumberObject* object = new (exec) NumberObject(exec->lexicalGlobalObject()->numberObjectStructure());
    object->setInternalValue(number);
    return object;
}

} // namespace TI
