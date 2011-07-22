/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef StringObject_h
#define StringObject_h

#include "JSWrapperObject.h"
#include "TiString.h"

namespace TI {

    class StringObject : public JSWrapperObject {
    public:
        StringObject(TiExcState*, NonNullPassRefPtr<Structure>);
        StringObject(TiExcState*, NonNullPassRefPtr<Structure>, const UString&);

        static StringObject* create(TiExcState*, TiString*);

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);

        virtual void put(TiExcState* exec, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const JS_EXPORTDATA ClassInfo info;

        TiString* internalValue() const { return asString(JSWrapperObject::internalValue());}

        static PassRefPtr<Structure> createStructure(TiValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount);
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesMarkChildren | OverridesGetPropertyNames | JSWrapperObject::StructureFlags;
        StringObject(NonNullPassRefPtr<Structure>, TiString*);
  };

    StringObject* asStringObject(TiValue);

    inline StringObject* asStringObject(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&StringObject::info));
        return static_cast<StringObject*>(asObject(value));
    }

} // namespace TI

#endif // StringObject_h
