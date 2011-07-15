/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TiGlobalData.h"

#include "ArgList.h"
#include "Collector.h"
#include "CollectorHeapIterator.h"
#include "CommonIdentifiers.h"
#include "FunctionConstructor.h"
#include "GetterSetter.h"
#include "Interpreter.h"
#include "JSActivation.h"
#include "TiAPIValueWrapper.h"
#include "TiArray.h"
#include "TiArrayArray.h"
#include "TiClassRef.h"
#include "TiFunction.h"
#include "TiLock.h"
#include "JSNotAnObject.h"
#include "TiPropertyNameIterator.h"
#include "TiStaticScopeObject.h"
#include "Lexer.h"
#include "Lookup.h"
#include "Nodes.h"
#include "Parser.h"
#include "RegExpCache.h"
#include <wtf/WTFThreadData.h>

#if ENABLE(JSC_MULTIPLE_THREADS)
#include <wtf/Threading.h>
#endif

#if PLATFORM(MAC)
#include "ProfilerServer.h"
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace WTI;

namespace TI {

extern JSC_CONST_HASHTABLE HashTable arrayTable;
extern JSC_CONST_HASHTABLE HashTable jsonTable;
extern JSC_CONST_HASHTABLE HashTable dateTable;
extern JSC_CONST_HASHTABLE HashTable mathTable;
extern JSC_CONST_HASHTABLE HashTable numberTable;
extern JSC_CONST_HASHTABLE HashTable regExpTable;
extern JSC_CONST_HASHTABLE HashTable regExpConstructorTable;
extern JSC_CONST_HASHTABLE HashTable stringTable;

void* TiGlobalData::jsArrayVPtr;
void* TiGlobalData::jsByteArrayVPtr;
void* TiGlobalData::jsStringVPtr;
void* TiGlobalData::jsFunctionVPtr;

void TiGlobalData::storeVPtrs()
{
    CollectorCell cell;
    void* storage = &cell;

    COMPILE_ASSERT(sizeof(TiArray) <= sizeof(CollectorCell), sizeof_TiArray_must_be_less_than_CollectorCell);
    TiCell* jsArray = new (storage) TiArray(TiArray::createStructure(jsNull()));
    TiGlobalData::jsArrayVPtr = jsArray->vptr();
    jsArray->~TiCell();

    COMPILE_ASSERT(sizeof(TiArrayArray) <= sizeof(CollectorCell), sizeof_TiArrayArray_must_be_less_than_CollectorCell);
    TiCell* jsByteArray = new (storage) TiArrayArray(TiArrayArray::VPtrStealingHack);
    TiGlobalData::jsByteArrayVPtr = jsByteArray->vptr();
    jsByteArray->~TiCell();

    COMPILE_ASSERT(sizeof(TiString) <= sizeof(CollectorCell), sizeof_TiString_must_be_less_than_CollectorCell);
    TiCell* jsString = new (storage) TiString(TiString::VPtrStealingHack);
    TiGlobalData::jsStringVPtr = jsString->vptr();
    jsString->~TiCell();

    COMPILE_ASSERT(sizeof(TiFunction) <= sizeof(CollectorCell), sizeof_TiFunction_must_be_less_than_CollectorCell);
    TiCell* jsFunction = new (storage) TiFunction(TiFunction::createStructure(jsNull()));
    TiGlobalData::jsFunctionVPtr = jsFunction->vptr();
    jsFunction->~TiCell();
}

TiGlobalData::TiGlobalData(GlobalDataType globalDataType, ThreadStackType threadStackType)
    : globalDataType(globalDataType)
    , clientData(0)
    , arrayTable(fastNew<HashTable>(TI::arrayTable))
    , dateTable(fastNew<HashTable>(TI::dateTable))
    , jsonTable(fastNew<HashTable>(TI::jsonTable))
    , mathTable(fastNew<HashTable>(TI::mathTable))
    , numberTable(fastNew<HashTable>(TI::numberTable))
    , regExpTable(fastNew<HashTable>(TI::regExpTable))
    , regExpConstructorTable(fastNew<HashTable>(TI::regExpConstructorTable))
    , stringTable(fastNew<HashTable>(TI::stringTable))
    , activationStructure(JSActivation::createStructure(jsNull()))
    , interruptedExecutionErrorStructure(TiObject::createStructure(jsNull()))
    , terminatedExecutionErrorStructure(TiObject::createStructure(jsNull()))
    , staticScopeStructure(TiStaticScopeObject::createStructure(jsNull()))
    , stringStructure(TiString::createStructure(jsNull()))
    , notAnObjectErrorStubStructure(JSNotAnObjectErrorStub::createStructure(jsNull()))
    , notAnObjectStructure(JSNotAnObject::createStructure(jsNull()))
    , propertyNameIteratorStructure(TiPropertyNameIterator::createStructure(jsNull()))
    , getterSetterStructure(GetterSetter::createStructure(jsNull()))
    , apiWrapperStructure(TiAPIValueWrapper::createStructure(jsNull()))
    , dummyMarkableCellStructure(TiCell::createDummyStructure())
#if USE(JSVALUE32)
    , numberStructure(JSNumberCell::createStructure(jsNull()))
#endif
    , identifierTable(globalDataType == Default ? wtfThreadData().currentIdentifierTable() : createIdentifierTable())
    , propertyNames(new CommonIdentifiers(this))
    , emptyList(new MarkedArgumentBuffer)
    , lexer(new Lexer(this))
    , parser(new Parser)
    , interpreter(new Interpreter)
    , heap(this)
    , initializingLazyNumericCompareFunction(false)
    , head(0)
    , dynamicGlobalObject(0)
    , functionCodeBlockBeingReparsed(0)
    , firstStringifierToMark(0)
    , markStack(jsArrayVPtr)
    , cachedUTCOffset(NaN)
    , maxReentryDepth(threadStackType == ThreadStackTypeSmall ? MaxSmallThreadReentryDepth : MaxLargeThreadReentryDepth)
    , m_regExpCache(new RegExpCache(this))
#ifndef NDEBUG
    , exclusiveThread(0)
#endif
{
#if PLATFORM(MAC)
    startProfilerServerIfNeeded();
#endif
#if ENABLE(JIT) && ENABLE(INTERPRETER)
#if PLATFORM(CF)
    CFStringRef canUseJITKey = CFStringCreateWithCString(0 , "TiCoreUseJIT", kCFStringEncodingMacRoman);
    CFBooleanRef canUseJIT = (CFBooleanRef)CFPreferencesCopyAppValue(canUseJITKey, kCFPreferencesCurrentApplication);
    if (canUseJIT) {
        m_canUseJIT = kCFBooleanTrue == canUseJIT;
        CFRelease(canUseJIT);
    } else
        m_canUseJIT = !getenv("TiCoreUseJIT");
    CFRelease(canUseJITKey);
#elif OS(UNIX)
    m_canUseJIT = !getenv("TiCoreUseJIT");
#else
    m_canUseJIT = true;
#endif
#endif
#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
    if (m_canUseJIT)
        m_canUseJIT = executableAllocator.isValid();
#endif
    jitStubs.set(new JITThunks(this));
#endif
}

TiGlobalData::~TiGlobalData()
{
    // By the time this is destroyed, heap.destroy() must already have been called.

    delete interpreter;
#ifndef NDEBUG
    // Zeroing out to make the behavior more predictable when someone attempts to use a deleted instance.
    interpreter = 0;
#endif

    arrayTable->deleteTable();
    dateTable->deleteTable();
    jsonTable->deleteTable();
    mathTable->deleteTable();
    numberTable->deleteTable();
    regExpTable->deleteTable();
    regExpConstructorTable->deleteTable();
    stringTable->deleteTable();

    fastDelete(const_cast<HashTable*>(arrayTable));
    fastDelete(const_cast<HashTable*>(dateTable));
    fastDelete(const_cast<HashTable*>(jsonTable));
    fastDelete(const_cast<HashTable*>(mathTable));
    fastDelete(const_cast<HashTable*>(numberTable));
    fastDelete(const_cast<HashTable*>(regExpTable));
    fastDelete(const_cast<HashTable*>(regExpConstructorTable));
    fastDelete(const_cast<HashTable*>(stringTable));

    delete parser;
    delete lexer;

    deleteAllValues(opaqueTiClassData);

    delete emptyList;

    delete propertyNames;
    if (globalDataType != Default)
        deleteIdentifierTable(identifierTable);

    delete clientData;
    delete m_regExpCache;
}

PassRefPtr<TiGlobalData> TiGlobalData::createContextGroup(ThreadStackType type)
{
    return adoptRef(new TiGlobalData(APIContextGroup, type));
}

PassRefPtr<TiGlobalData> TiGlobalData::create(ThreadStackType type)
{
    return adoptRef(new TiGlobalData(Default, type));
}

PassRefPtr<TiGlobalData> TiGlobalData::createLeaked(ThreadStackType type)
{
    Structure::startIgnoringLeaks();
    RefPtr<TiGlobalData> data = create(type);
    Structure::stopIgnoringLeaks();
    return data.release();
}

bool TiGlobalData::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

TiGlobalData& TiGlobalData::sharedInstance()
{
    TiGlobalData*& instance = sharedInstanceInternal();
    if (!instance) {
        instance = new TiGlobalData(APIShared, ThreadStackTypeSmall);
#if ENABLE(JSC_MULTIPLE_THREADS)
        instance->makeUsableFromMultipleThreads();
#endif
    }
    return *instance;
}

TiGlobalData*& TiGlobalData::sharedInstanceInternal()
{
    ASSERT(TiLock::currentThreadIsHoldingLock());
    static TiGlobalData* sharedInstance;
    return sharedInstance;
}

// FIXME: We can also detect forms like v1 < v2 ? -1 : 0, reverse comparison, etc.
const Vector<Instruction>& TiGlobalData::numericCompareFunction(TiExcState* exec)
{
    if (!lazyNumericCompareFunction.size() && !initializingLazyNumericCompareFunction) {
        initializingLazyNumericCompareFunction = true;
        RefPtr<FunctionExecutable> function = FunctionExecutable::fromGlobalCode(Identifier(exec, "numericCompare"), exec, 0, makeSource(UString("(function (v1, v2) { return v1 - v2; })")), 0, 0);
        lazyNumericCompareFunction = function->bytecode(exec, exec->scopeChain()).instructions();
        initializingLazyNumericCompareFunction = false;
    }

    return lazyNumericCompareFunction;
}

TiGlobalData::ClientData::~ClientData()
{
}

void TiGlobalData::resetDateCache()
{
    cachedUTCOffset = NaN;
    dstOffsetCache.reset();
    cachedDateString = UString();
    dateInstanceCache.reset();
}

void TiGlobalData::startSampling()
{
    interpreter->startSampling();
}

void TiGlobalData::stopSampling()
{
    interpreter->stopSampling();
}

void TiGlobalData::dumpSampleData(TiExcState* exec)
{
    interpreter->dumpSampleData(exec);
}

void TiGlobalData::recompileAllTiFunctions()
{
    // If Ti is running, it's not safe to recompile, since we'll end
    // up throwing away code that is live on the stack.
    ASSERT(!dynamicGlobalObject);

    LiveObjectIterator it = heap.primaryHeapBegin();
    LiveObjectIterator heapEnd = heap.primaryHeapEnd();
    for ( ; it != heapEnd; ++it) {
        if ((*it)->inherits(&TiFunction::info)) {
            TiFunction* function = asFunction(*it);
            if (!function->executable()->isHostFunction())
                function->jsExecutable()->recompile();
        }
    }
}

} // namespace TI
