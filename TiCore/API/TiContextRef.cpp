/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
#include "InitializeThreading.h"
#include <interpreter/CallFrame.h>
#include <interpreter/Interpreter.h>
#include "TiCallbackObject.h"
#include "TiClassRef.h"
#include "TiGlobalObject.h"
#include "TiObject.h"
#include "UStringBuilder.h"
#include <wtf/text/StringHash.h>


#if OS(DARWIN)
#include <mach-o/dyld.h>

static const int32_t webkitFirstVersionWithConcurrentGlobalContexts = 0x2100500; // 528.5.0
#endif

using namespace TI;

TiContextGroupRef TiContextGroupCreate()
{
    initializeThreading();
    return toRef(TiGlobalData::createContextGroup(ThreadStackTypeSmall).leakRef());
}

TiContextGroupRef TiContextGroupRetain(TiContextGroupRef group)
{
    toJS(group)->ref();
    return group;
}

void TiContextGroupRelease(TiContextGroupRef group)
{
    toJS(group)->deref();
}

TiGlobalContextRef TiGlobalContextCreate(TiClassRef globalObjectClass)
{
    initializeThreading();
#if OS(DARWIN)
    // When running on Tiger or Leopard, or if the application was linked before TiGlobalContextCreate was changed
    // to use a unique TiGlobalData, we use a shared one for compatibility.
#ifndef BUILDING_ON_LEOPARD
    if (NSVersionOfLinkTimeLibrary("TiCore") <= webkitFirstVersionWithConcurrentGlobalContexts) {
#else
    {
#endif
        TiLock lock(LockForReal);
        return TiGlobalContextCreateInGroup(toRef(&TiGlobalData::sharedInstance()), globalObjectClass);
    }
#endif // OS(DARWIN)

    return TiGlobalContextCreateInGroup(0, globalObjectClass);
}

TiGlobalContextRef TiGlobalContextCreateInGroup(TiContextGroupRef group, TiClassRef globalObjectClass)
{
    initializeThreading();

    TiLock lock(LockForReal);
    RefPtr<TiGlobalData> globalData = group ? PassRefPtr<TiGlobalData>(toJS(group)) : TiGlobalData::createContextGroup(ThreadStackTypeSmall);

    APIEntryShim entryShim(globalData.get(), false);

#if ENABLE(JSC_MULTIPLE_THREADS)
    globalData->makeUsableFromMultipleThreads();
#endif

    if (!globalObjectClass) {
        TiGlobalObject* globalObject = new (globalData.get()) TiGlobalObject(*globalData, TiGlobalObject::createStructure(*globalData, jsNull()));
        return TiGlobalContextRetain(toGlobalRef(globalObject->globalExec()));
    }

    TiGlobalObject* globalObject = new (globalData.get()) TiCallbackObject<TiGlobalObject>(*globalData, globalObjectClass, TiCallbackObject<TiGlobalObject>::createStructure(*globalData, jsNull()));
    TiExcState* exec = globalObject->globalExec();
    TiValue prototype = globalObjectClass->prototype(exec);
    if (!prototype)
        prototype = jsNull();
    globalObject->resetPrototype(*globalData, prototype);
    return TiGlobalContextRetain(toGlobalRef(exec));
}

TiGlobalContextRef TiGlobalContextRetain(TiGlobalContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    TiGlobalData& globalData = exec->globalData();
    gcProtect(exec->dynamicGlobalObject());
    globalData.ref();
    return ctx;
}

void TiGlobalContextRelease(TiGlobalContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    TiLock lock(exec);

    TiGlobalData& globalData = exec->globalData();
    TiGlobalObject* dgo = exec->dynamicGlobalObject();
    IdentifierTable* savedIdentifierTable = wtfThreadData().setCurrentIdentifierTable(globalData.identifierTable);

    // One reference is held by TiGlobalObject, another added by TiGlobalContextRetain().
    bool releasingContextGroup = globalData.refCount() == 2;
    bool releasingGlobalObject = Heap::heap(dgo)->unprotect(dgo);
    // If this is the last reference to a global data, it should also
    // be the only remaining reference to the global object too!
    ASSERT(!releasingContextGroup || releasingGlobalObject);

    // An API 'TiGlobalContextRef' retains two things - a global object and a
    // global data (or context group, in API terminology).
    // * If this is the last reference to any contexts in the given context group,
    //   call destroy on the heap (the global data is being  freed).
    // * If this was the last reference to the global object, then unprotecting
    //   it may release a lot of GC memory - tickle the activity callback to
    //   garbage collect soon.
    // * If there are more references remaining the the global object, then do nothing
    //   (specifically that is more protects, which we assume come from other TiGlobalContextRefs).
    if (releasingContextGroup) {
        globalData.clearBuiltinStructures();
        globalData.heap.destroy();
    } else if (releasingGlobalObject) {
        globalData.heap.activityCallback()->synchronize();
        (*globalData.heap.activityCallback())();
    }

    globalData.deref();

    wtfThreadData().setCurrentIdentifierTable(savedIdentifierTable);
}

TiObjectRef TiContextGetGlobalObject(TiContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    // It is necessary to call toThisObject to get the wrapper object when used with WebCore.
    return toRef(exec->lexicalGlobalObject()->toThisObject(exec));
}

TiContextGroupRef TiContextGetGroup(TiContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    return toRef(&exec->globalData());
}

TiGlobalContextRef TiContextGetGlobalContext(TiContextRef ctx)
{
    TiExcState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toGlobalRef(exec->lexicalGlobalObject()->globalExec());
}
    
TiStringRef TiContextCreateBacktrace(TiContextRef ctx, unsigned maxStackSize)
{
    TiExcState* exec = toJS(ctx);
    TiLock lock(exec);

    unsigned count = 0;
    UStringBuilder builder;
    CallFrame* callFrame = exec;
    UString functionName;
    if (exec->callee()) {
        if (asObject(exec->callee())->inherits(&InternalFunction::s_info)) {
            functionName = asInternalFunction(exec->callee())->name(exec);
            builder.append("#0 ");
            builder.append(functionName);
            builder.append("() ");
            count++;
        }
    }
    while (true) {
        ASSERT(callFrame);
        int signedLineNumber;
        intptr_t sourceID;
        UString urlString;
        TiValue function;
        
        UString levelStr = UString::number(count);
        
        exec->interpreter()->retrieveLastCaller(callFrame, signedLineNumber, sourceID, urlString, function);

        if (function)
            functionName = asFunction(function)->name(exec);
        else {
            // Caller is unknown, but if frame is empty we should still add the frame, because
            // something called us, and gave us arguments.
            if (count)
                break;
        }
        unsigned lineNumber = signedLineNumber >= 0 ? signedLineNumber : 0;
        if (!builder.isEmpty())
            builder.append("\n");
        builder.append("#");
        builder.append(levelStr);
        builder.append(" ");
        builder.append(functionName);
        builder.append("() at ");
        builder.append(urlString);
        builder.append(":");
        builder.append(UString::number(lineNumber));
        if (!function || ++count == maxStackSize)
            break;
        callFrame = callFrame->callerFrame();
    }
    return OpaqueTiString::create(builder.toUString()).leakRef();
}


