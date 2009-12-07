/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef TiArray_h
#define TiArray_h

#include "TiObject.h"

namespace TI {

    typedef HashMap<unsigned, TiValue> SparseArrayValueMap;

    struct ArrayStorage {
        unsigned m_length;
        unsigned m_numValuesInVector;
        SparseArrayValueMap* m_sparseValueMap;
        void* lazyCreationData; // A TiArray subclass can use this to fill the vector lazily.
        TiValue m_vector[1];
    };

    class TiArray : public TiObject {
        friend class JIT;
        friend class Walker;

    public:
        explicit TiArray(NonNullPassRefPtr<Structure>);
        TiArray(NonNullPassRefPtr<Structure>, unsigned initialLength);
        TiArray(NonNullPassRefPtr<Structure>, const ArgList& initialValues);
        virtual ~TiArray();

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void put(TiExcState*, unsigned propertyName, TiValue); // FIXME: Make protected and add setItem.

        static JS_EXPORTDATA const ClassInfo info;

        unsigned length() const { return m_storage->m_length; }
        void setLength(unsigned); // OK to use on new arrays, but not if it might be a RegExpMatchArray.

        void sort(TiExcState*);
        void sort(TiExcState*, TiValue compareFunction, CallType, const CallData&);
        void sortNumeric(TiExcState*, TiValue compareFunction, CallType, const CallData&);

        void push(TiExcState*, TiValue);
        TiValue pop();

        bool canGetIndex(unsigned i) { return i < m_vectorLength && m_storage->m_vector[i]; }
        TiValue getIndex(unsigned i)
        {
            ASSERT(canGetIndex(i));
            return m_storage->m_vector[i];
        }

        bool canSetIndex(unsigned i) { return i < m_vectorLength; }
        void setIndex(unsigned i, TiValue v)
        {
            ASSERT(canSetIndex(i));
            TiValue& x = m_storage->m_vector[i];
            if (!x) {
                ++m_storage->m_numValuesInVector;
                if (i >= m_storage->m_length)
                    m_storage->m_length = i + 1;
            }
            x = v;
        }

        void fillArgList(TiExcState*, MarkedArgumentBuffer&);
        void copyToRegisters(TiExcState*, Register*, uint32_t);

        static PassRefPtr<Structure> createStructure(TiValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags));
        }
        
        inline void markChildrenDirect(MarkStack& markStack);

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesMarkChildren | OverridesGetPropertyNames | TiObject::StructureFlags;
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual bool deleteProperty(TiExcState*, unsigned propertyName);
        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&);
        virtual void markChildren(MarkStack&);

        void* lazyCreationData();
        void setLazyCreationData(void*);

    private:
        virtual const ClassInfo* classInfo() const { return &info; }

        bool getOwnPropertySlotSlowCase(TiExcState*, unsigned propertyName, PropertySlot&);
        void putSlowCase(TiExcState*, unsigned propertyName, TiValue);

        bool increaseVectorLength(unsigned newLength);
        
        unsigned compactForSorting();

        enum ConsistencyCheckType { NormalConsistencyCheck, DestructorConsistencyCheck, SortConsistencyCheck };
        void checkConsistency(ConsistencyCheckType = NormalConsistencyCheck);

        unsigned m_vectorLength;
        ArrayStorage* m_storage;
    };

    TiArray* asArray(TiValue);

    inline TiArray* asArray(TiCell* cell)
    {
        ASSERT(cell->inherits(&TiArray::info));
        return static_cast<TiArray*>(cell);
    }

    inline TiArray* asArray(TiValue value)
    {
        return asArray(value.asCell());
    }

    inline bool isTiArray(TiGlobalData* globalData, TiValue v)
    {
        return v.isCell() && v.asCell()->vptr() == globalData->jsArrayVPtr;
    }
    inline bool isTiArray(TiGlobalData* globalData, TiCell* cell) { return cell->vptr() == globalData->jsArrayVPtr; }

    inline void TiArray::markChildrenDirect(MarkStack& markStack)
    {
        TiObject::markChildrenDirect(markStack);
        
        ArrayStorage* storage = m_storage;

        unsigned usedVectorLength = std::min(storage->m_length, m_vectorLength);
        markStack.appendValues(storage->m_vector, usedVectorLength, MayContainNullValues);

        if (SparseArrayValueMap* map = storage->m_sparseValueMap) {
            SparseArrayValueMap::iterator end = map->end();
            for (SparseArrayValueMap::iterator it = map->begin(); it != end; ++it)
                markStack.append(it->second);
        }
    }

    inline void MarkStack::markChildren(TiCell* cell)
    {
        ASSERT(Heap::isCellMarked(cell));
        if (!cell->structure()->typeInfo().overridesMarkChildren()) {
#ifdef NDEBUG
            asObject(cell)->markChildrenDirect(*this);
#else
            ASSERT(!m_isCheckingForDefaultMarkViolation);
            m_isCheckingForDefaultMarkViolation = true;
            cell->markChildren(*this);
            ASSERT(m_isCheckingForDefaultMarkViolation);
            m_isCheckingForDefaultMarkViolation = false;
#endif
            return;
        }
        if (cell->vptr() == m_jsArrayVPtr) {
            asArray(cell)->markChildrenDirect(*this);
            return;
        }
        cell->markChildren(*this);
    }

    inline void MarkStack::drain()
    {
        while (!m_markSets.isEmpty() || !m_values.isEmpty()) {
            while (!m_markSets.isEmpty() && m_values.size() < 50) {
                ASSERT(!m_markSets.isEmpty());
                MarkSet& current = m_markSets.last();
                ASSERT(current.m_values);
                TiValue* end = current.m_end;
                ASSERT(current.m_values);
                ASSERT(current.m_values != end);
            findNextUnmarkedNullValue:
                ASSERT(current.m_values != end);
                TiValue value = *current.m_values;
                current.m_values++;

                TiCell* cell;
                if (!value || !value.isCell() || Heap::isCellMarked(cell = value.asCell())) {
                    if (current.m_values == end) {
                        m_markSets.removeLast();
                        continue;
                    }
                    goto findNextUnmarkedNullValue;
                }

                Heap::markCell(cell);
                if (cell->structure()->typeInfo().type() < CompoundType) {
                    if (current.m_values == end) {
                        m_markSets.removeLast();
                        continue;
                    }
                    goto findNextUnmarkedNullValue;
                }

                if (current.m_values == end)
                    m_markSets.removeLast();

                markChildren(cell);
            }
            while (!m_values.isEmpty())
                markChildren(m_values.removeLast());
        }
    }
    
} // namespace TI

#endif // TiArray_h
