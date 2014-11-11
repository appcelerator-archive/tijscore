/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef MathObject_h
#define MathObject_h

#include "JSObject.h"

namespace TI {

    class MathObject : public JSNonFinalObject {
    private:
        MathObject(VM&, Structure*);

    public:
        typedef JSNonFinalObject Base;

        static MathObject* create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
        {
            MathObject* object = new (NotNull, allocateCell<MathObject>(vm.heap)) MathObject(vm, structure);
            object->finishCreation(vm, globalObject);
            return object;
        }

        DECLARE_INFO;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
        }

    protected:
        void finishCreation(VM&, JSGlobalObject*);
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | JSObject::StructureFlags;
    };

} // namespace TI

#endif // MathObject_h
