/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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

#include "Identifier.h"
#include "TiValue.h"
#include "Register.h"
#include <wtf/Assertions.h>
#include <wtf/NotFound.h>

namespace TI {

    class TiExcState;
    class TiObject;

#define JSC_VALUE_SLOT_MARKER 0
#define JSC_REGISTER_SLOT_MARKER reinterpret_cast<GetValueFunc>(1)
#define INDEX_GETTER_MARKER reinterpret_cast<GetValueFunc>(2)
#define GETTER_FUNCTION_MARKER reinterpret_cast<GetValueFunc>(3)

    class PropertySlot {
    public:
        enum CachedPropertyType {
            Uncacheable,
            Getter,
            Custom,
            Value
        };

        PropertySlot()
            : m_cachedPropertyType(Uncacheable)
        {
            clearBase();
            clearOffset();
            clearValue();
        }

        explicit PropertySlot(const TiValue base)
            : m_slotBase(base)
            , m_cachedPropertyType(Uncacheable)
        {
            clearOffset();
            clearValue();
        }

        typedef TiValue (*GetValueFunc)(TiExcState*, TiValue slotBase, const Identifier&);
        typedef TiValue (*GetIndexValueFunc)(TiExcState*, TiValue slotBase, unsigned);

        TiValue getValue(TiExcState* exec, const Identifier& propertyName) const
        {
            if (m_getValue == JSC_VALUE_SLOT_MARKER)
                return *m_data.valueSlot;
            if (m_getValue == JSC_REGISTER_SLOT_MARKER)
                return (*m_data.registerSlot).jsValue();
            if (m_getValue == INDEX_GETTER_MARKER)
                return m_getIndexValue(exec, slotBase(), index());
            if (m_getValue == GETTER_FUNCTION_MARKER)
                return functionGetter(exec);
            return m_getValue(exec, slotBase(), propertyName);
        }

        TiValue getValue(TiExcState* exec, unsigned propertyName) const
        {
            if (m_getValue == JSC_VALUE_SLOT_MARKER)
                return *m_data.valueSlot;
            if (m_getValue == JSC_REGISTER_SLOT_MARKER)
                return (*m_data.registerSlot).jsValue();
            if (m_getValue == INDEX_GETTER_MARKER)
                return m_getIndexValue(exec, m_slotBase, m_data.index);
            if (m_getValue == GETTER_FUNCTION_MARKER)
                return functionGetter(exec);
            return m_getValue(exec, slotBase(), Identifier::from(exec, propertyName));
        }

        CachedPropertyType cachedPropertyType() const { return m_cachedPropertyType; }
        bool isCacheable() const { return m_cachedPropertyType != Uncacheable; }
        bool isCacheableValue() const { return m_cachedPropertyType == Value; }
        size_t cachedOffset() const
        {
            ASSERT(isCacheable());
            return m_offset;
        }

        void setValueSlot(TiValue* valueSlot) 
        {
            ASSERT(valueSlot);
            clearBase();
            clearOffset();
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_data.valueSlot = valueSlot;
        }
        
        void setValueSlot(TiValue slotBase, TiValue* valueSlot)
        {
            ASSERT(valueSlot);
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_slotBase = slotBase;
            m_data.valueSlot = valueSlot;
        }
        
        void setValueSlot(TiValue slotBase, TiValue* valueSlot, size_t offset)
        {
            ASSERT(valueSlot);
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_slotBase = slotBase;
            m_data.valueSlot = valueSlot;
            m_offset = offset;
            m_cachedPropertyType = Value;
        }
        
        void setValue(TiValue value)
        {
            ASSERT(value);
            clearBase();
            clearOffset();
            m_getValue = JSC_VALUE_SLOT_MARKER;
            m_value = value;
            m_data.valueSlot = &m_value;
        }

        void setRegisterSlot(Register* registerSlot)
        {
            ASSERT(registerSlot);
            clearBase();
            clearOffset();
            m_getValue = JSC_REGISTER_SLOT_MARKER;
            m_data.registerSlot = registerSlot;
        }

        void setCustom(TiValue slotBase, GetValueFunc getValue)
        {
            ASSERT(slotBase);
            ASSERT(getValue);
            m_getValue = getValue;
            m_getIndexValue = 0;
            m_slotBase = slotBase;
        }
        
        void setCacheableCustom(TiValue slotBase, GetValueFunc getValue)
        {
            ASSERT(slotBase);
            ASSERT(getValue);
            m_getValue = getValue;
            m_getIndexValue = 0;
            m_slotBase = slotBase;
            m_cachedPropertyType = Custom;
        }

        void setCustomIndex(TiValue slotBase, unsigned index, GetIndexValueFunc getIndexValue)
        {
            ASSERT(slotBase);
            ASSERT(getIndexValue);
            m_getValue = INDEX_GETTER_MARKER;
            m_getIndexValue = getIndexValue;
            m_slotBase = slotBase;
            m_data.index = index;
        }

        void setGetterSlot(TiObject* getterFunc)
        {
            ASSERT(getterFunc);
            m_thisValue = m_slotBase;
            m_getValue = GETTER_FUNCTION_MARKER;
            m_data.getterFunc = getterFunc;
        }

        void setCacheableGetterSlot(TiValue slotBase, TiObject* getterFunc, unsigned offset)
        {
            ASSERT(getterFunc);
            m_getValue = GETTER_FUNCTION_MARKER;
            m_thisValue = m_slotBase;
            m_slotBase = slotBase;
            m_data.getterFunc = getterFunc;
            m_offset = offset;
            m_cachedPropertyType = Getter;
        }

        void setUndefined()
        {
            setValue(jsUndefined());
        }

        TiValue slotBase() const
        {
            return m_slotBase;
        }

        void setBase(TiValue base)
        {
            ASSERT(m_slotBase);
            ASSERT(base);
            m_slotBase = base;
        }

        void clearBase()
        {
#ifndef NDEBUG
            m_slotBase = TiValue();
#endif
        }

        void clearValue()
        {
#ifndef NDEBUG
            m_value = TiValue();
#endif
        }

        void clearOffset()
        {
            // Clear offset even in release builds, in case this PropertySlot has been used before.
            // (For other data members, we don't need to clear anything because reuse would meaningfully overwrite them.)
            m_offset = 0;
            m_cachedPropertyType = Uncacheable;
        }

        unsigned index() const { return m_data.index; }

        TiValue thisValue() const { return m_thisValue; }

        GetValueFunc customGetter() const
        {
            ASSERT(m_cachedPropertyType == Custom);
            return m_getValue;
        }
    private:
        TiValue functionGetter(TiExcState*) const;

        GetValueFunc m_getValue;
        GetIndexValueFunc m_getIndexValue;
        
        TiValue m_slotBase;
        union {
            TiObject* getterFunc;
            TiValue* valueSlot;
            Register* registerSlot;
            unsigned index;
        } m_data;

        TiValue m_value;
        TiValue m_thisValue;

        size_t m_offset;
        CachedPropertyType m_cachedPropertyType;
    };

} // namespace TI

#endif // PropertySlot_h
