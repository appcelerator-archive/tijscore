/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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

    TiObject* throwTypeError(TiExcState*, const UString&);
    extern const char* StrictModeReadonlyPropertyWriteError;

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

    typedef WriteBarrierBase<Unknown>* PropertyStorage;
    typedef const WriteBarrierBase<Unknown>* ConstPropertyStorage;

    class TiObject : public TiCell {
        friend class BatchedTransitionOptimizer;
        friend class JIT;
        friend class TiCell;
        friend void setUpStaticFunctionSlot(TiExcState* exec, const HashEntry* entry, TiObject* thisObj, const Identifier& propertyName, PropertySlot& slot);

    public:
        virtual void visitChildren(SlotVisitor&);
        ALWAYS_INLINE void visitChildrenDirect(SlotVisitor&);

        // The inline virtual destructor cannot be the first virtual function declared
        // in the class as it results in the vtable being generated as a weak symbol
        virtual ~TiObject();

        TiValue prototype() const;
        void setPrototype(TiGlobalData&, TiValue prototype);
        bool setPrototypeWithCycleCheck(TiGlobalData&, TiValue prototype);
        
        void setStructure(TiGlobalData&, Structure*);
        Structure* inheritorID(TiGlobalData&);

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

        virtual void putWithAttributes(TiGlobalData*, const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot);
        virtual void putWithAttributes(TiGlobalData*, const Identifier& propertyName, TiValue value, unsigned attributes);
        virtual void putWithAttributes(TiGlobalData*, unsigned propertyName, TiValue value, unsigned attributes);
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
        virtual TiObject* toObject(TiExcState*, TiGlobalObject*) const;

        virtual TiObject* toThisObject(TiExcState*) const;
        virtual TiValue toStrictThisObject(TiExcState*) const;
        virtual TiObject* unwrappedObject();

        bool getPropertySpecificValue(TiExcState* exec, const Identifier& propertyName, TiCell*& specificFunction) const;

        // This get function only looks at the property map.
        TiValue getDirect(TiGlobalData& globalData, const Identifier& propertyName) const
        {
            size_t offset = m_structure->get(globalData, propertyName);
            return offset != WTI::notFound ? getDirectOffset(offset) : TiValue();
        }

        WriteBarrierBase<Unknown>* getDirectLocation(TiGlobalData& globalData, const Identifier& propertyName)
        {
            size_t offset = m_structure->get(globalData, propertyName);
            return offset != WTI::notFound ? locationForOffset(offset) : 0;
        }

        WriteBarrierBase<Unknown>* getDirectLocation(TiGlobalData& globalData, const Identifier& propertyName, unsigned& attributes)
        {
            TiCell* specificFunction;
            size_t offset = m_structure->get(globalData, propertyName, attributes, specificFunction);
            return offset != WTI::notFound ? locationForOffset(offset) : 0;
        }

        size_t offsetForLocation(WriteBarrierBase<Unknown>* location) const
        {
            return location - propertyStorage();
        }

        void transitionTo(TiGlobalData&, Structure*);

        void removeDirect(TiGlobalData&, const Identifier& propertyName);
        bool hasCustomProperties() { return m_structure->didTransition(); }
        bool hasGetterSetterProperties() { return m_structure->hasGetterSetterProperties(); }

        bool putDirect(TiGlobalData&, const Identifier& propertyName, TiValue, unsigned attr, bool checkReadOnly, PutPropertySlot&);
        void putDirect(TiGlobalData&, const Identifier& propertyName, TiValue, unsigned attr = 0);
        bool putDirect(TiGlobalData&, const Identifier& propertyName, TiValue, PutPropertySlot&);

        void putDirectFunction(TiGlobalData&, const Identifier& propertyName, TiCell*, unsigned attr = 0);
        void putDirectFunction(TiGlobalData&, const Identifier& propertyName, TiCell*, unsigned attr, bool checkReadOnly, PutPropertySlot&);
        void putDirectFunction(TiExcState* exec, InternalFunction* function, unsigned attr = 0);
        void putDirectFunction(TiExcState* exec, TiFunction* function, unsigned attr = 0);

        void putDirectWithoutTransition(TiGlobalData&, const Identifier& propertyName, TiValue, unsigned attr = 0);
        void putDirectFunctionWithoutTransition(TiGlobalData&, const Identifier& propertyName, TiCell* value, unsigned attr = 0);
        void putDirectFunctionWithoutTransition(TiExcState* exec, InternalFunction* function, unsigned attr = 0);
        void putDirectFunctionWithoutTransition(TiExcState* exec, TiFunction* function, unsigned attr = 0);

        // Fast access to known property offsets.
        TiValue getDirectOffset(size_t offset) const { return propertyStorage()[offset].get(); }
        void putDirectOffset(TiGlobalData& globalData, size_t offset, TiValue value) { propertyStorage()[offset].set(globalData, this, value); }
        void putUndefinedAtDirectOffset(size_t offset) { propertyStorage()[offset].setUndefined(); }

        void fillGetterPropertySlot(PropertySlot&, WriteBarrierBase<Unknown>* location);

        virtual void defineGetter(TiExcState*, const Identifier& propertyName, TiObject* getterFunction, unsigned attributes = 0);
        virtual void defineSetter(TiExcState*, const Identifier& propertyName, TiObject* setterFunction, unsigned attributes = 0);
        virtual TiValue lookupGetter(TiExcState*, const Identifier& propertyName);
        virtual TiValue lookupSetter(TiExcState*, const Identifier& propertyName);
        virtual bool defineOwnProperty(TiExcState*, const Identifier& propertyName, PropertyDescriptor&, bool shouldThrow);

        virtual bool isGlobalObject() const { return false; }
        virtual bool isVariableObject() const { return false; }
        virtual bool isActivationObject() const { return false; }
        virtual bool isStrictModeFunction() const { return false; }
        virtual bool isErrorInstance() const { return false; }

        void seal(TiGlobalData&);
        void freeze(TiGlobalData&);
        virtual void preventExtensions(TiGlobalData&);
        bool isSealed(TiGlobalData& globalData) { return m_structure->isSealed(globalData); }
        bool isFrozen(TiGlobalData& globalData) { return m_structure->isFrozen(globalData); }
        bool isExtensible() { return m_structure->isExtensible(); }

        virtual ComplType exceptionType() const { return Throw; }

        void allocatePropertyStorage(size_t oldSize, size_t newSize);
        bool isUsingInlineStorage() const { return static_cast<const void*>(m_propertyStorage) == static_cast<const void*>(this + 1); }

        static const unsigned baseExternalStorageCapacity = 16;

        void flattenDictionaryObject(TiGlobalData& globalData)
        {
            m_structure->flattenDictionaryStructure(globalData, this);
        }

        void putAnonymousValue(TiGlobalData& globalData, unsigned index, TiValue value)
        {
            ASSERT(index < m_structure->anonymousSlotCount());
            locationForOffset(index)->set(globalData, this, value);
        }
        void clearAnonymousValue(unsigned index)
        {
            ASSERT(index < m_structure->anonymousSlotCount());
            locationForOffset(index)->clear();
        }
        TiValue getAnonymousValue(unsigned index) const
        {
            ASSERT(index < m_structure->anonymousSlotCount());
            return locationForOffset(index)->get();
        }

        static size_t offsetOfInlineStorage();
        
        static JS_EXPORTDATA const ClassInfo s_info;

    protected:
        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

        static const unsigned StructureFlags = 0;

        void putThisToAnonymousValue(unsigned index)
        {
            locationForOffset(index)->setWithoutWriteBarrier(this);
        }

        // To instantiate objects you likely want JSFinalObject, below.
        // To create derived types you likely want JSNonFinalObject, below.
        TiObject(TiGlobalData&, Structure*, PropertyStorage inlineStorage);
        TiObject(VPtrStealingHackType, PropertyStorage inlineStorage)
            : TiCell(VPtrStealingHack)
            , m_propertyStorage(inlineStorage)
        {
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
        
        ConstPropertyStorage propertyStorage() const { return m_propertyStorage; }
        PropertyStorage propertyStorage() { return m_propertyStorage; }

        const WriteBarrierBase<Unknown>* locationForOffset(size_t offset) const
        {
            return &propertyStorage()[offset];
        }

        WriteBarrierBase<Unknown>* locationForOffset(size_t offset)
        {
            return &propertyStorage()[offset];
        }

        bool putDirectInternal(TiGlobalData&, const Identifier& propertyName, TiValue, unsigned attr, bool checkReadOnly, PutPropertySlot&, TiCell*);
        bool putDirectInternal(TiGlobalData&, const Identifier& propertyName, TiValue, unsigned attr, bool checkReadOnly, PutPropertySlot&);
        void putDirectInternal(TiGlobalData&, const Identifier& propertyName, TiValue value, unsigned attr = 0);

        bool inlineGetOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);

        const HashEntry* findPropertyHashEntry(TiExcState*, const Identifier& propertyName) const;
        Structure* createInheritorID(TiGlobalData&);

        PropertyStorage m_propertyStorage;
        WriteBarrier<Structure> m_inheritorID;
    };


#if USE(JSVALUE32_64)
#define JSNonFinalObject_inlineStorageCapacity 4
#define JSFinalObject_inlineStorageCapacity 6
#else
#define JSNonFinalObject_inlineStorageCapacity 2
#define JSFinalObject_inlineStorageCapacity 4
#endif

COMPILE_ASSERT((JSFinalObject_inlineStorageCapacity >= JSNonFinalObject_inlineStorageCapacity), final_storage_is_at_least_as_large_as_non_final);

    // JSNonFinalObject is a type of TiObject that has some internal storage,
    // but also preserves some space in the collector cell for additional
    // data members in derived types.
    class JSNonFinalObject : public TiObject {
        friend class TiObject;

    public:
        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

    protected:
        explicit JSNonFinalObject(VPtrStealingHackType)
            : TiObject(VPtrStealingHack, m_inlineStorage)
        {
        }
    
        explicit JSNonFinalObject(TiGlobalData& globalData, Structure* structure)
            : TiObject(globalData, structure, m_inlineStorage)
        {
            ASSERT(!(OBJECT_OFFSETOF(JSNonFinalObject, m_inlineStorage) % sizeof(double)));
            ASSERT(this->structure()->propertyStorageCapacity() == JSNonFinalObject_inlineStorageCapacity);
        }

    private:
        WriteBarrier<Unknown> m_inlineStorage[JSNonFinalObject_inlineStorageCapacity];
    };

    // JSFinalObject is a type of TiObject that contains sufficent internal
    // storage to fully make use of the colloctor cell containing it.
    class JSFinalObject : public TiObject {
        friend class TiObject;

    public:
        static JSFinalObject* create(TiExcState* exec, Structure* structure)
        {
            return new (exec) JSFinalObject(exec->globalData(), structure);
        }

        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

    private:
        explicit JSFinalObject(TiGlobalData& globalData, Structure* structure)
            : TiObject(globalData, structure, m_inlineStorage)
        {
            ASSERT(OBJECT_OFFSETOF(JSFinalObject, m_inlineStorage) % sizeof(double) == 0);
            ASSERT(this->structure()->propertyStorageCapacity() == JSFinalObject_inlineStorageCapacity);
        }

        static const unsigned StructureFlags = TiObject::StructureFlags | IsJSFinalObject;

        WriteBarrierBase<Unknown> m_inlineStorage[JSFinalObject_inlineStorageCapacity];
    };

inline size_t TiObject::offsetOfInlineStorage()
{
    ASSERT(OBJECT_OFFSETOF(JSFinalObject, m_inlineStorage) == OBJECT_OFFSETOF(JSNonFinalObject, m_inlineStorage));
    return OBJECT_OFFSETOF(JSFinalObject, m_inlineStorage);
}

inline TiObject* constructEmptyObject(TiExcState* exec, Structure* structure)
{
    return JSFinalObject::create(exec, structure);
}

inline Structure* createEmptyObjectStructure(TiGlobalData& globalData, TiValue prototype)
{
    return JSFinalObject::createStructure(globalData, prototype);
}

inline TiObject* asObject(TiCell* cell)
{
    ASSERT(cell->isObject());
    return static_cast<TiObject*>(cell);
}

inline TiObject* asObject(TiValue value)
{
    return asObject(value.asCell());
}

inline TiObject::TiObject(TiGlobalData& globalData, Structure* structure, PropertyStorage inlineStorage)
    : TiCell(globalData, structure)
    , m_propertyStorage(inlineStorage)
{
    ASSERT(inherits(&s_info));
    ASSERT(m_structure->propertyStorageCapacity() < baseExternalStorageCapacity);
    ASSERT(m_structure->isEmpty());
    ASSERT(prototype().isNull() || Heap::heap(this) == Heap::heap(prototype()));
    ASSERT(static_cast<void*>(inlineStorage) == static_cast<void*>(this + 1));
    ASSERT(m_structure->typeInfo().type() == ObjectType);
}

inline TiObject::~TiObject()
{
    if (!isUsingInlineStorage())
        delete [] m_propertyStorage;
}

inline TiValue TiObject::prototype() const
{
    return m_structure->storedPrototype();
}

inline bool TiObject::setPrototypeWithCycleCheck(TiGlobalData& globalData, TiValue prototype)
{
    TiValue nextPrototypeValue = prototype;
    while (nextPrototypeValue && nextPrototypeValue.isObject()) {
        TiObject* nextPrototype = asObject(nextPrototypeValue)->unwrappedObject();
        if (nextPrototype == this)
            return false;
        nextPrototypeValue = nextPrototype->prototype();
    }
    setPrototype(globalData, prototype);
    return true;
}

inline void TiObject::setPrototype(TiGlobalData& globalData, TiValue prototype)
{
    ASSERT(prototype);
    setStructure(globalData, Structure::changePrototypeTransition(globalData, m_structure.get(), prototype));
}

inline void TiObject::setStructure(TiGlobalData& globalData, Structure* structure)
{
    ASSERT(structure->typeInfo().overridesVisitChildren() == m_structure->typeInfo().overridesVisitChildren());
    m_structure.set(globalData, this, structure);
}

inline Structure* TiObject::inheritorID(TiGlobalData& globalData)
{
    if (m_inheritorID) {
        ASSERT(m_inheritorID->isEmpty());
        return m_inheritorID.get();
    }
    return createInheritorID(globalData);
}

inline bool Structure::isUsingInlineStorage() const
{
    return propertyStorageCapacity() < TiObject::baseExternalStorageCapacity;
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
    if (WriteBarrierBase<Unknown>* location = getDirectLocation(exec->globalData(), propertyName)) {
        if (m_structure->hasGetterSetterProperties() && location->isGetterSetter())
            fillGetterPropertySlot(slot, location);
        else
            slot.setValue(this, location->get(), offsetForLocation(location));
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

inline bool TiObject::putDirectInternal(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot, TiCell* specificFunction)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (m_structure->isDictionary()) {
        unsigned currentAttributes;
        TiCell* currentSpecificFunction;
        size_t offset = m_structure->get(globalData, propertyName, currentAttributes, currentSpecificFunction);
        if (offset != WTI::notFound) {
            // If there is currently a specific function, and there now either isn't,
            // or the new value is different, then despecify.
            if (currentSpecificFunction && (specificFunction != currentSpecificFunction))
                m_structure->despecifyDictionaryFunction(globalData, propertyName);
            if (checkReadOnly && currentAttributes & ReadOnly)
                return false;

            putDirectOffset(globalData, offset, value);
            // At this point, the objects structure only has a specific value set if previously there
            // had been one set, and if the new value being specified is the same (otherwise we would
            // have despecified, above).  So, if currentSpecificFunction is not set, or if the new
            // value is different (or there is no new value), then the slot now has no value - and
            // as such it is cachable.
            // If there was previously a value, and the new value is the same, then we cannot cache.
            if (!currentSpecificFunction || (specificFunction != currentSpecificFunction))
                slot.setExistingProperty(this, offset);
            return true;
        }

        if (checkReadOnly && !isExtensible())
            return false;

        size_t currentCapacity = m_structure->propertyStorageCapacity();
        offset = m_structure->addPropertyWithoutTransition(globalData, propertyName, attributes, specificFunction);
        if (currentCapacity != m_structure->propertyStorageCapacity())
            allocatePropertyStorage(currentCapacity, m_structure->propertyStorageCapacity());

        ASSERT(offset < m_structure->propertyStorageCapacity());
        putDirectOffset(globalData, offset, value);
        // See comment on setNewProperty call below.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return true;
    }

    size_t offset;
    size_t currentCapacity = m_structure->propertyStorageCapacity();
    if (Structure* structure = Structure::addPropertyTransitionToExistingStructure(m_structure.get(), propertyName, attributes, specificFunction, offset)) {    
        if (currentCapacity != structure->propertyStorageCapacity())
            allocatePropertyStorage(currentCapacity, structure->propertyStorageCapacity());

        ASSERT(offset < structure->propertyStorageCapacity());
        setStructure(globalData, structure);
        putDirectOffset(globalData, offset, value);
        // This is a new property; transitions with specific values are not currently cachable,
        // so leave the slot in an uncachable state.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return true;
    }

    unsigned currentAttributes;
    TiCell* currentSpecificFunction;
    offset = m_structure->get(globalData, propertyName, currentAttributes, currentSpecificFunction);
    if (offset != WTI::notFound) {
        if (checkReadOnly && currentAttributes & ReadOnly)
            return false;

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
                putDirectOffset(globalData, offset, value);
                return true;
            }
            // case (2) Despecify, fall through to (3).
            setStructure(globalData, Structure::despecifyFunctionTransition(globalData, m_structure.get(), propertyName));
        }

        // case (3) set the slot, do the put, return.
        slot.setExistingProperty(this, offset);
        putDirectOffset(globalData, offset, value);
        return true;
    }

    if (checkReadOnly && !isExtensible())
        return false;

    Structure* structure = Structure::addPropertyTransition(globalData, m_structure.get(), propertyName, attributes, specificFunction, offset);

    if (currentCapacity != structure->propertyStorageCapacity())
        allocatePropertyStorage(currentCapacity, structure->propertyStorageCapacity());

    ASSERT(offset < structure->propertyStorageCapacity());
    setStructure(globalData, structure);
    putDirectOffset(globalData, offset, value);
    // This is a new property; transitions with specific values are not currently cachable,
    // so leave the slot in an uncachable state.
    if (!specificFunction)
        slot.setNewProperty(this, offset);
    return true;
}

inline bool TiObject::putDirectInternal(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    return putDirectInternal(globalData, propertyName, value, attributes, checkReadOnly, slot, getTiFunction(globalData, value));
}

inline void TiObject::putDirectInternal(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    PutPropertySlot slot;
    putDirectInternal(globalData, propertyName, value, attributes, false, slot, getTiFunction(globalData, value));
}

inline bool TiObject::putDirect(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    return putDirectInternal(globalData, propertyName, value, attributes, checkReadOnly, slot, 0);
}

inline void TiObject::putDirect(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    PutPropertySlot slot;
    putDirectInternal(globalData, propertyName, value, attributes, false, slot, 0);
}

inline bool TiObject::putDirect(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    return putDirectInternal(globalData, propertyName, value, 0, false, slot, 0);
}

inline void TiObject::putDirectFunction(TiGlobalData& globalData, const Identifier& propertyName, TiCell* value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    putDirectInternal(globalData, propertyName, value, attributes, checkReadOnly, slot, value);
}

inline void TiObject::putDirectFunction(TiGlobalData& globalData, const Identifier& propertyName, TiCell* value, unsigned attr)
{
    PutPropertySlot slot;
    putDirectInternal(globalData, propertyName, value, attr, false, slot, value);
}

inline void TiObject::putDirectWithoutTransition(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    size_t currentCapacity = m_structure->propertyStorageCapacity();
    size_t offset = m_structure->addPropertyWithoutTransition(globalData, propertyName, attributes, 0);
    if (currentCapacity != m_structure->propertyStorageCapacity())
        allocatePropertyStorage(currentCapacity, m_structure->propertyStorageCapacity());
    putDirectOffset(globalData, offset, value);
}

inline void TiObject::putDirectFunctionWithoutTransition(TiGlobalData& globalData, const Identifier& propertyName, TiCell* value, unsigned attributes)
{
    size_t currentCapacity = m_structure->propertyStorageCapacity();
    size_t offset = m_structure->addPropertyWithoutTransition(globalData, propertyName, attributes, value);
    if (currentCapacity != m_structure->propertyStorageCapacity())
        allocatePropertyStorage(currentCapacity, m_structure->propertyStorageCapacity());
    putDirectOffset(globalData, offset, value);
}

inline void TiObject::transitionTo(TiGlobalData& globalData, Structure* newStructure)
{
    if (m_structure->propertyStorageCapacity() != newStructure->propertyStorageCapacity())
        allocatePropertyStorage(m_structure->propertyStorageCapacity(), newStructure->propertyStorageCapacity());
    setStructure(globalData, newStructure);
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

inline void TiValue::putDirect(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    ASSERT(isCell() && isObject());
    if (!asObject(asCell())->putDirect(exec->globalData(), propertyName, value, slot) && slot.isStrictMode())
        throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
}

inline void TiValue::put(TiExcState* exec, unsigned propertyName, TiValue value)
{
    if (UNLIKELY(!isCell())) {
        synthesizeObject(exec)->put(exec, propertyName, value);
        return;
    }
    asCell()->put(exec, propertyName, value);
}

ALWAYS_INLINE void TiObject::visitChildrenDirect(SlotVisitor& visitor)
{
    TiCell::visitChildren(visitor);

    PropertyStorage storage = propertyStorage();
    size_t storageSize = m_structure->propertyStorageSize();
    visitor.appendValues(storage, storageSize);
    if (m_inheritorID)
        visitor.append(&m_inheritorID);
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

inline TiValue TiValue::toStrictThisObject(TiExcState* exec) const
{
    if (!isObject())
        return *this;
    return asObject(asCell())->toStrictThisObject(exec);
}

ALWAYS_INLINE TiObject* Register::function() const
{
    if (!jsValue())
        return 0;
    return asObject(jsValue());
}

ALWAYS_INLINE Register Register::withCallee(TiObject* callee)
{
    Register r;
    r = TiValue(callee);
    return r;
}

} // namespace TI

#endif // TiObject_h
