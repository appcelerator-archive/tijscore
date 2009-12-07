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

#ifndef TiGlobalObjectFunctions_h
#define TiGlobalObjectFunctions_h

#include <wtf/unicode/Unicode.h>

namespace TI {

    class ArgList;
    class TiExcState;
    class TiObject;
    class TiValue;

    // FIXME: These functions should really be in TiGlobalObject.cpp, but putting them there
    // is a 0.5% reduction.

    TiValue JSC_HOST_CALL globalFuncEval(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncParseInt(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncParseFloat(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncIsNaN(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncIsFinite(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncDecodeURI(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncDecodeURIComponent(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncEncodeURI(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncEncodeURIComponent(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncEscape(TiExcState*, TiObject*, TiValue, const ArgList&);
    TiValue JSC_HOST_CALL globalFuncUnescape(TiExcState*, TiObject*, TiValue, const ArgList&);
#ifndef NDEBUG
    TiValue JSC_HOST_CALL globalFuncJSCPrint(TiExcState*, TiObject*, TiValue, const ArgList&);
#endif

    static const double mantissaOverflowLowerBound = 9007199254740992.0;
    double parseIntOverflow(const char*, int length, int radix);
    bool isStrWhiteSpace(UChar);

} // namespace TI

#endif // TiGlobalObjectFunctions_h
