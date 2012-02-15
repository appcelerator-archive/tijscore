/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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

#include "TiObjectWithGlobalObject.h"
#include "RegExp.h"

namespace TI {
    
    class RegExpObject : public TiObjectWithGlobalObject {
    public:
        typedef TiObjectWithGlobalObject Base;

        RegExpObject(TiGlobalObject*, Structure*, RegExp*);
        virtual ~RegExpObject();

        void setRegExp(TiGlobalData& globalData, RegExp* r) { d->regExp.set(globalData, this, r); }
        RegExp* regExp() const { return d->regExp.get(); }

        void setLastIndex(size_t lastIndex)
        {
            d->lastIndex.setWithoutWriteBarrier(jsNumber(lastIndex));
        }
        void setLastIndex(TiGlobalData& globalData, TiValue lastIndex)
        {
            d->lastIndex.set(globalData, this, lastIndex);
        }
        TiValue getLastIndex() const
        {
            return d->lastIndex.get();
        }

        TiValue test(TiExcState*);
        TiValue exec(TiExcState*);

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);

        static JS_EXPORTDATA const ClassInfo s_info;

        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

    protected:
        static const unsigned StructureFlags = OverridesVisitChildren | OverridesGetOwnPropertySlot | TiObjectWithGlobalObject::StructureFlags;

    private:
        virtual void visitChildren(SlotVisitor&);

        bool match(TiExcState*);

        struct RegExpObjectData {
            WTF_MAKE_FAST_ALLOCATED;
        public:
            RegExpObjectData(TiGlobalData& globalData, RegExpObject* owner, RegExp* regExp)
                : regExp(globalData, owner, regExp)
            {
                lastIndex.setWithoutWriteBarrier(jsNumber(0));
            }

            WriteBarrier<RegExp> regExp;
            WriteBarrier<Unknown> lastIndex;
        };
#if COMPILER(MSVC)
        friend void WTI::deleteOwnedPtr<RegExpObjectData>(RegExpObjectData*);
#endif
        OwnPtr<RegExpObjectData> d;
    };

    RegExpObject* asRegExpObject(TiValue);

    inline RegExpObject* asRegExpObject(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&RegExpObject::s_info));
        return static_cast<RegExpObject*>(asObject(value));
    }

} // namespace TI

#endif // RegExpObject_h
