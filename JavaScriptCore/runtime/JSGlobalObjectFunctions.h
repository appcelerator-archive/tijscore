/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef JSGlobalObjectFunctions_h
#define JSGlobalObjectFunctions_h

#include "JSCTiValue.h"
#include <wtf/unicode/Unicode.h>

namespace TI {

class ArgList;
class ExecState;
class JSObject;

// FIXME: These functions should really be in JSGlobalObject.cpp, but putting them there
// is a 0.5% reduction.

EncodedTiValue JSC_HOST_CALL globalFuncEval(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncParseInt(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncParseFloat(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncIsNaN(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncIsFinite(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncDecodeURI(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncDecodeURIComponent(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncEncodeURI(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncEncodeURIComponent(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncEscape(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncUnescape(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncThrowTypeError(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncProtoGetter(ExecState*);
EncodedTiValue JSC_HOST_CALL globalFuncProtoSetter(ExecState*);

static const double mantissaOverflowLowerBound = 9007199254740992.0;
double parseIntOverflow(const LChar*, int length, int radix);
ALWAYS_INLINE double parseIntOverflow(const char* s, int length, int radix) { return parseIntOverflow(reinterpret_cast<const LChar*>(s), length, radix); }
double parseIntOverflow(const UChar*, int length, int radix);
bool isStrWhiteSpace(UChar);
double jsToNumber(const WTI::String&);

} // namespace TI

#endif // JSGlobalObjectFunctions_h
