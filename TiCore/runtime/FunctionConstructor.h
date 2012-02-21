/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef FunctionConstructor_h
#define FunctionConstructor_h

#include "InternalFunction.h"

namespace TI {

    class FunctionPrototype;

    class FunctionConstructor : public InternalFunction {
    public:
        FunctionConstructor(TiExcState*, TiGlobalObject*, Structure*, FunctionPrototype*);

    private:
        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);
    };

    TiObject* constructFunction(TiExcState*, TiGlobalObject*, const ArgList&, const Identifier& functionName, const UString& sourceURL, int lineNumber);
    TiObject* constructFunction(TiExcState*, TiGlobalObject*, const ArgList&);

    TiObject* constructFunctionSkippingEvalEnabledCheck(TiExcState*, TiGlobalObject*, const ArgList&, const Identifier&, const UString&, int lineNumber);

} // namespace TI

#endif // FunctionConstructor_h
