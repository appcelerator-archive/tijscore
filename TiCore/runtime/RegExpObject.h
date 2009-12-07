/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
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

#ifndef RegExpObject_h
#define RegExpObject_h

#include "TiObject.h"
#include "RegExp.h"

namespace TI {

    class RegExpObject : public TiObject {
    public:
        RegExpObject(NonNullPassRefPtr<Structure>, NonNullPassRefPtr<RegExp>);
        virtual ~RegExpObject();

        void setRegExp(PassRefPtr<RegExp> r) { d->regExp = r; }
        RegExp* regExp() const { return d->regExp.get(); }

        void setLastIndex(double lastIndex) { d->lastIndex = lastIndex; }
        double lastIndex() const { return d->lastIndex; }

        TiValue test(TiExcState*, const ArgList&);
        TiValue exec(TiExcState*, const ArgList&);

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;

        static PassRefPtr<Structure> createStructure(TiValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags));
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | TiObject::StructureFlags;

    private:
        bool match(TiExcState*, const ArgList&);

        virtual CallType getCallData(CallData&);

        struct RegExpObjectData : FastAllocBase {
            RegExpObjectData(NonNullPassRefPtr<RegExp> regExp, double lastIndex)
                : regExp(regExp)
                , lastIndex(lastIndex)
            {
            }

            RefPtr<RegExp> regExp;
            double lastIndex;
        };

        OwnPtr<RegExpObjectData> d;
    };

    RegExpObject* asRegExpObject(TiValue);

    inline RegExpObject* asRegExpObject(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&RegExpObject::info));
        return static_cast<RegExpObject*>(asObject(value));
    }

} // namespace TI

#endif // RegExpObject_h
