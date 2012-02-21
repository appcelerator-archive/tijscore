/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2006 Maks Orlovich
 *  Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSWrapperObject_h
#define JSWrapperObject_h

#include "TiObject.h"

namespace TI {

    // This class is used as a base for classes such as String,
    // Number, Boolean and Date which are wrappers for primitive types.
    class JSWrapperObject : public JSNonFinalObject {
    protected:
        explicit JSWrapperObject(TiGlobalData&, Structure*);

    public:
        TiValue internalValue() const;
        void setInternalValue(TiGlobalData&, TiValue);

        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype) 
        { 
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

    protected:
        static const unsigned StructureFlags = OverridesVisitChildren | JSNonFinalObject::StructureFlags;

    private:
        virtual void visitChildren(SlotVisitor&);
        
        WriteBarrier<Unknown> m_internalValue;
    };

    inline JSWrapperObject::JSWrapperObject(TiGlobalData& globalData, Structure* structure)
        : JSNonFinalObject(globalData, structure)
    {
    }

    inline TiValue JSWrapperObject::internalValue() const
    {
        return m_internalValue.get();
    }

    inline void JSWrapperObject::setInternalValue(TiGlobalData& globalData, TiValue value)
    {
        ASSERT(value);
        ASSERT(!value.isObject());
        m_internalValue.set(globalData, this, value);
    }

} // namespace TI

#endif // JSWrapperObject_h
