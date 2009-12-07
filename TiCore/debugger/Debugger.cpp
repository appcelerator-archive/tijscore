/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
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

#include "config.h"
#include "Debugger.h"

#include "CollectorHeapIterator.h"
#include "Error.h"
#include "Interpreter.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "Parser.h"
#include "Protect.h"

namespace TI {

Debugger::~Debugger()
{
    HashSet<TiGlobalObject*>::iterator end = m_globalObjects.end();
    for (HashSet<TiGlobalObject*>::iterator it = m_globalObjects.begin(); it != end; ++it)
        (*it)->setDebugger(0);
}

void Debugger::attach(TiGlobalObject* globalObject)
{
    ASSERT(!globalObject->debugger());
    globalObject->setDebugger(this);
    m_globalObjects.add(globalObject);
}

void Debugger::detach(TiGlobalObject* globalObject)
{
    ASSERT(m_globalObjects.contains(globalObject));
    m_globalObjects.remove(globalObject);
    globalObject->setDebugger(0);
}

void Debugger::recompileAllTiFunctions(TiGlobalData* globalData)
{
    // If Ti is running, it's not safe to recompile, since we'll end
    // up throwing away code that is live on the stack.
    ASSERT(!globalData->dynamicGlobalObject);
    if (globalData->dynamicGlobalObject)
        return;

    typedef HashSet<FunctionExecutable*> FunctionExecutableSet;
    typedef HashMap<SourceProvider*, TiExcState*> SourceProviderMap;

    FunctionExecutableSet functionExecutables;
    SourceProviderMap sourceProviders;

    Heap::iterator heapEnd = globalData->heap.primaryHeapEnd();
    for (Heap::iterator it = globalData->heap.primaryHeapBegin(); it != heapEnd; ++it) {
        if (!(*it)->inherits(&TiFunction::info))
            continue;

        TiFunction* function = asFunction(*it);
        if (function->executable()->isHostFunction())
            continue;

        FunctionExecutable* executable = function->jsExecutable();

        // Check if the function is already in the set - if so,
        // we've already retranslated it, nothing to do here.
        if (!functionExecutables.add(executable).second)
            continue;

        TiExcState* exec = function->scope().globalObject()->TiGlobalObject::globalExec();
        executable->recompile(exec);
        if (function->scope().globalObject()->debugger() == this)
            sourceProviders.add(executable->source().provider(), exec);
    }

    // Call sourceParsed() after reparsing all functions because it will execute
    // Ti in the inspector.
    SourceProviderMap::const_iterator end = sourceProviders.end();
    for (SourceProviderMap::const_iterator iter = sourceProviders.begin(); iter != end; ++iter)
        sourceParsed(iter->second, SourceCode(iter->first), -1, 0);
}

TiValue evaluateInGlobalCallFrame(const UString& script, TiValue& exception, TiGlobalObject* globalObject)
{
    CallFrame* globalCallFrame = globalObject->globalExec();

    RefPtr<EvalExecutable> eval = EvalExecutable::create(globalCallFrame, makeSource(script));
    TiObject* error = eval->compile(globalCallFrame, globalCallFrame->scopeChain());
    if (error)
        return error;

    return globalObject->globalData()->interpreter->execute(eval.get(), globalCallFrame, globalObject, globalCallFrame->scopeChain(), &exception);
}

} // namespace TI
