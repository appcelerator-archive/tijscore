/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef JSNumberCell_h
#define JSNumberCell_h

#include "CallFrame.h"
#include "TiCell.h"
#include "JSImmediate.h"
#include "Collector.h"
#include "UString.h"
#include <stddef.h> // for size_t

namespace TI {

    extern const double NaN;
    extern const double Inf;

#if USE(JSVALUE32)
    TiValue jsNumberCell(TiExcState*, double);

    class Identifier;
    class TiCell;
    class TiObject;
    class TiString;
    class PropertySlot;

    struct ClassInfo;
    struct Instruction;

    class JSNumberCell : public TiCell {
        friend class JIT;
        friend TiValue jsNumberCell(TiGlobalData*, double);
        friend TiValue jsNumberCell(TiExcState*, double);

    public:
        double value() const { return m_value; }

        virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(TiExcState*, double& number, TiValue& value);
        virtual bool toBoolean(TiExcState*) const;
        virtual double toNumber(TiExcState*) const;
        virtual UString toString(TiExcState*) const;
        virtual TiObject* toObject(TiExcState*) const;

        virtual TiObject* toThisObject(TiExcState*) const;
        virtual TiValue getJSNumber();

        void* operator new(size_t size, TiExcState* exec)
        {
            return exec->heap()->allocateNumber(size);
        }

        void* operator new(size_t size, TiGlobalData* globalData)
        {
            return globalData->heap.allocateNumber(size);
        }

        static PassRefPtr<Structure> createStructure(TiValue proto) { return Structure::create(proto, TypeInfo(NumberType, OverridesGetOwnPropertySlot | NeedsThisConversion), AnonymousSlotCount); }

    private:
        JSNumberCell(TiGlobalData* globalData, double value)
            : TiCell(globalData->numberStructure.get())
            , m_value(value)
        {
        }

        JSNumberCell(TiExcState* exec, double value)
            : TiCell(exec->globalData().numberStructure.get())
            , m_value(value)
        {
        }

        virtual bool getUInt32(uint32_t&) const;

        double m_value;
    };

    TiValue jsNumberCell(TiGlobalData*, double);

    inline bool isNumberCell(TiValue v)
    {
        return v.isCell() && v.asCell()->isNumber();
    }

    inline JSNumberCell* asNumberCell(TiValue v)
    {
        ASSERT(isNumberCell(v));
        return static_cast<JSNumberCell*>(v.asCell());
    }

    ALWAYS_INLINE TiValue::TiValue(EncodeAsDoubleTag, TiExcState* exec, double d)
    {
        *this = jsNumberCell(exec, d);
    }

    inline TiValue::TiValue(TiExcState* exec, double d)
    {
        TiValue v = JSImmediate::from(d);
        *this = v ? v : jsNumberCell(exec, d);
    }

    inline TiValue::TiValue(TiExcState* exec, int i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(exec, i);
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(exec, i);
    }

    inline TiValue::TiValue(TiExcState* exec, long i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(exec, i);
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned long i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(exec, i);
    }

    inline TiValue::TiValue(TiExcState* exec, long long i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(exec, static_cast<double>(i));
    }

    inline TiValue::TiValue(TiExcState* exec, unsigned long long i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(exec, static_cast<double>(i));
    }

    inline TiValue::TiValue(TiGlobalData* globalData, double d)
    {
        TiValue v = JSImmediate::from(d);
        *this = v ? v : jsNumberCell(globalData, d);
    }

    inline TiValue::TiValue(TiGlobalData* globalData, int i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(globalData, i);
    }

    inline TiValue::TiValue(TiGlobalData* globalData, unsigned i)
    {
        TiValue v = JSImmediate::from(i);
        *this = v ? v : jsNumberCell(globalData, i);
    }

    inline bool TiValue::isDouble() const
    {
        return isNumberCell(asValue());
    }

    inline double TiValue::asDouble() const
    {
        return asNumberCell(asValue())->value();
    }

    inline bool TiValue::isNumber() const
    {
        return JSImmediate::isNumber(asValue()) || isDouble();
    }

    inline double TiValue::uncheckedGetNumber() const
    {
        ASSERT(isNumber());
        return JSImmediate::isImmediate(asValue()) ? JSImmediate::toDouble(asValue()) : asDouble();
    }

#endif // USE(JSVALUE32)

#if USE(JSVALUE64)
    ALWAYS_INLINE TiValue::TiValue(EncodeAsDoubleTag, TiExcState*, double d)
    {
        *this = JSImmediate::fromNumberOutsideIntegerRange(d);
    }

    inline TiValue::TiValue(TiExcState*, double d)
    {
        TiValue v = JSImmediate::from(d);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiExcState*, int i)
    {
        TiValue v = JSImmediate::from(i);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiExcState*, unsigned i)
    {
        TiValue v = JSImmediate::from(i);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiExcState*, long i)
    {
        TiValue v = JSImmediate::from(i);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiExcState*, unsigned long i)
    {
        TiValue v = JSImmediate::from(i);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiExcState*, long long i)
    {
        TiValue v = JSImmediate::from(static_cast<double>(i));
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiExcState*, unsigned long long i)
    {
        TiValue v = JSImmediate::from(static_cast<double>(i));
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiGlobalData*, double d)
    {
        TiValue v = JSImmediate::from(d);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiGlobalData*, int i)
    {
        TiValue v = JSImmediate::from(i);
        ASSERT(v);
        *this = v;
    }

    inline TiValue::TiValue(TiGlobalData*, unsigned i)
    {
        TiValue v = JSImmediate::from(i);
        ASSERT(v);
        *this = v;
    }

    inline bool TiValue::isDouble() const
    {
        return JSImmediate::isDouble(asValue());
    }

    inline double TiValue::asDouble() const
    {
        return JSImmediate::doubleValue(asValue());
    }

    inline bool TiValue::isNumber() const
    {
        return JSImmediate::isNumber(asValue());
    }

    inline double TiValue::uncheckedGetNumber() const
    {
        ASSERT(isNumber());
        return JSImmediate::toDouble(asValue());
    }

#endif // USE(JSVALUE64)

#if USE(JSVALUE32) || USE(JSVALUE64)

    inline TiValue::TiValue(TiExcState*, char i)
    {
        ASSERT(JSImmediate::from(i));
        *this = JSImmediate::from(i);
    }

    inline TiValue::TiValue(TiExcState*, unsigned char i)
    {
        ASSERT(JSImmediate::from(i));
        *this = JSImmediate::from(i);
    }

    inline TiValue::TiValue(TiExcState*, short i)
    {
        ASSERT(JSImmediate::from(i));
        *this = JSImmediate::from(i);
    }

    inline TiValue::TiValue(TiExcState*, unsigned short i)
    {
        ASSERT(JSImmediate::from(i));
        *this = JSImmediate::from(i);
    }

    inline TiValue jsNaN(TiExcState* exec)
    {
        return jsNumber(exec, NaN);
    }

    inline TiValue jsNaN(TiGlobalData* globalData)
    {
        return jsNumber(globalData, NaN);
    }

    // --- TiValue inlines ----------------------------

    ALWAYS_INLINE TiValue TiValue::toJSNumber(TiExcState* exec) const
    {
        return isNumber() ? asValue() : jsNumber(exec, this->toNumber(exec));
    }

    inline bool TiValue::getNumber(double &result) const
    {
        if (isInt32())
            result = asInt32();
        else if (LIKELY(isDouble()))
            result = asDouble();
        else {
            ASSERT(!isNumber());
            return false;
        }
        return true;
    }

#endif // USE(JSVALUE32) || USE(JSVALUE64)

} // namespace TI

#endif // JSNumberCell_h
