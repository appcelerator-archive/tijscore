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
#include "JSNumberCell.h"

#if USE(JSVALUE32)

#include "NumberObject.h"
#include "UString.h"

namespace TI {

TiValue JSNumberCell::toPrimitive(TiExcState*, PreferredPrimitiveType) const
{
    return const_cast<JSNumberCell*>(this);
}

bool JSNumberCell::getPrimitiveNumber(TiExcState*, double& number, TiValue& value)
{
    number = m_value;
    value = this;
    return true;
}

bool JSNumberCell::toBoolean(TiExcState*) const
{
    return m_value < 0.0 || m_value > 0.0; // false for NaN
}

double JSNumberCell::toNumber(TiExcState*) const
{
  return m_value;
}

UString JSNumberCell::toString(TiExcState*) const
{
    return UString::from(m_value);
}

TiObject* JSNumberCell::toObject(TiExcState* exec) const
{
    return constructNumber(exec, const_cast<JSNumberCell*>(this));
}

TiObject* JSNumberCell::toThisObject(TiExcState* exec) const
{
    return constructNumber(exec, const_cast<JSNumberCell*>(this));
}

bool JSNumberCell::getUInt32(uint32_t& uint32) const
{
    uint32 = static_cast<uint32_t>(m_value);
    return uint32 == m_value;
}

TiValue JSNumberCell::getJSNumber()
{
    return this;
}

TiValue jsNumberCell(TiExcState* exec, double d)
{
    return new (exec) JSNumberCell(exec, d);
}

TiValue jsNumberCell(TiGlobalData* globalData, double d)
{
    return new (globalData) JSNumberCell(globalData, d);
}

} // namespace TI

#else // USE(JSVALUE32)

// Keep our exported symbols lists happy.
namespace TI {

TiValue jsNumberCell(TiExcState*, double);

TiValue jsNumberCell(TiExcState*, double)
{
    ASSERT_NOT_REACHED();
    return TiValue();
}

} // namespace TI

#endif // USE(JSVALUE32)
