/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007 Apple Inc. All rights reserved.
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

#ifndef Completion_h
#define Completion_h

#include "JSCTiValue.h"

namespace TI {
    
    struct ParserError;
    class ExecState;
    class JSScope;
    class SourceCode;
    class VM;

    JS_EXPORT_PRIVATE bool checkSyntax(VM&, const SourceCode&, ParserError&);
    JS_EXPORT_PRIVATE bool checkSyntax(ExecState*, const SourceCode&, TiValue* exception = 0);
    JS_EXPORT_PRIVATE TiValue evaluate(ExecState*, const SourceCode&, TiValue thisValue = TiValue(), TiValue* exception = 0);

} // namespace TI

#endif // Completion_h
