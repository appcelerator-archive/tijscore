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

#ifndef ErrorInstance_h
#define ErrorInstance_h

#include "Interpreter.h"
#include "JSObject.h"
#include "SourceProvider.h"

namespace TI {

    class ErrorInstance : public JSNonFinalObject {
    public:
        typedef JSNonFinalObject Base;

        DECLARE_INFO;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, TiValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ErrorInstanceType, StructureFlags), info());
        }

        static ErrorInstance* create(VM& vm, Structure* structure, const String& message, Vector<StackFrame> stackTrace = Vector<StackFrame>())
        {
            ErrorInstance* instance = new (NotNull, allocateCell<ErrorInstance>(vm.heap)) ErrorInstance(vm, structure);
            instance->finishCreation(vm, message, stackTrace);
            return instance;
        }

        static ErrorInstance* create(ExecState* exec, Structure* structure, TiValue message, Vector<StackFrame> stackTrace = Vector<StackFrame>())
        {
            return create(exec->vm(), structure, message.isUndefined() ? String() : message.toString(exec)->value(exec), stackTrace);
        }

        bool appendSourceToMessage() { return m_appendSourceToMessage; }
        void setAppendSourceToMessage() { m_appendSourceToMessage = true; }
        void clearAppendSourceToMessage() { m_appendSourceToMessage = false; }

    protected:
        explicit ErrorInstance(VM&, Structure*);

        void finishCreation(VM&, const String&, Vector<StackFrame> = Vector<StackFrame>());

        bool m_appendSourceToMessage;
    };

} // namespace TI

#endif // ErrorInstance_h
