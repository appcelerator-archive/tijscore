/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef TiCell_h
#define TiCell_h

#include "Collector.h"
#include "JSImmediate.h"
#include "TiValue.h"
#include "MarkStack.h"
#include "Structure.h"
#include <wtf/Noncopyable.h>

namespace TI {

    class TiCell : public NoncopyableCustomAllocated {
        friend class GetterSetter;
        friend class Heap;
        friend class JIT;
        friend class JSNumberCell;
        friend class TiObject;
        friend class TiPropertyNameIterator;
        friend class TiString;
        friend class TiValue;
        friend class TiAPIValueWrapper;
        friend class JSZombie;
        friend class TiGlobalData;

    private:
        explicit TiCell(Structure*);
        virtual ~TiCell();

    public:
        static PassRefPtr<Structure> createDummyStructure()
        {
            return Structure::create(jsNull(), TypeInfo(UnspecifiedType), AnonymousSlotCount);
        }

        // Querying the type.
#if USE(JSVALUE32)
        bool isNumber() const;
#endif
        bool isString() const;
        bool isObject() const;
        virtual bool isGetterSetter() const;
        bool inherits(const ClassInfo*) const;
        virtual bool isAPIValueWrapper() const { return false; }
        virtual bool isPropertyNameIterator() const { return false; }

        Structure* structure() const;

        // Extracting the value.
        bool getString(TiExcState* exec, UString&) const;
        UString getString(TiExcState* exec) const; // null string if not a string
        TiObject* getObject(); // NULL if not an object
        const TiObject* getObject() const; // NULL if not an object
        
        virtual CallType getCallData(CallData&);
        virtual ConstructType getConstructData(ConstructData&);

        // Extracting integer values.
        // FIXME: remove these methods, can check isNumberCell in TiValue && then call asNumberCell::*.
        virtual bool getUInt32(uint32_t&) const;

        // Basic conversions.
        virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(TiExcState*, double& number, TiValue&);
        virtual bool toBoolean(TiExcState*) const;
        virtual double toNumber(TiExcState*) const;
        virtual UString toString(TiExcState*) const;
        virtual TiObject* toObject(TiExcState*) const;

        // Garbage collection.
        void* operator new(size_t, TiExcState*);
        void* operator new(size_t, TiGlobalData*);
        void* operator new(size_t, void* placementNewDestination) { return placementNewDestination; }

        virtual void markChildren(MarkStack&);
#if ENABLE(JSC_ZOMBIES)
        virtual bool isZombie() const { return false; }
#endif

        // Object operations, with the toObject operation included.
        virtual const ClassInfo* classInfo() const;
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual void put(TiExcState*, unsigned propertyName, TiValue);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual bool deleteProperty(TiExcState*, unsigned propertyName);

        virtual TiObject* toThisObject(TiExcState*) const;
        virtual TiValue getJSNumber();
        void* vptr() { return *reinterpret_cast<void**>(this); }
        void setVPtr(void* vptr) { *reinterpret_cast<void**>(this) = vptr; }

        // FIXME: Rename getOwnPropertySlot to virtualGetOwnPropertySlot, and
        // fastGetOwnPropertySlot to getOwnPropertySlot. Callers should always
        // call this function, not its slower virtual counterpart. (For integer
        // property names, we want a similar interface with appropriate optimizations.)
        bool fastGetOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);

    protected:
        static const unsigned AnonymousSlotCount = 0;

    private:
        // Base implementation; for non-object classes implements getPropertySlot.
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        
        Structure* m_structure;
    };

    inline TiCell::TiCell(Structure* structure)
        : m_structure(structure)
    {
    }

    inline TiCell::~TiCell()
    {
    }

#if USE(JSVALUE32)
    inline bool TiCell::isNumber() const
    {
        return m_structure->typeInfo().type() == NumberType;
    }
#endif

    inline bool TiCell::isObject() const
    {
        return m_structure->typeInfo().type() == ObjectType;
    }

    inline bool TiCell::isString() const
    {
        return m_structure->typeInfo().type() == StringType;
    }

    inline Structure* TiCell::structure() const
    {
        return m_structure;
    }

    inline void TiCell::markChildren(MarkStack&)
    {
    }

    inline void* TiCell::operator new(size_t size, TiGlobalData* globalData)
    {
        return globalData->heap.allocate(size);
    }

    inline void* TiCell::operator new(size_t size, TiExcState* exec)
    {
        return exec->heap()->allocate(size);
    }

    // --- TiValue inlines ----------------------------

    inline bool TiValue::isString() const
    {
        return isCell() && asCell()->isString();
    }

    inline bool TiValue::isGetterSetter() const
    {
        return isCell() && asCell()->isGetterSetter();
    }

    inline bool TiValue::isObject() const
    {
        return isCell() && asCell()->isObject();
    }

    inline bool TiValue::getString(TiExcState* exec, UString& s) const
    {
        return isCell() && asCell()->getString(exec, s);
    }

    inline UString TiValue::getString(TiExcState* exec) const
    {
        return isCell() ? asCell()->getString(exec) : UString();
    }

    inline TiObject* TiValue::getObject() const
    {
        return isCell() ? asCell()->getObject() : 0;
    }

    inline CallType TiValue::getCallData(CallData& callData)
    {
        return isCell() ? asCell()->getCallData(callData) : CallTypeNone;
    }

    inline ConstructType TiValue::getConstructData(ConstructData& constructData)
    {
        return isCell() ? asCell()->getConstructData(constructData) : ConstructTypeNone;
    }

    ALWAYS_INLINE bool TiValue::getUInt32(uint32_t& v) const
    {
        if (isInt32()) {
            int32_t i = asInt32();
            v = static_cast<uint32_t>(i);
            return i >= 0;
        }
        if (isDouble()) {
            double d = asDouble();
            v = static_cast<uint32_t>(d);
            return v == d;
        }
        return false;
    }

#if !USE(JSVALUE32_64)
    ALWAYS_INLINE TiCell* TiValue::asCell() const
    {
        ASSERT(isCell());
        return m_ptr;
    }
#endif // !USE(JSVALUE32_64)

    inline TiValue TiValue::toPrimitive(TiExcState* exec, PreferredPrimitiveType preferredType) const
    {
        return isCell() ? asCell()->toPrimitive(exec, preferredType) : asValue();
    }

    inline bool TiValue::getPrimitiveNumber(TiExcState* exec, double& number, TiValue& value)
    {
        if (isInt32()) {
            number = asInt32();
            value = *this;
            return true;
        }
        if (isDouble()) {
            number = asDouble();
            value = *this;
            return true;
        }
        if (isCell())
            return asCell()->getPrimitiveNumber(exec, number, value);
        if (isTrue()) {
            number = 1.0;
            value = *this;
            return true;
        }
        if (isFalse() || isNull()) {
            number = 0.0;
            value = *this;
            return true;
        }
        ASSERT(isUndefined());
        number = nonInlineNaN();
        value = *this;
        return true;
    }

    inline bool TiValue::toBoolean(TiExcState* exec) const
    {
        if (isInt32())
            return asInt32() != 0;
        if (isDouble())
            return asDouble() > 0.0 || asDouble() < 0.0; // false for NaN
        if (isCell())
            return asCell()->toBoolean(exec);
        return isTrue(); // false, null, and undefined all convert to false.
    }

    ALWAYS_INLINE double TiValue::toNumber(TiExcState* exec) const
    {
        if (isInt32())
            return asInt32();
        if (isDouble())
            return asDouble();
        if (isCell())
            return asCell()->toNumber(exec);
        if (isTrue())
            return 1.0;
        return isUndefined() ? nonInlineNaN() : 0; // null and false both convert to 0.
    }

    inline bool TiValue::needsThisConversion() const
    {
        if (UNLIKELY(!isCell()))
            return true;
        return asCell()->structure()->typeInfo().needsThisConversion();
    }

    inline TiValue TiValue::getJSNumber()
    {
        if (isInt32() || isDouble())
            return *this;
        if (isCell())
            return asCell()->getJSNumber();
        return TiValue();
    }

    inline TiObject* TiValue::toObject(TiExcState* exec) const
    {
        return isCell() ? asCell()->toObject(exec) : toObjectSlowCase(exec);
    }

    inline TiObject* TiValue::toThisObject(TiExcState* exec) const
    {
        return isCell() ? asCell()->toThisObject(exec) : toThisObjectSlowCase(exec);
    }

    ALWAYS_INLINE void MarkStack::append(TiCell* cell)
    {
        ASSERT(!m_isCheckingForDefaultMarkViolation);
        ASSERT(cell);
        if (Heap::isCellMarked(cell))
            return;
        Heap::markCell(cell);
        if (cell->structure()->typeInfo().type() >= CompoundType)
            m_values.append(cell);
    }

    ALWAYS_INLINE void MarkStack::append(TiValue value)
    {
        ASSERT(value);
        if (value.isCell())
            append(value.asCell());
    }

    inline Heap* Heap::heap(TiValue v)
    {
        if (!v.isCell())
            return 0;
        return heap(v.asCell());
    }

    inline Heap* Heap::heap(TiCell* c)
    {
        return cellBlock(c)->heap;
    }
    
#if ENABLE(JSC_ZOMBIES)
    inline bool TiValue::isZombie() const
    {
        return isCell() && asCell() && asCell()->isZombie();
    }
#endif
} // namespace TI

#endif // TiCell_h
