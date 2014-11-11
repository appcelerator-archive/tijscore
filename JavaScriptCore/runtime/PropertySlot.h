/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2005, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef PropertySlot_h
#define PropertySlot_h

#include "JSCTiValue.h"
#include "PropertyName.h"
#include "PropertyOffset.h"
#include "Register.h"
#include <wtf/Assertions.h>

namespace TI {

class ExecState;
class GetterSetter;

// ECMA 262-3 8.6.1
// Property attributes
enum Attribute {
    None         = 0,
    ReadOnly     = 1 << 1,  // property can be only read, not written
    DontEnum     = 1 << 2,  // property doesn't appear in (for .. in ..)
    DontDelete   = 1 << 3,  // property can't be deleted
    Function     = 1 << 4,  // property is a function - only used by static hashtables
    Accessor     = 1 << 5,  // property is a getter/setter
};

class PropertySlot {
    enum PropertyType {
        TypeUnset,
        TypeValue,
        TypeGetter,
        TypeCustom,
        TypeCustomIndex
    };

public:
    explicit PropertySlot(const TiValue thisValue)
        : m_propertyType(TypeUnset)
        , m_offset(invalidOffset)
        , m_thisValue(thisValue)
    {
    }

    typedef EncodedTiValue (*GetValueFunc)(ExecState*, EncodedTiValue slotBase, EncodedTiValue thisValue, PropertyName);
    typedef EncodedTiValue (*GetIndexValueFunc)(ExecState*, EncodedTiValue slotBase, EncodedTiValue thisValue, unsigned);

    TiValue getValue(ExecState*, PropertyName) const;
    TiValue getValue(ExecState*, unsigned propertyName) const;

    bool isCacheable() const { return m_offset != invalidOffset; }
    bool isValue() const { return m_propertyType == TypeValue; }
    bool isAccessor() const { return m_propertyType == TypeGetter; }
    bool isCustom() const { return m_propertyType == TypeCustom; }
    bool isCacheableValue() const { return isCacheable() && isValue(); }
    bool isCacheableGetter() const { return isCacheable() && isAccessor(); }
    bool isCacheableCustom() const { return isCacheable() && isCustom(); }

    unsigned attributes() const { return m_attributes; }

    PropertyOffset cachedOffset() const
    {
        ASSERT(isCacheable());
        return m_offset;
    }

    GetterSetter* getterSetter() const
    {
        ASSERT(isAccessor());
        return m_data.getter.getterSetter;
    }

    GetValueFunc customGetter() const
    {
        ASSERT(isCacheableCustom());
        return m_data.custom.getValue;
    }

    JSObject* slotBase() const
    {
        ASSERT(m_propertyType != TypeUnset);
        return m_slotBase;
    }

    void setValue(JSObject* slotBase, unsigned attributes, TiValue value)
    {
        ASSERT(value);
        m_data.value = TiValue::encode(value);
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeValue;
        m_offset = invalidOffset;
    }
    
    void setValue(JSObject* slotBase, unsigned attributes, TiValue value, PropertyOffset offset)
    {
        ASSERT(value);
        m_data.value = TiValue::encode(value);
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeValue;
        m_offset = offset;
    }

    void setValue(JSString*, unsigned attributes, TiValue value)
    {
        ASSERT(value);
        m_data.value = TiValue::encode(value);
        m_attributes = attributes;

        m_slotBase = 0;
        m_propertyType = TypeValue;
        m_offset = invalidOffset;
    }

    void setCustom(JSObject* slotBase, unsigned attributes, GetValueFunc getValue)
    {
        ASSERT(getValue);
        m_data.custom.getValue = getValue;
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeCustom;
        m_offset = invalidOffset;
    }
    
    void setCacheableCustom(JSObject* slotBase, unsigned attributes, GetValueFunc getValue)
    {
        ASSERT(getValue);
        m_data.custom.getValue = getValue;
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeCustom;
        m_offset = !invalidOffset;
    }

    void setCustomIndex(JSObject* slotBase, unsigned attributes, unsigned index, GetIndexValueFunc getIndexValue)
    {
        ASSERT(getIndexValue);
        m_data.customIndex.getIndexValue = getIndexValue;
        m_data.customIndex.index = index;
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeCustomIndex;
        m_offset = invalidOffset;
    }

    void setGetterSlot(JSObject* slotBase, unsigned attributes, GetterSetter* getterSetter)
    {
        ASSERT(getterSetter);
        m_data.getter.getterSetter = getterSetter;
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeGetter;
        m_offset = invalidOffset;
    }

    void setCacheableGetterSlot(JSObject* slotBase, unsigned attributes, GetterSetter* getterSetter, PropertyOffset offset)
    {
        ASSERT(getterSetter);
        m_data.getter.getterSetter = getterSetter;
        m_attributes = attributes;

        ASSERT(slotBase);
        m_slotBase = slotBase;
        m_propertyType = TypeGetter;
        m_offset = offset;
    }

    void setUndefined()
    {
        m_data.value = TiValue::encode(jsUndefined());
        m_attributes = ReadOnly | DontDelete | DontEnum;

        m_slotBase = 0;
        m_propertyType = TypeValue;
        m_offset = invalidOffset;
    }

private:
    JS_EXPORT_PRIVATE TiValue functionGetter(ExecState*) const;

    unsigned m_attributes;
    union {
        EncodedTiValue value;
        struct {
            GetterSetter* getterSetter;
        } getter;
        struct {
            GetValueFunc getValue;
        } custom;
        struct {
            GetIndexValueFunc getIndexValue;
            unsigned index;
        } customIndex;
    } m_data;

    PropertyType m_propertyType;
    PropertyOffset m_offset;
    const TiValue m_thisValue;
    JSObject* m_slotBase;
};

} // namespace TI

#endif // PropertySlot_h
