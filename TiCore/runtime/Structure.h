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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef Structure_h
#define Structure_h

#include "Identifier.h"
#include "TiType.h"
#include "TiValue.h"
#include "PropertyMapHashTable.h"
#include "PropertyNameArray.h"
#include "Protect.h"
#include "StructureChain.h"
#include "StructureTransitionTable.h"
#include "TiTypeInfo.h"
#include "UString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#ifndef NDEBUG
#define DUMP_PROPERTYMAP_STATS 0
#else
#define DUMP_PROPERTYMAP_STATS 0
#endif

namespace TI {

    class MarkStack;
    class PropertyNameArray;
    class PropertyNameArrayData;

    class Structure : public RefCounted<Structure> {
    public:
        friend class JIT;
        friend class StructureTransitionTable;
        static PassRefPtr<Structure> create(TiValue prototype, const TypeInfo& typeInfo)
        {
            return adoptRef(new Structure(prototype, typeInfo));
        }

        static void startIgnoringLeaks();
        static void stopIgnoringLeaks();

        static void dumpStatistics();

        static PassRefPtr<Structure> addPropertyTransition(Structure*, const Identifier& propertyName, unsigned attributes, TiCell* specificValue, size_t& offset);
        static PassRefPtr<Structure> addPropertyTransitionToExistingStructure(Structure*, const Identifier& propertyName, unsigned attributes, TiCell* specificValue, size_t& offset);
        static PassRefPtr<Structure> removePropertyTransition(Structure*, const Identifier& propertyName, size_t& offset);
        static PassRefPtr<Structure> changePrototypeTransition(Structure*, TiValue prototype);
        static PassRefPtr<Structure> despecifyFunctionTransition(Structure*, const Identifier&);
        static PassRefPtr<Structure> addAnonymousSlotsTransition(Structure*, unsigned count);
        static PassRefPtr<Structure> getterSetterTransition(Structure*);
        static PassRefPtr<Structure> toCacheableDictionaryTransition(Structure*);
        static PassRefPtr<Structure> toUncacheableDictionaryTransition(Structure*);

        PassRefPtr<Structure> flattenDictionaryStructure(TiObject*);

        ~Structure();

        // These should be used with caution.  
        size_t addPropertyWithoutTransition(const Identifier& propertyName, unsigned attributes, TiCell* specificValue);
        size_t removePropertyWithoutTransition(const Identifier& propertyName);
        void setPrototypeWithoutTransition(TiValue prototype) { m_prototype = prototype; }
        
        bool isDictionary() const { return m_dictionaryKind != NoneDictionaryKind; }
        bool isUncacheableDictionary() const { return m_dictionaryKind == UncachedDictionaryKind; }

        const TypeInfo& typeInfo() const { return m_typeInfo; }

        TiValue storedPrototype() const { return m_prototype; }
        TiValue prototypeForLookup(TiExcState*) const;
        StructureChain* prototypeChain(TiExcState*) const;

        Structure* previousID() const { return m_previous.get(); }

        void growPropertyStorageCapacity();
        unsigned propertyStorageCapacity() const { return m_propertyStorageCapacity; }
        unsigned propertyStorageSize() const { return m_propertyTable ? m_propertyTable->keyCount + m_propertyTable->anonymousSlotCount + (m_propertyTable->deletedOffsets ? m_propertyTable->deletedOffsets->size() : 0) : m_offset + 1; }
        bool isUsingInlineStorage() const;

        size_t get(const Identifier& propertyName);
        size_t get(const UString::Rep* rep, unsigned& attributes, TiCell*& specificValue);
        size_t get(const Identifier& propertyName, unsigned& attributes, TiCell*& specificValue)
        {
            ASSERT(!propertyName.isNull());
            return get(propertyName.ustring().rep(), attributes, specificValue);
        }
        bool transitionedFor(const TiCell* specificValue)
        {
            return m_specificValueInPrevious == specificValue;
        }
        bool hasTransition(UString::Rep*, unsigned attributes);
        bool hasTransition(const Identifier& propertyName, unsigned attributes)
        {
            return hasTransition(propertyName._ustring.rep(), attributes);
        }

        bool hasGetterSetterProperties() const { return m_hasGetterSetterProperties; }
        void setHasGetterSetterProperties(bool hasGetterSetterProperties) { m_hasGetterSetterProperties = hasGetterSetterProperties; }

        bool hasNonEnumerableProperties() const { return m_hasNonEnumerableProperties; }

        bool hasAnonymousSlots() const { return m_propertyTable && m_propertyTable->anonymousSlotCount; }
        
        bool isEmpty() const { return m_propertyTable ? !m_propertyTable->keyCount : m_offset == noOffset; }

        TiCell* specificValue() { return m_specificValueInPrevious; }
        void despecifyDictionaryFunction(const Identifier& propertyName);

        void setEnumerationCache(TiPropertyNameIterator* enumerationCache); // Defined in TiPropertyNameIterator.h.
        TiPropertyNameIterator* enumerationCache() { return m_enumerationCache.get(); }
        void getEnumerablePropertyNames(PropertyNameArray&);

    private:
        Structure(TiValue prototype, const TypeInfo&);
        
        typedef enum { 
            NoneDictionaryKind = 0,
            CachedDictionaryKind = 1,
            UncachedDictionaryKind = 2
        } DictionaryKind;
        static PassRefPtr<Structure> toDictionaryTransition(Structure*, DictionaryKind);

        size_t put(const Identifier& propertyName, unsigned attributes, TiCell* specificValue);
        size_t remove(const Identifier& propertyName);
        void addAnonymousSlots(unsigned slotCount);

        void expandPropertyMapHashTable();
        void rehashPropertyMapHashTable();
        void rehashPropertyMapHashTable(unsigned newTableSize);
        void createPropertyMapHashTable();
        void createPropertyMapHashTable(unsigned newTableSize);
        void insertIntoPropertyMapHashTable(const PropertyMapEntry&);
        void checkConsistency();

        bool despecifyFunction(const Identifier&);

        PropertyMapHashTable* copyPropertyTable();
        void materializePropertyMap();
        void materializePropertyMapIfNecessary()
        {
            if (m_propertyTable || !m_previous)             
                return;
            materializePropertyMap();
        }

        signed char transitionCount() const
        {
            // Since the number of transitions is always the same as m_offset, we keep the size of Structure down by not storing both.
            return m_offset == noOffset ? 0 : m_offset + 1;
        }
        
        bool isValid(TiExcState*, StructureChain* cachedPrototypeChain) const;

        static const unsigned emptyEntryIndex = 0;
    
        static const signed char s_maxTransitionLength = 64;

        static const signed char noOffset = -1;

        TypeInfo m_typeInfo;

        TiValue m_prototype;
        mutable RefPtr<StructureChain> m_cachedPrototypeChain;

        RefPtr<Structure> m_previous;
        RefPtr<UString::Rep> m_nameInPrevious;
        TiCell* m_specificValueInPrevious;

        StructureTransitionTable table;

        ProtectedPtr<TiPropertyNameIterator> m_enumerationCache;

        PropertyMapHashTable* m_propertyTable;

        uint32_t m_propertyStorageCapacity;
        signed char m_offset;

        unsigned m_dictionaryKind : 2;
        bool m_isPinnedPropertyTable : 1;
        bool m_hasGetterSetterProperties : 1;
        bool m_hasNonEnumerableProperties : 1;
#if COMPILER(WINSCW)
        // Workaround for Symbian WINSCW compiler that cannot resolve unsigned type of the declared 
        // bitfield, when used as argument in make_pair() function calls in structure.ccp.
        // This bitfield optimization is insignificant for the Symbian emulator target.
        unsigned m_attributesInPrevious;
#else
        unsigned m_attributesInPrevious : 7;
#endif
        unsigned m_anonymousSlotsInPrevious : 6;
    };

    inline size_t Structure::get(const Identifier& propertyName)
    {
        ASSERT(!propertyName.isNull());

        materializePropertyMapIfNecessary();
        if (!m_propertyTable)
            return WTI::notFound;

        UString::Rep* rep = propertyName._ustring.rep();

        unsigned i = rep->computedHash();

#if DUMP_PROPERTYMAP_STATS
        ++numProbes;
#endif

        unsigned entryIndex = m_propertyTable->entryIndices[i & m_propertyTable->sizeMask];
        if (entryIndex == emptyEntryIndex)
            return WTI::notFound;

        if (rep == m_propertyTable->entries()[entryIndex - 1].key)
            return m_propertyTable->entries()[entryIndex - 1].offset;

#if DUMP_PROPERTYMAP_STATS
        ++numCollisions;
#endif

        unsigned k = 1 | WTI::doubleHash(rep->computedHash());

        while (1) {
            i += k;

#if DUMP_PROPERTYMAP_STATS
            ++numRehashes;
#endif

            entryIndex = m_propertyTable->entryIndices[i & m_propertyTable->sizeMask];
            if (entryIndex == emptyEntryIndex)
                return WTI::notFound;

            if (rep == m_propertyTable->entries()[entryIndex - 1].key)
                return m_propertyTable->entries()[entryIndex - 1].offset;
        }
    }
    
    bool StructureTransitionTable::contains(const StructureTransitionTableHash::Key& key, TiCell* specificValue)
    {
        if (usingSingleTransitionSlot()) {
            Structure* existingTransition = singleTransition();
            return existingTransition && existingTransition->m_nameInPrevious.get() == key.first
                   && existingTransition->m_attributesInPrevious == key.second
                   && (existingTransition->m_specificValueInPrevious == specificValue || existingTransition->m_specificValueInPrevious == 0);
        }
        TransitionTable::iterator find = table()->find(key);
        if (find == table()->end())
            return false;

        return find->second.first || find->second.second->transitionedFor(specificValue);
    }

    Structure* StructureTransitionTable::get(const StructureTransitionTableHash::Key& key, TiCell* specificValue) const
    {
        if (usingSingleTransitionSlot()) {
            Structure* existingTransition = singleTransition();
            if (existingTransition && existingTransition->m_nameInPrevious.get() == key.first
                && existingTransition->m_attributesInPrevious == key.second
                && (existingTransition->m_specificValueInPrevious == specificValue || existingTransition->m_specificValueInPrevious == 0))
                return existingTransition;
            return 0;
        }

        Transition transition = table()->get(key);
        if (transition.second && transition.second->transitionedFor(specificValue))
            return transition.second;
        return transition.first;
    }

    bool StructureTransitionTable::hasTransition(const StructureTransitionTableHash::Key& key) const
    {
        if (usingSingleTransitionSlot()) {
            Structure* transition = singleTransition();
            return transition && transition->m_nameInPrevious == key.first
            && transition->m_attributesInPrevious == key.second;
        }
        return table()->contains(key);
    }
    
    void StructureTransitionTable::reifySingleTransition()
    {
        ASSERT(usingSingleTransitionSlot());
        Structure* existingTransition = singleTransition();
        TransitionTable* transitionTable = new TransitionTable;
        setTransitionTable(transitionTable);
        if (existingTransition)
            add(make_pair(existingTransition->m_nameInPrevious.get(), existingTransition->m_attributesInPrevious), existingTransition, existingTransition->m_specificValueInPrevious);
    }
} // namespace TI

#endif // Structure_h
