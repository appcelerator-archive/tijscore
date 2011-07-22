/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
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

#ifndef GlobalEvalFunction_h
#define GlobalEvalFunction_h

#include "PrototypeFunction.h"

namespace TI {

    class TiGlobalObject;

    class GlobalEvalFunction : public PrototypeFunction {
    public:
        GlobalEvalFunction(TiExcState*, NonNullPassRefPtr<Structure>, int len, const Identifier&, NativeFunction, TiGlobalObject* expectedThisObject);
        TiGlobalObject* cachedGlobalObject() const { return m_cachedGlobalObject; }

        static PassRefPtr<Structure> createStructure(TiValue prototype) 
        { 
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount);
        }

    protected:
        static const unsigned StructureFlags = ImplementsHasInstance | OverridesMarkChildren | OverridesGetPropertyNames | PrototypeFunction::StructureFlags;

    private:
        virtual void markChildren(MarkStack&);

        TiGlobalObject* m_cachedGlobalObject;
    };

} // namespace TI

#endif // GlobalEvalFunction_h
