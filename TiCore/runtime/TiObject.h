/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef TiObject_h
#define TiObject_h

#include "ArgList.h"
#include "ClassInfo.h"
#include "CommonIdentifiers.h"
#include "Completion.h"
#include "CallFrame.h"
#include "TiCell.h"
#include "JSNumberCell.h"
#include "MarkStack.h"
#include "PropertySlot.h"
#include "PutPropertySlot.h"
#include "ScopeChain.h"
#include "Structure.h"
#include "TiGlobalData.h"
#include "TiString.h"
#include <wtf/StdLibExtras.h>

namespace TI {

    inline TiCell* getTiFunction(TiGlobalData& globalData, TiValue value)
    {
        if (value.isCell() && (value.asCell()->vptr() == globalData.jsFunctionVPtr))
            return value.asCell();
        return 0;
    }
    
    class HashEntry;
    class InternalFunction;
    class PropertyDescriptor;
    class PropertyNameArray;
    class Structure;
    struct HashTable;

    // ECMA 262-3 8.6.1
    // Property attributes
    enum Attribute {
        None         = 0,
        ReadOnly     = 1 << 1,  // property can be only read, not written
        DontEnum     = 1 << 2,  // property doesn't appear in (for .. in ..)
        DontDelete   = 1 << 3,  // property can't be deleted
        Function     = 1 << 4,  // property is a function - only used by static hashtables
        Getter       = 1 << 5,  // property is a getter
        Setter       = 1 << 6   // property is a setter
    };

    typedef EncodedTiValue* PropertyStorage;
    typedef const EncodedTiValue* ConstPropertyStorage;

    class TiObject : public TiCell {
        friend class BatchedTransitionOptimizer;
        friend class JIT;
        friend class TiCell;

    public:
        explicit TiObject(NonNullPassRefPtr<Structure>);

        virtual void markChildren(MarkStack&);
        ALWAYS_INLINE void markChildrenDirect(MarkStack& markStack);

        // The inline virtual destructor cannot be the first virtual function declared
        // in the class as it results in the vtable being generated as a weak symbol
        virtual ~TiObject();

        TiValue prototype() const;
        void setPrototype(TiValue prototype);
        
        void setStructure(NonNullPassRefPtr<Structure>);
        Structure* inheritorID();

        virtual UString className() const;

        TiValue get(TiExcState*, const Identifier& propertyName) const;
        TiValue get(TiExcState*, unsigned propertyName) const;

        bool getPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        bool getPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        bool getPropertyDescriptor(TiExcState*, const Identifier& propertyName, PropertyDescriptor&);

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);

        virtual void put(TiExcState*, const Identifier& propertyName, TiValue value, PutPropertySlot&);
        virtual void put(TiExcState*, unsigned propertyName, TiValue value);

        virtual void putWithAttributes(TiExcState*, const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot);
        virtual void putWithAttributes(TiExcState*, const Identifier& propertyName, TiValue value, unsigned attributes);
        virtual void putWithAttributes(TiExcState*, unsigned propertyName, TiValue value, unsigned attributes);

        bool propertyIsEnumerable(TiExcState*, const Identifier& propertyName) const;

        bool hasProperty(TiExcState*, const Identifier& propertyName) const;
        bool hasProperty(TiExcState*, unsigned propertyName) const;
        bool hasOwnProperty(TiExcState*, const Identifier& propertyName) const;

        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual bool deleteProperty(TiExcState*, unsigned propertyName);

        virtual TiValue defaultValue(TiExcState*, PreferredPrimitiveType) const;

        virtual bool hasInstance(TiExcState*, TiValue, TiValue prototypeProperty);

        virtual void getPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

        virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType = NoPreference) const;
        virtual bool getPrimitiveNumber(TiExcState*, double& number, TiValue& value);
        virtual bool toBoolean(TiExcState*) const;
        virtual double toNumber(TiExcState*) const;
        virtual UString toString(TiExcState*) const;
        virtual TiObject* toObject(TiExcState*) const;

        virtual TiObject* toThisObject(TiExcState*) const;
        virtual TiObject* unwrappedObject();

        bool getPropertySpecificValue(TiExcState* exec, const Identifier& propertyName, TiCell*& specificFunction) const;

        // This get function only looks at the property map.
        TiValue getDirect(const Identifier& propertyName) const
        {
            size_t offset = m_structure->get(propertyName);
            return offset != WTI::notFound ? getDirectOffset(offset) : TiValue();
        }

        TiValue* getDirectLocation(const Identifier& propertyName)
        {
            size_t offset = m_structure->get(propertyName);
            return offset != WTI::notFound ? locationForOffset(offset) : 0;
        }

        TiValue* getDirectLocation(const Identifier& propertyName, unsigned& attributes)
        {
            TiCell* specificFunction;
            size_t offset = m_structure->get(propertyName, attributes, specificFunction);
            return offset != WTI::notFound ? locationForOffset(offset) : 0;
        }

        size_t offsetForLocation(TiValue* location) const
        {
            return location - reinterpret_cast<const TiValue*>(propertyStorage());
        }

        void transitionTo(Structure*);

        void removeDirect(const Identifier& propertyName);
        bool hasCustomProperties() { return !m_structure->isEmpty(); }
        bool hasGetterSetterProperties() { return m_structure->hasGetterSetterProperties(); }

        void putDirect(const Identifier& propertyName, TiValue value, unsigned attr, bool checkReadOnly, PutPropertySlot& slot);
        void putDirect(const Identifier& propertyName, TiValue value, unsigned attr = 0);
        void putDirect(const Identifier& propertyName, TiValue value, PutPropertySlot&);

        void putDirectFunction(const Identifier& propertyName, TiCell* value, unsigned attr = 0);
        void putDirectFunction(const Identifier& propertyName, TiCell* value, unsigned attr, bool checkReadOnly, PutPropertySlot& slot);
        void putDirectFunction(TiExcState* exec, InternalFunction* function, unsigned attr = 0);

        void putDirectWithoutTransition(const Identifier& propertyName, TiValue value, unsigned attr = 0);
        void putDirectFunctionWithoutTransition(const Identifier& propertyName, TiCell* value, unsigned attr = 0);
        void putDirectFunctionWithoutTransition(TiExcState* exec, InternalFunction* function, unsigned attr = 0);

        // Fast access to known property offsets.
        TiValue getDirectOffset(size_t offset) const { return TiValue::decode(propertyStorage()[offset]); }
        void putDirectOffset(size_t offset, TiValue value) { propertyStorage()[offset] = TiValue::encode(value); }

        void fillGetterPropertySlot(PropertySlot&, TiValue* location);

        virtual void defineGetter(TiExcState*, const Identifier& propertyName, TiObject* getterFunction, unsigned attributes = 0);
        virtual void defineSetter(TiExcState*, const Identifier& propertyName, TiObject* setterFunction, unsigned attributes = 0);
        virtual TiValue lookupGetter(TiExcState*, const Identifier& propertyName);
        virtual TiValue lookupSetter(TiExcState*, const Identifier& propertyName);
        virtual bool defineOwnProperty(TiExcState*, const Identifier& propertyName, PropertyDescriptor&, bool shouldThrow);

        virtual bool isGlobalObject() const { return false; }
        virtual bool isVariableObject() const { return false; }
        virtual bool isActivationObject() const { return false; }
        virtual bool isNotAnObjectErrorStub() const { return false; }

        virtual ComplType exceptionType() const { return Throw; }

        void allocatePropertyStorage(size_t oldSize, size_t newSize);
        void allocatePropertyStorageInline(size_t oldSize, size_t newSize);
        bool isUsingInlineStorage() const { return m_structure->isUsingInlineStorage(); }

        static const unsigned inlineStorageCapacity = sizeof(EncodedTiValue) == 2 * sizeof(void*) ? 4 : 3;
        static const unsigned nonInlineBaseStorageCapacity = 16;

        static PassRefPtr<Structure> createStructure(TiValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount);
        }

        void flattenDictionaryObject()
        {
            m_structure->flattenDictionaryStructure(this);
        }

    protected:
        static const unsigned StructureFlags = 0;

        void putAnonymousValue(unsigned index, TiValue value)
        {
            ASSERT(index < m_structure->anonymousSlotCount());
            *locationForOffset(index) = value;
        }
        TiValue getAnonymousValue(unsigned index) const
        {
            ASSERT(index < m_structure->anonymousSlotCount());
            return *locationForOffset(index);
        }

    private:
        // Nobody should ever ask any of these questions on something already known to be a TiObject.
        using TiCell::isAPIValueWrapper;
        using TiCell::isGetterSetter;
        using TiCell::toObject;
        void getObject();
        void getString(TiExcState* exec);
        void isObject();
        void isString();
#if USE(JSVALUE32)
        void isNumber();
#endif

        ConstPropertyStorage propertyStorage() const { return (isUsingInlineStorage() ? m_inlineStorage : m_externalStorage); }
        PropertyStorage propertyStorage() { return (isUsingInlineStorage() ? m_inlineStorage : m_externalStorage); }

        const TiValue* locationForOffset(size_t offset) const
        {
            return reinterpret_cast<const TiValue*>(&propertyStorage()[offset]);
        }

        TiValue* locationForOffset(size_t offset)
        {
            return reinterpret_cast<TiValue*>(&propertyStorage()[offset]);
        }

        void putDirectInternal(const Identifier& propertyName, TiValue value, unsigned attr, bool checkReadOnly, PutPropertySlot& slot, TiCell*);
        void putDirectInternal(TiGlobalData&, const Identifier& propertyName, TiValue value, unsigned attr, bool checkReadOnly, PutPropertySlot& slot);
        void putDirectInternal(TiGlobalData&, const Identifier& propertyName, TiValue value, unsigned attr = 0);

        bool inlineGetOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);

        const HashEntry* findPropertyHashEntry(TiExcState*, const Identifier& propertyName) const;
        Structure* createInheritorID();

        union {
            PropertyStorage m_externalStorage;
            EncodedTiValue m_inlineStorage[inlineStorageCapacity];
        };

        RefPtr<Structure> m_inheritorID;
    };
    
inline TiObject* asObject(TiCell* cell)
{
    ASSERT(cell->isObject());
    return static_cast<TiObject*>(cell);
}

inline TiObject* asObject(TiValue value)
{
    return asObject(value.asCell());
}

inline TiObject::TiObject(NonNullPassRefPtr<Structure> structure)
    : TiCell(structure.releaseRef()) // ~TiObject balances this ref()
{
    ASSERT(m_structure->propertyStorageCapacity() == inlineStorageCapacity);
    ASSERT(m_structure->isEmpty());
    ASSERT(prototype().isNull() || Heap::heap(this) == Heap::heap(prototype()));
#if USE(JSVALUE64) || USE(JSVALUE32_64)
    ASSERT(OBJECT_OFFSETOF(TiObject, m_inlineStorage) % sizeof(double) == 0);
#endif
}

inline TiObject::~TiObject()
{
    ASSERT(m_structure);
    if (!isUsingInlineStorage())
        delete [] m_externalStorage;
    m_structure->deref();
}

inline TiValue TiObject::prototype() const
{
    return m_structure->storedPrototype();
}

inline void TiObject::setPrototype(TiValue prototype)
{
    ASSERT(prototype);
    RefPtr<Structure> newStructure = Structure::changePrototypeTransition(m_structure, prototype);
    setStructure(newStructure.release());
}

inline void TiObject::setStructure(NonNullPassRefPtr<Structure> structure)
{
    m_structure->deref();
    m_structure = structure.releaseRef(); // ~TiObject balances this ref()
}

inline Structure* TiObject::inheritorID()
{
    if (m_inheritorID)
        return m_inheritorID.get();
    return createInheritorID();
}

inline bool Structure::isUsingInlineStorage() const
{
    return (propertyStorageCapacity() == TiObject::inlineStorageCapacity);
}

inline bool TiCell::inherits(const ClassInfo* info) const
{
    for (const ClassInfo* ci = classInfo(); ci; ci = ci->parentClass) {
        if (ci == info)
            return true;
    }
    return false;
}

// this method is here to be after the inline declaration of TiCell::inherits
inline bool TiValue::inherits(const ClassInfo* classInfo) const
{
    return isCell() && asCell()->inherits(classInfo);
}

ALWAYS_INLINE bool TiObject::inlineGetOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (TiValue* location = getDirectLocation(propertyName)) {
        if (m_structure->hasGetterSetterProperties() && location[0].isGetterSetter())
            fillGetterPropertySlot(slot, location);
        else
            slot.setValueSlot(this, location, offsetForLocation(location));
        return true;
    }

    // non-standard Netscape extension
    if (propertyName == exec->propertyNames().underscoreProto) {
        slot.setValue(prototype());
        return true;
    }

    return false;
}

// It may seem crazy to inline a function this large, especially a virtual function,
// but it makes a big difference to property lookup that derived classes can inline their
// base class call to this.
ALWAYS_INLINE bool TiObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return inlineGetOwnPropertySlot(exec, propertyName, slot);
}

ALWAYS_INLINE bool TiCell::fastGetOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (!structure()->typeInfo().overridesGetOwnPropertySlot())
        return asObject(this)->inlineGetOwnPropertySlot(exec, propertyName, slot);
    return getOwnPropertySlot(exec, propertyName, slot);
}

// It may seem crazy to inline a function this large but it makes a big difference
// since this is function very hot in variable lookup
ALWAYS_INLINE bool TiObject::getPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    TiObject* object = this;
    while (true) {
        if (object->fastGetOwnPropertySlot(exec, propertyName, slot))
            return true;
        TiValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

ALWAYS_INLINE bool TiObject::getPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
{
    TiObject* object = this;
    while (true) {
        if (object->getOwnPropertySlot(exec, propertyName, slot))
            return true;
        TiValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

inline TiValue TiObject::get(TiExcState* exec, const Identifier& propertyName) const
{
    PropertySlot slot(this);
    if (const_cast<TiObject*>(this)->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);
    
    return jsUndefined();
}

inline TiValue TiObject::get(TiExcState* exec, unsigned propertyName) const
{
    PropertySlot slot(this);
    if (const_cast<TiObject*>(this)->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);

    return jsUndefined();
}

inline void TiObject::putDirectInternal(const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot, TiCell* specificFunction)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (m_structure->isDictionary()) {
        unsigned currentAttributes;
        TiCell* currentSpecificFunction;
        size_t offset = m_structure->get(propertyName, currentAttributes, currentSpecificFunction);
        if (offset != WTI::notFound) {
            // If there is currently a specific function, and there now either isn't,
            // or the new value is different, then despecify.
            if (currentSpecificFunction && (specificFunction != currentSpecificFunction))
                m_structure->despecifyDictionaryFunction(propertyName);
            if (checkReadOnly && currentAttributes & ReadOnly)
                return;
            putDirectOffset(offset, value);
            // At this point, the objects structure only has a specific value set if previously there
            // had been one set, and if the new value being specified is the same (otherwise we would
            // have despecified, above).  So, if currentSpecificFunction is not set, or if the new
            // value is different (or there is no new value), then the slot now has no value - and
            // as such it is cachable.
            // If there was previously a value, and the new value is the same, then we cannot cache.
            if (!currentSpecificFunction || (specificFunction != currentSpecificFunction))
                slot.setExistingProperty(this, offset);
            return;
        }

        size_t currentCapacity = m_structure->propertyStorageCapacity();
        offset = m_structure->addPropertyWithoutTransition(propertyName, attributes, specificFunction);
        if (currentCapacity != m_structure->propertyStorageCapacity())
            allocatePropertyStorage(currentCapacity, m_structure->propertyStorageCapacity());

        ASSERT(offset < m_structure->propertyStorageCapacity());
        putDirectOffset(offset, value);
        // See comment on setNewProperty call below.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return;
    }

    size_t offset;
    size_t currentCapacity = m_structure->propertyStorageCapacity();
    if (RefPtr<Structure> structure = Structure::addPropertyTransitionToExistingStructure(m_structure, propertyName, attributes, specificFunction, offset)) {    
        if (currentCapacity != structure->propertyStorageCapacity())
            allocatePropertyStorage(currentCapacity, structure->propertyStorageCapacity());

        ASSERT(offset < structure->propertyStorageCapacity());
        setStructure(structure.release());
        putDirectOffset(offset, value);
        // This is a new property; transitions with specific values are not currently cachable,
        // so leave the slot in an uncachable state.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return;
    }

    unsigned currentAttributes;
    TiCell* currentSpecificFunction;
    offset = m_structure->get(propertyName, currentAttributes, currentSpecificFunction);
    if (offset != WTI::notFound) {
        if (checkReadOnly && currentAttributes & ReadOnly)
            return;

        // There are three possibilities here:
        //  (1) There is an existing specific value set, and we're overwriting with *the same value*.
        //       * Do nothing - no need to despecify, but that means we can't cache (a cached
        //         put could write a different value). Leave the slot in an uncachable state.
        //  (2) There is a specific value currently set, but we're writing a different value.
        //       * First, we have to despecify.  Having done so, this is now a regular slot
        //         with no specific value, so go ahead & cache like normal.
        //  (3) Normal case, there is no specific value set.
        //       * Go ahead & cache like normal.
        if (currentSpecificFunction) {
            // case (1) Do the put, then return leaving the slot uncachable.
            if (specificFunction == currentSpecificFunction) {
                putDirectOffset(offset, value);
                return;
            }
            // case (2) Despecify, fall through to (3).
            setStructure(Structure::despecifyFunctionTransition(m_structure, propertyName));
        }

        // case (3) set the slot, do the put, return.
        slot.setExistingProperty(this, offset);
        putDirectOffset(offset, value);
        return;
    }

    // If we have a specific function, we may have got to this point if there is
    // already a transition with the correct property name and attributes, but
    // specialized to a different function.  In this case we just want to give up
    // and despecialize the transition.
    // In this case we clear the value of specificFunction which will result
    // in us adding a non-specific transition, and any subsequent lookup in
    // Structure::addPropertyTransitionToExistingStructure will just use that.
    if (specificFunction && m_structure->hasTransition(propertyName, attributes))
        specificFunction = 0;

    RefPtr<Structure> structure = Structure::addPropertyTransition(m_structure, propertyName, attributes, specificFunction, offset);

    if (currentCapacity != structure->propertyStorageCapacity())
        allocatePropertyStorage(currentCapacity, structure->propertyStorageCapacity());

    ASSERT(offset < structure->propertyStorageCapacity());
    setStructure(structure.release());
    putDirectOffset(offset, value);
    // This is a new property; transitions with specific values are not currently cachable,
    // so leave the slot in an uncachable state.
    if (!specificFunction)
        slot.setNewProperty(this, offset);
}

inline void TiObject::putDirectInternal(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    putDirectInternal(propertyName, value, attributes, checkReadOnly, slot, getTiFunction(globalData, value));
}

inline void TiObject::putDirectInternal(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    PutPropertySlot slot;
    putDirectInternal(propertyName, value, attributes, false, slot, getTiFunction(globalData, value));
}

inline void TiObject::putDirect(const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    putDirectInternal(propertyName, value, attributes, checkReadOnly, slot, 0);
}

inline void TiObject::putDirect(const Identifier& propertyName, TiValue value, unsigned attributes)
{
    PutPropertySlot slot;
    putDirectInternal(propertyName, value, attributes, false, slot, 0);
}

inline void TiObject::putDirect(const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    putDirectInternal(propertyName, value, 0, false, slot, 0);
}

inline void TiObject::putDirectFunction(const Identifier& propertyName, TiCell* value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    putDirectInternal(propertyName, value, attributes, checkReadOnly, slot, value);
}

inline void TiObject::putDirectFunction(const Identifier& propertyName, TiCell* value, unsigned attr)
{
    PutPropertySlot slot;
    putDirectInternal(propertyName, value, attr, false, slot, value);
}

inline void TiObject::putDirectWithoutTransition(const Identifier& propertyName, TiValue value, unsigned attributes)
{
    size_t currentCapacity = m_structure->propertyStorageCapacity();
    size_t offset = m_structure->addPropertyWithoutTransition(propertyName, attributes, 0);
    if (currentCapacity != m_structure->propertyStorageCapacity())
        allocatePropertyStorage(currentCapacity, m_structure->propertyStorageCapacity());
    putDirectOffset(offset, value);
}

inline void TiObject::putDirectFunctionWithoutTransition(const Identifier& propertyName, TiCell* value, unsigned attributes)
{
    size_t currentCapacity = m_structure->propertyStorageCapacity();
    size_t offset = m_structure->addPropertyWithoutTransition(propertyName, attributes, value);
    if (currentCapacity != m_structure->propertyStorageCapacity())
        allocatePropertyStorage(currentCapacity, m_structure->propertyStorageCapacity());
    putDirectOffset(offset, value);
}

inline void TiObject::transitionTo(Structure* newStructure)
{
    if (m_structure->propertyStorageCapacity() != newStructure->propertyStorageCapacity())
        allocatePropertyStorage(m_structure->propertyStorageCapacity(), newStructure->propertyStorageCapacity());
    setStructure(newStructure);
}

inline TiValue TiObject::toPrimitive(TiExcState* exec, PreferredPrimitiveType preferredType) const
{
    return defaultValue(exec, preferredType);
}

inline TiValue TiValue::get(TiExcState* exec, const Identifier& propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline TiValue TiValue::get(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot) const
{
    if (UNLIKELY(!isCell())) {
        TiObject* prototype = synthesizePrototype(exec);
        if (propertyName == exec->propertyNames().underscoreProto)
            return prototype;
        if (!prototype->getPropertySlot(exec, propertyName, slot))
            return jsUndefined();
        return slot.getValue(exec, propertyName);
    }
    TiCell* cell = asCell();
    while (true) {
        if (cell->fastGetOwnPropertySlot(exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        TiValue prototype = asObject(cell)->prototype();
        if (!prototype.isObject())
            return jsUndefined();
        cell = asObject(prototype);
    }
}

inline TiValue TiValue::get(TiExcState* exec, unsigned propertyName) const
{
    PropertySlot slot(asValue());
    return get(exec, propertyName, slot);
}

inline TiValue TiValue::get(TiExcState* exec, unsigned propertyName, PropertySlot& slot) const
{
    if (UNLIKELY(!isCell())) {
        TiObject* prototype = synthesizePrototype(exec);
        if (!prototype->getPropertySlot(exec, propertyName, slot))
            return jsUndefined();
        return slot.getValue(exec, propertyName);
    }
    TiCell* cell = const_cast<TiCell*>(asCell());
    while (true) {
        if (cell->getOwnPropertySlot(exec, propertyName, slot))
            return slot.getValue(exec, propertyName);
        TiValue prototype = asObject(cell)->prototype();
        if (!prototype.isObject())
            return jsUndefined();
        cell = prototype.asCell();
    }
}

inline void TiValue::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    if (UNLIKELY(!isCell())) {
        synthesizeObject(exec)->put(exec, propertyName, value, slot);
        return;
    }
    asCell()->put(exec, propertyName, value, slot);
}

inline void TiValue::putDirect(TiExcState*, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    ASSERT(isCell() && isObject());
    asObject(asCell())->putDirect(propertyName, value, slot);
}

inline void TiValue::put(TiExcState* exec, unsigned propertyName, TiValue value)
{
    if (UNLIKELY(!isCell())) {
        synthesizeObject(exec)->put(exec, propertyName, value);
        return;
    }
    asCell()->put(exec, propertyName, value);
}

ALWAYS_INLINE void TiObject::allocatePropertyStorageInline(size_t oldSize, size_t newSize)
{
    ASSERT(newSize > oldSize);

    // It's important that this function not rely on m_structure, since
    // we might be in the middle of a transition.
    bool wasInline = (oldSize == TiObject::inlineStorageCapacity);

    PropertyStorage oldPropertyStorage = (wasInline ? m_inlineStorage : m_externalStorage);
    PropertyStorage newPropertyStorage = new EncodedTiValue[newSize];

    for (unsigned i = 0; i < oldSize; ++i)
       newPropertyStorage[i] = oldPropertyStorage[i];

    if (!wasInline)
        delete [] oldPropertyStorage;

    m_externalStorage = newPropertyStorage;
}

ALWAYS_INLINE void TiObject::markChildrenDirect(MarkStack& markStack)
{
    TiCell::markChildren(markStack);

    markStack.append(prototype());
    
    PropertyStorage storage = propertyStorage();
    size_t storageSize = m_structure->propertyStorageSize();
    markStack.appendValues(reinterpret_cast<TiValue*>(storage), storageSize);
}

// --- TiValue inlines ----------------------------

ALWAYS_INLINE UString TiValue::toThisString(TiExcState* exec) const
{
    return isString() ? static_cast<TiString*>(asCell())->value(exec) : toThisObject(exec)->toString(exec);
}

inline TiString* TiValue::toThisTiString(TiExcState* exec) const
{
    return isString() ? static_cast<TiString*>(asCell()) : jsString(exec, toThisObject(exec)->toString(exec));
}

} // namespace TI

#endif // TiObject_h
