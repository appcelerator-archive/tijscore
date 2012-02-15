/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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
#include "TiCell.h"
#include "TiType.h"
#include "TiValue.h"
#include "PropertyMapHashTable.h"
#include "PropertyNameArray.h"
#include "Protect.h"
#include "StructureTransitionTable.h"
#include "TiTypeInfo.h"
#include "UString.h"
#include "Weak.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>


namespace TI {

    class MarkStack;
    class PropertyNameArray;
    class PropertyNameArrayData;
    class StructureChain;
    typedef MarkStack SlotVisitor;

    struct ClassInfo;

    enum EnumerationMode {
        ExcludeDontEnumProperties,
        IncludeDontEnumProperties
    };

    class Structure : public TiCell {
    public:
        friend class StructureTransitionTable;
        static Structure* create(TiGlobalData& globalData, TiValue prototype, const TypeInfo& typeInfo, unsigned anonymousSlotCount, const ClassInfo* classInfo)
        {
            ASSERT(globalData.structureStructure);
            ASSERT(classInfo);
            return new (&globalData) Structure(globalData, prototype, typeInfo, anonymousSlotCount, classInfo);
        }

        static void dumpStatistics();

        static Structure* addPropertyTransition(TiGlobalData&, Structure*, const Identifier& propertyName, unsigned attributes, TiCell* specificValue, size_t& offset);
        static Structure* addPropertyTransitionToExistingStructure(Structure*, const Identifier& propertyName, unsigned attributes, TiCell* specificValue, size_t& offset);
        static Structure* removePropertyTransition(TiGlobalData&, Structure*, const Identifier& propertyName, size_t& offset);
        static Structure* changePrototypeTransition(TiGlobalData&, Structure*, TiValue prototype);
        static Structure* despecifyFunctionTransition(TiGlobalData&, Structure*, const Identifier&);
        static Structure* getterSetterTransition(TiGlobalData&, Structure*);
        static Structure* toCacheableDictionaryTransition(TiGlobalData&, Structure*);
        static Structure* toUncacheableDictionaryTransition(TiGlobalData&, Structure*);
        static Structure* sealTransition(TiGlobalData&, Structure*);
        static Structure* freezeTransition(TiGlobalData&, Structure*);
        static Structure* preventExtensionsTransition(TiGlobalData&, Structure*);

        bool isSealed(TiGlobalData&);
        bool isFrozen(TiGlobalData&);
        bool isExtensible() const { return !m_preventExtensions; }
        bool didTransition() const { return m_didTransition; }

        Structure* flattenDictionaryStructure(TiGlobalData&, TiObject*);

        ~Structure();

        // These should be used with caution.  
        size_t addPropertyWithoutTransition(TiGlobalData&, const Identifier& propertyName, unsigned attributes, TiCell* specificValue);
        size_t removePropertyWithoutTransition(TiGlobalData&, const Identifier& propertyName);
        void setPrototypeWithoutTransition(TiGlobalData& globalData, TiValue prototype) { m_prototype.set(globalData, this, prototype); }
        
        bool isDictionary() const { return m_dictionaryKind != NoneDictionaryKind; }
        bool isUncacheableDictionary() const { return m_dictionaryKind == UncachedDictionaryKind; }

        const TypeInfo& typeInfo() const { ASSERT(structure()->classInfo() == &s_info); return m_typeInfo; }

        TiValue storedPrototype() const { return m_prototype.get(); }
        TiValue prototypeForLookup(TiExcState*) const;
        StructureChain* prototypeChain(TiExcState*) const;
        void visitChildren(SlotVisitor&);

        Structure* previousID() const { ASSERT(structure()->classInfo() == &s_info); return m_previous.get(); }

        void growPropertyStorageCapacity();
        unsigned propertyStorageCapacity() const { ASSERT(structure()->classInfo() == &s_info); return m_propertyStorageCapacity; }
        unsigned propertyStorageSize() const { ASSERT(structure()->classInfo() == &s_info); return m_anonymousSlotCount + (m_propertyTable ? m_propertyTable->propertyStorageSize() : static_cast<unsigned>(m_offset + 1)); }
        bool isUsingInlineStorage() const;

        size_t get(TiGlobalData&, const Identifier& propertyName);
        size_t get(TiGlobalData&, StringImpl* propertyName, unsigned& attributes, TiCell*& specificValue);
        size_t get(TiGlobalData& globalData, const Identifier& propertyName, unsigned& attributes, TiCell*& specificValue)
        {
            ASSERT(!propertyName.isNull());
            ASSERT(structure()->classInfo() == &s_info);
            return get(globalData, propertyName.impl(), attributes, specificValue);
        }

        bool hasGetterSetterProperties() const { return m_hasGetterSetterProperties; }
        void setHasGetterSetterProperties(bool hasGetterSetterProperties) { m_hasGetterSetterProperties = hasGetterSetterProperties; }

        bool hasNonEnumerableProperties() const { return m_hasNonEnumerableProperties; }

        bool hasAnonymousSlots() const { return !!m_anonymousSlotCount; }
        unsigned anonymousSlotCount() const { return m_anonymousSlotCount; }
        
        bool isEmpty() const { return m_propertyTable ? m_propertyTable->isEmpty() : m_offset == noOffset; }

        void despecifyDictionaryFunction(TiGlobalData&, const Identifier& propertyName);
        void disableSpecificFunctionTracking() { m_specificFunctionThrashCount = maxSpecificFunctionThrashCount; }

        void setEnumerationCache(TiGlobalData&, TiPropertyNameIterator* enumerationCache); // Defined in TiPropertyNameIterator.h.
        TiPropertyNameIterator* enumerationCache(); // Defined in TiPropertyNameIterator.h.
        void getPropertyNames(TiGlobalData&, PropertyNameArray&, EnumerationMode mode);

        const ClassInfo* classInfo() const { return m_classInfo; }

        static ptrdiff_t prototypeOffset()
        {
            return OBJECT_OFFSETOF(Structure, m_prototype);
        }

        static ptrdiff_t typeInfoFlagsOffset()
        {
            return OBJECT_OFFSETOF(Structure, m_typeInfo) + TypeInfo::flagsOffset();
        }

        static ptrdiff_t typeInfoTypeOffset()
        {
            return OBJECT_OFFSETOF(Structure, m_typeInfo) + TypeInfo::typeOffset();
        }

        static Structure* createStructure(TiGlobalData& globalData)
        {
            ASSERT(!globalData.structureStructure);
            return new (&globalData) Structure(globalData);
        }
        
        static JS_EXPORTDATA const ClassInfo s_info;

    private:
        Structure(TiGlobalData&, TiValue prototype, const TypeInfo&, unsigned anonymousSlotCount, const ClassInfo*);
        Structure(TiGlobalData&);
        Structure(TiGlobalData&, const Structure*);

        static Structure* create(TiGlobalData& globalData, const Structure* structure)
        {
            ASSERT(globalData.structureStructure);
            return new (&globalData) Structure(globalData, structure);
        }
        
        typedef enum { 
            NoneDictionaryKind = 0,
            CachedDictionaryKind = 1,
            UncachedDictionaryKind = 2
        } DictionaryKind;
        static Structure* toDictionaryTransition(TiGlobalData&, Structure*, DictionaryKind);

        size_t putSpecificValue(TiGlobalData&, const Identifier& propertyName, unsigned attributes, TiCell* specificValue);
        size_t remove(const Identifier& propertyName);

        void createPropertyMap(unsigned keyCount = 0);
        void checkConsistency();

        bool despecifyFunction(TiGlobalData&, const Identifier&);
        void despecifyAllFunctions(TiGlobalData&);

        PassOwnPtr<PropertyTable> copyPropertyTable(TiGlobalData&, Structure* owner);
        void materializePropertyMap(TiGlobalData&);
        void materializePropertyMapIfNecessary(TiGlobalData& globalData)
        {
            ASSERT(structure()->classInfo() == &s_info);
            if (!m_propertyTable && m_previous)
                materializePropertyMap(globalData);
        }

        signed char transitionCount() const
        {
            // Since the number of transitions is always the same as m_offset, we keep the size of Structure down by not storing both.
            return m_offset == noOffset ? 0 : m_offset + 1;
        }

        bool isValid(TiExcState*, StructureChain* cachedPrototypeChain) const;

        static const signed char s_maxTransitionLength = 64;

        static const signed char noOffset = -1;

        static const unsigned maxSpecificFunctionThrashCount = 3;

        TypeInfo m_typeInfo;

        WriteBarrier<Unknown> m_prototype;
        mutable WriteBarrier<StructureChain> m_cachedPrototypeChain;

        WriteBarrier<Structure> m_previous;
        RefPtr<StringImpl> m_nameInPrevious;
        WriteBarrier<TiCell> m_specificValueInPrevious;

        const ClassInfo* m_classInfo;

        StructureTransitionTable m_transitionTable;

        WriteBarrier<TiPropertyNameIterator> m_enumerationCache;

        OwnPtr<PropertyTable> m_propertyTable;

        uint32_t m_propertyStorageCapacity;

        // m_offset does not account for anonymous slots
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
        unsigned m_specificFunctionThrashCount : 2;
        unsigned m_anonymousSlotCount : 5;
        unsigned m_preventExtensions : 1;
        unsigned m_didTransition : 1;
        // 3 free bits
    };

    inline size_t Structure::get(TiGlobalData& globalData, const Identifier& propertyName)
    {
        ASSERT(structure()->classInfo() == &s_info);
        materializePropertyMapIfNecessary(globalData);
        if (!m_propertyTable)
            return notFound;

        PropertyMapEntry* entry = m_propertyTable->find(propertyName.impl()).first;
        ASSERT(!entry || entry->offset >= m_anonymousSlotCount);
        return entry ? entry->offset : notFound;
    }

    inline bool TiCell::isObject() const
    {
        return m_structure->typeInfo().type() == ObjectType;
    }

    inline bool TiCell::isString() const
    {
        return m_structure->typeInfo().type() == StringType;
    }

    inline const ClassInfo* TiCell::classInfo() const
    {
#if ENABLE(GC_VALIDATION)
        return m_structure.unvalidatedGet()->classInfo();
#else
        return m_structure->classInfo();
#endif
    }

    inline Structure* TiCell::createDummyStructure(TiGlobalData& globalData)
    {
        return Structure::create(globalData, jsNull(), TypeInfo(UnspecifiedType), AnonymousSlotCount, &s_dummyCellInfo);
    }

    inline bool TiValue::needsThisConversion() const
    {
        if (UNLIKELY(!isCell()))
            return true;
        return asCell()->structure()->typeInfo().needsThisConversion();
    }

    ALWAYS_INLINE void MarkStack::internalAppend(TiCell* cell)
    {
        ASSERT(!m_isCheckingForDefaultMarkViolation);
        ASSERT(cell);
        if (Heap::testAndSetMarked(cell))
            return;
        if (cell->structure() && cell->structure()->typeInfo().type() >= CompoundType)
            m_values.append(cell);
    }

    inline StructureTransitionTable::Hash::Key StructureTransitionTable::keyForWeakGCMapFinalizer(void*, Structure* structure)
    {
        // Newer versions of the STL have an std::make_pair function that takes rvalue references.
        // When either of the parameters are bitfields, the C++ compiler will try to bind them as lvalues, which is invalid. To work around this, use unary "+" to make the parameter an rvalue.
        // See https://bugs.webkit.org/show_bug.cgi?id=59261 for more details.
        return Hash::Key(structure->m_nameInPrevious.get(), +structure->m_attributesInPrevious);
    }

} // namespace TI

#endif // Structure_h
