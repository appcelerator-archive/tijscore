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

#ifndef BooleanConstructor_h
#define BooleanConstructor_h

#include "InternalFunction.h"

namespace TI {

class BooleanPrototype;

class BooleanConstructor : public InternalFunction {
public:
    typedef InternalFunction Base;

    static BooleanConstructor* create(VM& vm, Structure* structure, BooleanPrototype* booleanPrototype)
    {
        BooleanConstructor* constructor = new (NotNull, allocateCell<BooleanConstructor>(vm.heap)) BooleanConstructor(vm, structure);
        constructor->finishCreation(vm, booleanPrototype);
        return constructor;
    }

    DECLARE_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype) 
    { 
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info()); 
    }

protected:
    void finishCreation(VM&, BooleanPrototype*);

private:
    BooleanConstructor(VM&, Structure*);
    static ConstructType getConstructData(JSCell*, ConstructData&);
    static CallType getCallData(JSCell*, CallData&);
};

JSObject* constructBooleanFromImmediateBoolean(ExecState*, JSGlobalObject*, TiValue);
JSObject* constructBoolean(ExecState*, const ArgList&);

} // namespace TI

#endif // BooleanConstructor_h
