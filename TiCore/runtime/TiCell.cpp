/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All rights reserved.
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
#include "TiCell.h"

#include "TiFunction.h"
#include "TiString.h"
#include "TiObject.h"
#include <wtf/MathExtras.h>

namespace TI {

#if defined NAN && defined INFINITY

extern const double NaN = NAN;
extern const double Inf = INFINITY;

#else // !(defined NAN && defined INFINITY)

// The trick is to define the NaN and Inf globals with a different type than the declaration.
// This trick works because the mangled name of the globals does not include the type, although
// I'm not sure that's guaranteed. There could be alignment issues with this, since arrays of
// characters don't necessarily need the same alignment doubles do, but for now it seems to work.
// It would be good to figure out a 100% clean way that still avoids code that runs at init time.

// Note, we have to use union to ensure alignment. Otherwise, NaN_Bytes can start anywhere,
// while NaN_double has to be 4-byte aligned for 32-bits.
// With -fstrict-aliasing enabled, unions are the only safe way to do type masquerading.

static const union {
    struct {
        unsigned char NaN_Bytes[8];
        unsigned char Inf_Bytes[8];
    } bytes;
    
    struct {
        double NaN_Double;
        double Inf_Double;
    } doubles;
    
} NaNInf = { {
#if CPU(BIG_ENDIAN)
    { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 },
    { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 }
#elif CPU(MIDDLE_ENDIAN)
    { 0, 0, 0xf8, 0x7f, 0, 0, 0, 0 },
    { 0, 0, 0xf0, 0x7f, 0, 0, 0, 0 }
#else
    { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f },
    { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f }
#endif
} } ;

extern const double NaN = NaNInf.doubles.NaN_Double;
extern const double Inf = NaNInf.doubles.Inf_Double;
 
#endif // !(defined NAN && defined INFINITY)

bool TiCell::getUInt32(uint32_t&) const
{
    return false;
}

bool TiCell::getString(TiExcState* exec, UString&stringValue) const
{
    if (!isString())
        return false;
    stringValue = static_cast<const TiString*>(this)->value(exec);
    return true;
}

UString TiCell::getString(TiExcState* exec) const
{
    return isString() ? static_cast<const TiString*>(this)->value(exec) : UString();
}

TiObject* TiCell::getObject()
{
    return isObject() ? asObject(this) : 0;
}

const TiObject* TiCell::getObject() const
{
    return isObject() ? static_cast<const TiObject*>(this) : 0;
}

CallType TiCell::getCallData(CallData&)
{
    return CallTypeNone;
}

ConstructType TiCell::getConstructData(ConstructData&)
{
    return ConstructTypeNone;
}

bool TiCell::getOwnPropertySlot(TiExcState* exec, const Identifier& identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by TiValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    TiObject* object = toObject(exec);
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

bool TiCell::getOwnPropertySlot(TiExcState* exec, unsigned identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by TiValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    TiObject* object = toObject(exec);
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

void TiCell::put(TiExcState* exec, const Identifier& identifier, TiValue value, PutPropertySlot& slot)
{
    toObject(exec)->put(exec, identifier, value, slot);
}

void TiCell::put(TiExcState* exec, unsigned identifier, TiValue value)
{
    toObject(exec)->put(exec, identifier, value);
}

bool TiCell::deleteProperty(TiExcState* exec, const Identifier& identifier)
{
    return toObject(exec)->deleteProperty(exec, identifier);
}

bool TiCell::deleteProperty(TiExcState* exec, unsigned identifier)
{
    return toObject(exec)->deleteProperty(exec, identifier);
}

TiObject* TiCell::toThisObject(TiExcState* exec) const
{
    return toObject(exec);
}

const ClassInfo* TiCell::classInfo() const
{
    return 0;
}

TiValue TiCell::getJSNumber()
{
    return TiValue();
}

bool TiCell::isGetterSetter() const
{
    return false;
}

TiValue TiCell::toPrimitive(TiExcState*, PreferredPrimitiveType) const
{
    ASSERT_NOT_REACHED();
    return TiValue();
}

bool TiCell::getPrimitiveNumber(TiExcState*, double&, TiValue&)
{
    ASSERT_NOT_REACHED();
    return false;
}

bool TiCell::toBoolean(TiExcState*) const
{
    ASSERT_NOT_REACHED();
    return false;
}

double TiCell::toNumber(TiExcState*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

UString TiCell::toString(TiExcState*) const
{
    ASSERT_NOT_REACHED();
    return UString();
}

TiObject* TiCell::toObject(TiExcState*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

} // namespace TI
