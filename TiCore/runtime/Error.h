/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef Error_h
#define Error_h

#include "TiObject.h"
#include <stdint.h>

namespace TI {

    class TiExcState;
    class TiGlobalData;
    class TiGlobalObject;
    class TiObject;
    class SourceCode;
    class Structure;
    class UString;

    // Methods to create a range of internal errors.
    TiObject* createError(TiGlobalObject*, const UString&);
    TiObject* createEvalError(TiGlobalObject*, const UString&);
    TiObject* createRangeError(TiGlobalObject*, const UString&);
    TiObject* createReferenceError(TiGlobalObject*, const UString&);
    TiObject* createSyntaxError(TiGlobalObject*, const UString&);
    TiObject* createTypeError(TiGlobalObject*, const UString&);
    TiObject* createURIError(TiGlobalObject*, const UString&);
    // TiExcState wrappers.
    TiObject* createError(TiExcState*, const UString&);
    TiObject* createEvalError(TiExcState*, const UString&);
    TiObject* createRangeError(TiExcState*, const UString&);
    TiObject* createReferenceError(TiExcState*, const UString&);
    TiObject* createSyntaxError(TiExcState*, const UString&);
    TiObject* createTypeError(TiExcState*, const UString&);
    TiObject* createURIError(TiExcState*, const UString&);

    // Methods to add 
    bool hasErrorInfo(TiExcState*, TiObject* error);
    TiObject* addErrorInfo(TiGlobalData*, TiObject* error, int line, const SourceCode&);
    // TiExcState wrappers.
    TiObject* addErrorInfo(TiExcState*, TiObject* error, int line, const SourceCode&);

    // Methods to throw Errors.
    TiValue throwError(TiExcState*, TiValue);
    TiObject* throwError(TiExcState*, TiObject*);

    // Convenience wrappers, create an throw an exception with a default message.
    TiObject* throwTypeError(TiExcState*);
    TiObject* throwSyntaxError(TiExcState*);

    // Convenience wrappers, wrap result as an EncodedTiValue.
    inline EncodedTiValue throwVMError(TiExcState* exec, TiValue error) { return TiValue::encode(throwError(exec, error)); }
    inline EncodedTiValue throwVMTypeError(TiExcState* exec) { return TiValue::encode(throwTypeError(exec)); }

    TiValue createTypeErrorFunction(TiExcState* exec, const UString& message);
    
} // namespace TI

#endif // Error_h
