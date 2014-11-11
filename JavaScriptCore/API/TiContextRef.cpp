/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "TiContextRef.h"
#include "TiContextRefPrivate.h"

#include "APICast.h"
#include "CallFrame.h"
#include "CallFrameInlines.h"
#include "InitializeThreading.h"
#include "JSCallbackObject.h"
#include "TiClassRef.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "Operations.h"
#include "SourceProvider.h"
#include "StackVisitor.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

#if OS(DARWIN)
#include <mach-o/dyld.h>

static const int32_t webkitFirstVersionWithConcurrentGlobalContexts = 0x2100500; // 528.5.0
#endif

using namespace TI;

// From the API's perspective, a context group remains alive iff
//     (a) it has been TiContextGroupRetained
//     OR
//     (b) one of its contexts has been TiContextRetained

TiContextGroupRef TiContextGroupCreate()
{
    initializeThreading();
    return toRef(VM::createContextGroup().leakRef());
}

TiContextGroupRef TiContextGroupRetain(TiContextGroupRef group)
{
    toJS(group)->ref();
    return group;
}

void TiContextGroupRelease(TiContextGroupRef group)
{
    IdentifierTable* savedIdentifierTable;
    VM& vm = *toJS(group);

    {
        JSLockHolder lock(vm);
        savedIdentifierTable = wtfThreadData().setCurrentIdentifierTable(vm.identifierTable);
        vm.deref();
    }

    wtfThreadData().setCurrentIdentifierTable(savedIdentifierTable);
}

static bool internalScriptTimeoutCallback(ExecState* exec, void* callbackPtr, void* callbackData)
{
    JSShouldTerminateCallback callback = reinterpret_cast<JSShouldTerminateCallback>(callbackPtr);
    TiContextRef contextRef = toRef(exec);
    ASSERT(callback);
    return callback(contextRef, callbackData);
}

void TiContextGroupSetExecutionTimeLimit(TiContextGroupRef group, double limit, JSShouldTerminateCallback callback, void* callbackData)
{
    VM& vm = *toJS(group);
    APIEntryShim entryShim(&vm);
    Watchdog& watchdog = vm.watchdog;
    if (callback) {
        void* callbackPtr = reinterpret_cast<void*>(callback);
        watchdog.setTimeLimit(vm, limit, internalScriptTimeoutCallback, callbackPtr, callbackData);
    } else
        watchdog.setTimeLimit(vm, limit);
}

void TiContextGroupClearExecutionTimeLimit(TiContextGroupRef group)
{
    VM& vm = *toJS(group);
    APIEntryShim entryShim(&vm);
    Watchdog& watchdog = vm.watchdog;
    watchdog.setTimeLimit(vm, std::numeric_limits<double>::infinity());
}

// From the API's perspective, a global context remains alive iff it has been TiGlobalContextRetained.

TiGlobalContextRef TiGlobalContextCreate(TiClassRef globalObjectClass)
{
    initializeThreading();

#if OS(DARWIN)
    // If the application was linked before TiGlobalContextCreate was changed to use a unique VM,
    // we use a shared one for backwards compatibility.
    if (NSVersionOfLinkTimeLibrary("JavaScriptCore") <= webkitFirstVersionWithConcurrentGlobalContexts) {
        return TiGlobalContextCreateInGroup(toRef(&VM::sharedInstance()), globalObjectClass);
    }
#endif // OS(DARWIN)

    return TiGlobalContextCreateInGroup(0, globalObjectClass);
}

TiGlobalContextRef TiGlobalContextCreateInGroup(TiContextGroupRef group, TiClassRef globalObjectClass)
{
    initializeThreading();

    RefPtr<VM> vm = group ? PassRefPtr<VM>(toJS(group)) : VM::createContextGroup();

    APIEntryShim entryShim(vm.get(), false);
    vm->makeUsableFromMultipleThreads();

    if (!globalObjectClass) {
        JSGlobalObject* globalObject = JSGlobalObject::create(*vm, JSGlobalObject::createStructure(*vm, jsNull()));
        globalObject->setGlobalThis(*vm, JSProxy::create(*vm, JSProxy::createStructure(*vm, globalObject, globalObject->prototype()), globalObject));
        return TiGlobalContextRetain(toGlobalRef(globalObject->globalExec()));
    }

    JSGlobalObject* globalObject = JSCallbackObject<JSGlobalObject>::create(*vm, globalObjectClass, JSCallbackObject<JSGlobalObject>::createStructure(*vm, 0, jsNull()));
    ExecState* exec = globalObject->globalExec();
    TiValue prototype = globalObjectClass->prototype(exec);
    if (!prototype)
        prototype = jsNull();
    globalObject->resetPrototype(*vm, prototype);
    return TiGlobalContextRetain(toGlobalRef(exec));
}

TiGlobalContextRef TiGlobalContextRetain(TiGlobalContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    VM& vm = exec->vm();
    gcProtect(exec->vmEntryGlobalObject());
    vm.ref();
    return ctx;
}

void TiGlobalContextRelease(TiGlobalContextRef ctx)
{
    IdentifierTable* savedIdentifierTable;
    ExecState* exec = toJS(ctx);
    {
        JSLockHolder lock(exec);

        VM& vm = exec->vm();
        savedIdentifierTable = wtfThreadData().setCurrentIdentifierTable(vm.identifierTable);

        bool protectCountIsZero = Heap::heap(exec->vmEntryGlobalObject())->unprotect(exec->vmEntryGlobalObject());
        if (protectCountIsZero)
            vm.heap.reportAbandonedObjectGraph();
        vm.deref();
    }

    wtfThreadData().setCurrentIdentifierTable(savedIdentifierTable);
}

TiObjectRef TiContextGetGlobalObject(TiContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(jsCast<JSObject*>(exec->lexicalGlobalObject()->methodTable()->toThis(exec->lexicalGlobalObject(), exec, NotStrictMode)));
}

TiContextGroupRef TiContextGetGroup(TiContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    return toRef(&exec->vm());
}

TiGlobalContextRef TiContextGetGlobalContext(TiContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toGlobalRef(exec->lexicalGlobalObject()->globalExec());
}

TiStringRef TiGlobalContextCopyName(TiGlobalContextRef ctx)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }

    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    String name = exec->vmEntryGlobalObject()->name();
    if (name.isNull())
        return 0;

    return OpaqueTiString::create(name).leakRef();
}

void TiGlobalContextSetName(TiGlobalContextRef ctx, TiStringRef name)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    exec->vmEntryGlobalObject()->setName(name ? name->string() : String());
}


class BacktraceFunctor {
public:
    BacktraceFunctor(StringBuilder& builder, unsigned remainingCapacityForFrameCapture)
        : m_builder(builder)
        , m_remainingCapacityForFrameCapture(remainingCapacityForFrameCapture)
    {
    }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        if (m_remainingCapacityForFrameCapture) {
            // If callee is unknown, but we've not added any frame yet, we should
            // still add the frame, because something called us, and gave us arguments.
            JSObject* callee = visitor->callee();
            if (!callee && visitor->index())
                return StackVisitor::Done;

            StringBuilder& builder = m_builder;
            if (!builder.isEmpty())
                builder.append('\n');
            builder.append('#');
            builder.appendNumber(visitor->index());
            builder.append(' ');
            builder.append(visitor->functionName());
            builder.appendLiteral("() at ");
            builder.append(visitor->sourceURL());
            if (visitor->isJSFrame()) {
                builder.append(':');
                unsigned lineNumber;
                unsigned unusedColumn;
                visitor->computeLineAndColumn(lineNumber, unusedColumn);
                builder.appendNumber(lineNumber);
            }

            if (!callee)
                return StackVisitor::Done;

            m_remainingCapacityForFrameCapture--;
            return StackVisitor::Continue;
        }
        return StackVisitor::Done;
    }

private:
    StringBuilder& m_builder;
    unsigned m_remainingCapacityForFrameCapture;
};

TiStringRef TiContextCreateBacktrace(TiContextRef ctx, unsigned maxStackSize)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    JSLockHolder lock(exec);
    StringBuilder builder;
    CallFrame* frame = exec->vm().topCallFrame;

    ASSERT(maxStackSize);
    BacktraceFunctor functor(builder, maxStackSize);
    frame->iterate(functor);

    return OpaqueTiString::create(builder.toString()).leakRef();
}


