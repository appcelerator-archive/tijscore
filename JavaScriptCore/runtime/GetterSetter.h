/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
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

#ifndef GetterSetter_h
#define GetterSetter_h

#include "JSCell.h"

#include "CallFrame.h"
#include "Structure.h"

namespace TI {

    class JSObject;

    // This is an internal value object which stores getter and setter functions
    // for a property.
    class GetterSetter : public JSCell {
        friend class JIT;

    private:        
        GetterSetter(VM& vm)
            : JSCell(vm, vm.getterSetterStructure.get())
        {
        }

    public:
        typedef JSCell Base;

        static GetterSetter* create(VM& vm)
        {
            GetterSetter* getterSetter = new (NotNull, allocateCell<GetterSetter>(vm.heap)) GetterSetter(vm);
            getterSetter->finishCreation(vm);
            return getterSetter;
        }

        static void visitChildren(JSCell*, SlotVisitor&);

        JSObject* getter() const { return m_getter.get(); }
        void setGetter(VM& vm, JSObject* getter) { m_getter.setMayBeNull(vm, this, getter); }
        JSObject* setter() const { return m_setter.get(); }
        void setSetter(VM& vm, JSObject* setter) { m_setter.setMayBeNull(vm, this, setter); }
        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(GetterSetterType, OverridesVisitChildren), info());
        }
        
        DECLARE_INFO;

    private:
        WriteBarrier<JSObject> m_getter;
        WriteBarrier<JSObject> m_setter;  
    };

    GetterSetter* asGetterSetter(TiValue);

    inline GetterSetter* asGetterSetter(TiValue value)
    {
        ASSERT(value.asCell()->isGetterSetter());
        return static_cast<GetterSetter*>(value.asCell());
    }

    TiValue callGetter(ExecState*, TiValue base, TiValue getterSetter);
    void callSetter(ExecState*, TiValue base, TiValue getterSetter, TiValue value, ECMAMode);

} // namespace TI

#endif // GetterSetter_h
