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

#ifndef RegExpConstructor_h
#define RegExpConstructor_h

#include "InternalFunction.h"
#include "RegExp.h"
#include <wtf/OwnPtr.h>

namespace TI {

    class RegExp;
    class RegExpPrototype;
    struct RegExpConstructorPrivate;

    struct RegExpConstructorPrivate : FastAllocBase {
        // Global search cache / settings
        RegExpConstructorPrivate()
            : lastNumSubPatterns(0)
            , multiline(false)
            , lastOvectorIndex(0)
        {
        }

        const Vector<int, 32>& lastOvector() const { return ovector[lastOvectorIndex]; }
        Vector<int, 32>& lastOvector() { return ovector[lastOvectorIndex]; }
        Vector<int, 32>& tempOvector() { return ovector[lastOvectorIndex ? 0 : 1]; }
        void changeLastOvector() { lastOvectorIndex = lastOvectorIndex ? 0 : 1; }

        UString input;
        UString lastInput;
        Vector<int, 32> ovector[2];
        unsigned lastNumSubPatterns : 30;
        bool multiline : 1;
        unsigned lastOvectorIndex : 1;
    };

    class RegExpConstructor : public InternalFunction {
    public:
        RegExpConstructor(TiExcState*, NonNullPassRefPtr<Structure>, RegExpPrototype*);

        static PassRefPtr<Structure> createStructure(TiValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount);
        }

        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);

        static const ClassInfo info;

        void performMatch(RegExp*, const UString&, int startOffset, int& position, int& length, int** ovector = 0);
        TiObject* arrayOfMatches(TiExcState*) const;

        void setInput(const UString&);
        const UString& input() const;

        void setMultiline(bool);
        bool multiline() const;

        TiValue getBackref(TiExcState*, unsigned) const;
        TiValue getLastParen(TiExcState*) const;
        TiValue getLeftContext(TiExcState*) const;
        TiValue getRightContext(TiExcState*) const;

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | ImplementsHasInstance | InternalFunction::StructureFlags;

    private:
        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);

        virtual const ClassInfo* classInfo() const { return &info; }

        OwnPtr<RegExpConstructorPrivate> d;
    };

    RegExpConstructor* asRegExpConstructor(TiValue);

    TiObject* constructRegExp(TiExcState*, const ArgList&);

    inline RegExpConstructor* asRegExpConstructor(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&RegExpConstructor::info));
        return static_cast<RegExpConstructor*>(asObject(value));
    }

    /* 
      To facilitate result caching, exec(), test(), match(), search(), and replace() dipatch regular
      expression matching through the performMatch function. We use cached results to calculate, 
      e.g., RegExp.lastMatch and RegExp.leftParen.
    */
    inline void RegExpConstructor::performMatch(RegExp* r, const UString& s, int startOffset, int& position, int& length, int** ovector)
    {
        position = r->match(s, startOffset, &d->tempOvector());

        if (ovector)
            *ovector = d->tempOvector().data();

        if (position != -1) {
            ASSERT(!d->tempOvector().isEmpty());

            length = d->tempOvector()[1] - d->tempOvector()[0];

            d->input = s;
            d->lastInput = s;
            d->changeLastOvector();
            d->lastNumSubPatterns = r->numSubpatterns();
        }
    }

} // namespace TI

#endif // RegExpConstructor_h
