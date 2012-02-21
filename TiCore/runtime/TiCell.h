/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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

#include "CallData.h"
#include "CallFrame.h"
#include "ConstructData.h"
#include "Heap.h"
#include "TiLock.h"
#include "TiValueInlineMethods.h"
#include "MarkStack.h"
#include "WriteBarrier.h"
#include <wtf/Noncopyable.h>

namespace TI {

    class TiGlobalObject;
    class Structure;

#if COMPILER(MSVC)
    // If WTF_MAKE_NONCOPYABLE is applied to TiCell we end up with a bunch of
    // undefined references to the TiCell copy constructor and assignment operator
    // when linking TiCore.
    class MSVCBugWorkaround {
        WTF_MAKE_NONCOPYABLE(MSVCBugWorkaround);

    protected:
        MSVCBugWorkaround() { }
        ~MSVCBugWorkaround() { }
    };

    class TiCell : MSVCBugWorkaround {
#else
    class TiCell {
        WTF_MAKE_NONCOPYABLE(TiCell);
#endif

        friend class ExecutableBase;
        friend class GetterSetter;
        friend class Heap;
        friend class TiObject;
        friend class TiPropertyNameIterator;
        friend class TiString;
        friend class TiValue;
        friend class TiAPIValueWrapper;
        friend class JSZombie;
        friend class TiGlobalData;
        friend class MarkedSpace;
        friend class MarkedBlock;
        friend class ScopeChainNode;
        friend class Structure;
        friend class StructureChain;
        friend class RegExp;
        enum CreatingEarlyCellTag { CreatingEarlyCell };

    protected:
        enum VPtrStealingHackType { VPtrStealingHack };

    private:
        explicit TiCell(VPtrStealingHackType) { }
        TiCell(TiGlobalData&, Structure*);
        TiCell(TiGlobalData&, Structure*, CreatingEarlyCellTag);
        virtual ~TiCell();
        static const ClassInfo s_dummyCellInfo;

    public:
        static Structure* createDummyStructure(TiGlobalData&);

        // Querying the type.
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
        virtual TiObject* toObject(TiExcState*, TiGlobalObject*) const;

        // Garbage collection.
        void* operator new(size_t, TiExcState*);
        void* operator new(size_t, TiGlobalData*);
        void* operator new(size_t, void* placementNewDestination) { return placementNewDestination; }

        virtual void visitChildren(SlotVisitor&);
#if ENABLE(JSC_ZOMBIES)
        virtual bool isZombie() const { return false; }
#endif

        // Object operations, with the toObject operation included.
        const ClassInfo* classInfo() const;
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

        static ptrdiff_t structureOffset()
        {
            return OBJECT_OFFSETOF(TiCell, m_structure);
        }

#if ENABLE(GC_VALIDATION)
        Structure* unvalidatedStructure() { return m_structure.unvalidatedGet(); }
#endif
        
    protected:
        static const unsigned AnonymousSlotCount = 0;

    private:
        // Base implementation; for non-object classes implements getPropertySlot.
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        
        WriteBarrier<Structure> m_structure;
    };

    inline TiCell::TiCell(TiGlobalData& globalData, Structure* structure)
        : m_structure(globalData, this, structure)
    {
        ASSERT(m_structure);
    }

    inline TiCell::TiCell(TiGlobalData& globalData, Structure* structure, CreatingEarlyCellTag)
    {
#if ENABLE(GC_VALIDATION)
        if (structure)
#endif
            m_structure.setEarlyValue(globalData, this, structure);
        // Very first set of allocations won't have a real structure.
        ASSERT(m_structure || !globalData.dummyMarkableCellStructure);
    }

    inline TiCell::~TiCell()
    {
#if ENABLE(GC_VALIDATION)
        m_structure.clear();
#endif
    }

    inline Structure* TiCell::structure() const
    {
        return m_structure.get();
    }

    inline void TiCell::visitChildren(SlotVisitor& visitor)
    {
        visitor.append(&m_structure);
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

    template <typename Base> UString HandleConverter<Base, Unknown>::getString(TiExcState* exec) const
    {
        return jsValue().getString(exec);
    }

    inline TiObject* TiValue::getObject() const
    {
        return isCell() ? asCell()->getObject() : 0;
    }

    inline CallType getCallData(TiValue value, CallData& callData)
    {
        CallType result = value.isCell() ? value.asCell()->getCallData(callData) : CallTypeNone;
        ASSERT(result == CallTypeNone || value.isValidCallee());
        return result;
    }

    inline ConstructType getConstructData(TiValue value, ConstructData& constructData)
    {
        ConstructType result = value.isCell() ? value.asCell()->getConstructData(constructData) : ConstructTypeNone;
        ASSERT(result == ConstructTypeNone || value.isValidCallee());
        return result;
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
        return isCell() ? asCell()->toObject(exec, exec->lexicalGlobalObject()) : toObjectSlowCase(exec, exec->lexicalGlobalObject());
    }

    inline TiObject* TiValue::toObject(TiExcState* exec, TiGlobalObject* globalObject) const
    {
        return isCell() ? asCell()->toObject(exec, globalObject) : toObjectSlowCase(exec, globalObject);
    }

    inline TiObject* TiValue::toThisObject(TiExcState* exec) const
    {
        return isCell() ? asCell()->toThisObject(exec) : toThisObjectSlowCase(exec);
    }

    inline Heap* Heap::heap(TiValue v)
    {
        if (!v.isCell())
            return 0;
        return heap(v.asCell());
    }

    inline Heap* Heap::heap(TiCell* c)
    {
        return MarkedSpace::heap(c);
    }
    
#if ENABLE(JSC_ZOMBIES)
    inline bool TiValue::isZombie() const
    {
        return isCell() && asCell() > (TiCell*)0x1ffffffffL && asCell()->isZombie();
    }
#endif

    inline void* MarkedBlock::allocate()
    {
        while (m_nextAtom < m_endAtom) {
            if (!m_marks.testAndSet(m_nextAtom)) {
                TiCell* cell = reinterpret_cast<TiCell*>(&atoms()[m_nextAtom]);
                m_nextAtom += m_atomsPerCell;
                cell->~TiCell();
                return cell;
            }
            m_nextAtom += m_atomsPerCell;
        }

        return 0;
    }
    
    inline MarkedSpace::SizeClass& MarkedSpace::sizeClassFor(size_t bytes)
    {
        ASSERT(bytes && bytes < maxCellSize);
        if (bytes < preciseCutoff)
            return m_preciseSizeClasses[(bytes - 1) / preciseStep];
        return m_impreciseSizeClasses[(bytes - 1) / impreciseStep];
    }

    inline void* MarkedSpace::allocate(size_t bytes)
    {
        SizeClass& sizeClass = sizeClassFor(bytes);
        return allocateFromSizeClass(sizeClass);
    }
    
    inline void* Heap::allocate(size_t bytes)
    {
        ASSERT(globalData()->identifierTable == wtfThreadData().currentIdentifierTable());
        ASSERT(TiLock::lockCount() > 0);
        ASSERT(TiLock::currentThreadIsHoldingLock());
        ASSERT(bytes <= MarkedSpace::maxCellSize);
        ASSERT(m_operationInProgress == NoOperation);

        m_operationInProgress = Allocation;
        void* result = m_markedSpace.allocate(bytes);
        m_operationInProgress = NoOperation;
        if (result)
            return result;

        return allocateSlowCase(bytes);
    }

    inline void* TiCell::operator new(size_t size, TiGlobalData* globalData)
    {
        TiCell* result = static_cast<TiCell*>(globalData->heap.allocate(size));
        result->m_structure.clear();
        return result;
    }

    inline void* TiCell::operator new(size_t size, TiExcState* exec)
    {
        TiCell* result = static_cast<TiCell*>(exec->heap()->allocate(size));
        result->m_structure.clear();
        return result;
    }

} // namespace TI

#endif // TiCell_h
