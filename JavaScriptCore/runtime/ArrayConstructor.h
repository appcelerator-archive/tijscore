/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
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

#ifndef ArrayConstructor_h
#define ArrayConstructor_h

#include "InternalFunction.h"

namespace TI {

class ArrayAllocationProfile;
class ArrayPrototype;
class JSArray;

class ArrayConstructor : public InternalFunction {
public:
    typedef InternalFunction Base;

    static ArrayConstructor* create(VM& vm, Structure* structure, ArrayPrototype* arrayPrototype)
    {
        ArrayConstructor* constructor = new (NotNull, allocateCell<ArrayConstructor>(vm.heap)) ArrayConstructor(vm, structure);
        constructor->finishCreation(vm, arrayPrototype);
        return constructor;
    }

    DECLARE_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

protected:
    void finishCreation(VM&, ArrayPrototype*);
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InternalFunction::StructureFlags;

private:
    ArrayConstructor(VM&, Structure*);
    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);

    static ConstructType getConstructData(JSCell*, ConstructData&);
    static CallType getCallData(JSCell*, CallData&);
};

JSObject* constructArrayWithSizeQuirk(ExecState*, ArrayAllocationProfile*, JSGlobalObject*, TiValue);

} // namespace TI

#endif // ArrayConstructor_h
