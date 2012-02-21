/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2011 Apple Inc. All rights reserved.
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
#include "Heap.h"
#include "CommonIdentifiers.h"
#include "DebuggerActivation.h"
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
#include "JSZombie.h"
#include "Lexer.h"
#include "Lookup.h"
#include "Nodes.h"
#include "Parser.h"
#include "RegExpCache.h"
#include "RegExpObject.h"
#include "StrictEvalActivation.h"
#include <wtf/WTFThreadData.h>
#if ENABLE(REGEXP_TRACING)
#include "RegExp.h"
#endif


#if ENABLE(JSC_MULTIPLE_THREADS)
#include <wtf/Threading.h>
#endif

#if PLATFORM(MAC)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace WTI;

namespace {

using namespace TI;

class Recompiler {
public:
    void operator()(TiCell*);
};

inline void Recompiler::operator()(TiCell* cell)
{
    if (!cell->inherits(&TiFunction::s_info))
        return;
    TiFunction* function = asFunction(cell);
    if (function->executable()->isHostFunction())
        return;
    function->jsExecutable()->discardCode();
}

} // namespace

namespace TI {

extern JSC_CONST_HASHTABLE HashTable arrayConstructorTable;
extern JSC_CONST_HASHTABLE HashTable arrayPrototypeTable;
extern JSC_CONST_HASHTABLE HashTable booleanPrototypeTable;
extern JSC_CONST_HASHTABLE HashTable jsonTable;
extern JSC_CONST_HASHTABLE HashTable dateTable;
extern JSC_CONST_HASHTABLE HashTable dateConstructorTable;
extern JSC_CONST_HASHTABLE HashTable errorPrototypeTable;
extern JSC_CONST_HASHTABLE HashTable globalObjectTable;
extern JSC_CONST_HASHTABLE HashTable mathTable;
extern JSC_CONST_HASHTABLE HashTable numberConstructorTable;
extern JSC_CONST_HASHTABLE HashTable numberPrototypeTable;
extern JSC_CONST_HASHTABLE HashTable objectConstructorTable;
extern JSC_CONST_HASHTABLE HashTable objectPrototypeTable;
extern JSC_CONST_HASHTABLE HashTable regExpTable;
extern JSC_CONST_HASHTABLE HashTable regExpConstructorTable;
extern JSC_CONST_HASHTABLE HashTable regExpPrototypeTable;
extern JSC_CONST_HASHTABLE HashTable stringTable;
extern JSC_CONST_HASHTABLE HashTable stringConstructorTable;

void* TiGlobalData::jsArrayVPtr;
void* TiGlobalData::jsByteArrayVPtr;
void* TiGlobalData::jsStringVPtr;
void* TiGlobalData::jsFunctionVPtr;

#if COMPILER(GCC)
// Work around for gcc trying to coalesce our reads of the various cell vptrs
#define CLOBBER_MEMORY() do { \
    asm volatile ("" : : : "memory"); \
} while (false)
#else
#define CLOBBER_MEMORY() do { } while (false)
#endif

void TiGlobalData::storeVPtrs()
{
    // Enough storage to fit a TiArray, TiArrayArray, TiString, or TiFunction.
    // COMPILE_ASSERTS below check that this is true.
    char storage[64];

    COMPILE_ASSERT(sizeof(TiArray) <= sizeof(storage), sizeof_TiArray_must_be_less_than_storage);
    TiCell* jsArray = new (storage) TiArray(TiArray::VPtrStealingHack);
    CLOBBER_MEMORY();
    TiGlobalData::jsArrayVPtr = jsArray->vptr();

    COMPILE_ASSERT(sizeof(TiArrayArray) <= sizeof(storage), sizeof_TiArrayArray_must_be_less_than_storage);
    TiCell* jsByteArray = new (storage) TiArrayArray(TiArrayArray::VPtrStealingHack);
    CLOBBER_MEMORY();
    TiGlobalData::jsByteArrayVPtr = jsByteArray->vptr();

    COMPILE_ASSERT(sizeof(TiString) <= sizeof(storage), sizeof_TiString_must_be_less_than_storage);
    TiCell* jsString = new (storage) TiString(TiString::VPtrStealingHack);
    CLOBBER_MEMORY();
    TiGlobalData::jsStringVPtr = jsString->vptr();

    COMPILE_ASSERT(sizeof(TiFunction) <= sizeof(storage), sizeof_TiFunction_must_be_less_than_storage);
    TiCell* jsFunction = new (storage) TiFunction(TiCell::VPtrStealingHack);
    CLOBBER_MEMORY();
    TiGlobalData::jsFunctionVPtr = jsFunction->vptr();
}

TiGlobalData::TiGlobalData(GlobalDataType globalDataType, ThreadStackType threadStackType)
    : globalDataType(globalDataType)
    , clientData(0)
    , arrayConstructorTable(fastNew<HashTable>(TI::arrayConstructorTable))
    , arrayPrototypeTable(fastNew<HashTable>(TI::arrayPrototypeTable))
    , booleanPrototypeTable(fastNew<HashTable>(TI::booleanPrototypeTable))
    , dateTable(fastNew<HashTable>(TI::dateTable))
    , dateConstructorTable(fastNew<HashTable>(TI::dateConstructorTable))
    , errorPrototypeTable(fastNew<HashTable>(TI::errorPrototypeTable))
    , globalObjectTable(fastNew<HashTable>(TI::globalObjectTable))
    , jsonTable(fastNew<HashTable>(TI::jsonTable))
    , mathTable(fastNew<HashTable>(TI::mathTable))
    , numberConstructorTable(fastNew<HashTable>(TI::numberConstructorTable))
    , numberPrototypeTable(fastNew<HashTable>(TI::numberPrototypeTable))
    , objectConstructorTable(fastNew<HashTable>(TI::objectConstructorTable))
    , objectPrototypeTable(fastNew<HashTable>(TI::objectPrototypeTable))
    , regExpTable(fastNew<HashTable>(TI::regExpTable))
    , regExpConstructorTable(fastNew<HashTable>(TI::regExpConstructorTable))
    , regExpPrototypeTable(fastNew<HashTable>(TI::regExpPrototypeTable))
    , stringTable(fastNew<HashTable>(TI::stringTable))
    , stringConstructorTable(fastNew<HashTable>(TI::stringConstructorTable))
    , identifierTable(globalDataType == Default ? wtfThreadData().currentIdentifierTable() : createIdentifierTable())
    , propertyNames(new CommonIdentifiers(this))
    , emptyList(new MarkedArgumentBuffer)
#if ENABLE(ASSEMBLER)
    , executableAllocator(*this)
    , regexAllocator(*this)
#endif
    , lexer(new Lexer(this))
    , parser(new Parser)
    , interpreter(0)
    , heap(this)
    , globalObjectCount(0)
    , dynamicGlobalObject(0)
    , cachedUTCOffset(NaN)
    , maxReentryDepth(threadStackType == ThreadStackTypeSmall ? MaxSmallThreadReentryDepth : MaxLargeThreadReentryDepth)
    , m_regExpCache(new RegExpCache(this))
#if ENABLE(REGEXP_TRACING)
    , m_rtTraceList(new RTTraceList())
#endif
#ifndef NDEBUG
    , exclusiveThread(0)
#endif
{
    interpreter = new Interpreter(*this);
    if (globalDataType == Default)
        m_stack = wtfThreadData().stack();

    // Need to be careful to keep everything consistent here
    IdentifierTable* existingEntryIdentifierTable = wtfThreadData().setCurrentIdentifierTable(identifierTable);
    TiLock lock(SilenceAssertionsOnly);
    structureStructure.set(*this, Structure::createStructure(*this));
    debuggerActivationStructure.set(*this, DebuggerActivation::createStructure(*this, jsNull()));
    activationStructure.set(*this, JSActivation::createStructure(*this, jsNull()));
    interruptedExecutionErrorStructure.set(*this, JSNonFinalObject::createStructure(*this, jsNull()));
    terminatedExecutionErrorStructure.set(*this, JSNonFinalObject::createStructure(*this, jsNull()));
    staticScopeStructure.set(*this, TiStaticScopeObject::createStructure(*this, jsNull()));
    strictEvalActivationStructure.set(*this, StrictEvalActivation::createStructure(*this, jsNull()));
    stringStructure.set(*this, TiString::createStructure(*this, jsNull()));
    notAnObjectStructure.set(*this, JSNotAnObject::createStructure(*this, jsNull()));
    propertyNameIteratorStructure.set(*this, TiPropertyNameIterator::createStructure(*this, jsNull()));
    getterSetterStructure.set(*this, GetterSetter::createStructure(*this, jsNull()));
    apiWrapperStructure.set(*this, TiAPIValueWrapper::createStructure(*this, jsNull()));
    scopeChainNodeStructure.set(*this, ScopeChainNode::createStructure(*this, jsNull()));
    executableStructure.set(*this, ExecutableBase::createStructure(*this, jsNull()));
    nativeExecutableStructure.set(*this, NativeExecutable::createStructure(*this, jsNull()));
    evalExecutableStructure.set(*this, EvalExecutable::createStructure(*this, jsNull()));
    programExecutableStructure.set(*this, ProgramExecutable::createStructure(*this, jsNull()));
    functionExecutableStructure.set(*this, FunctionExecutable::createStructure(*this, jsNull()));
    dummyMarkableCellStructure.set(*this, TiCell::createDummyStructure(*this));
    regExpStructure.set(*this, RegExp::createStructure(*this, jsNull()));
    structureChainStructure.set(*this, StructureChain::createStructure(*this, jsNull()));

#if ENABLE(JSC_ZOMBIES)
    zombieStructure.set(*this, JSZombie::createStructure(*this, jsNull()));
#endif

    wtfThreadData().setCurrentIdentifierTable(existingEntryIdentifierTable);

#if ENABLE(JIT) && ENABLE(INTERPRETER)
#if USE(CF)
    CFStringRef canUseJITKey = CFStringCreateWithCString(0 , "TiCoreUseJIT", kCFStringEncodingMacRoman);
    CFBooleanRef canUseJIT = (CFBooleanRef)CFPreferencesCopyAppValue(canUseJITKey, kCFPreferencesCurrentApplication);
    if (canUseJIT) {
        m_canUseJIT = kCFBooleanTrue == canUseJIT;
        CFRelease(canUseJIT);
    } else {
      char* canUseJITString = getenv("TiCoreUseJIT");
      m_canUseJIT = !canUseJITString || atoi(canUseJITString);
    }
    CFRelease(canUseJITKey);
#elif OS(UNIX)
    char* canUseJITString = getenv("TiCoreUseJIT");
    m_canUseJIT = !canUseJITString || atoi(canUseJITString);
#else
    m_canUseJIT = true;
#endif
#endif
#if ENABLE(JIT)
#if ENABLE(INTERPRETER)
    if (m_canUseJIT)
        m_canUseJIT = executableAllocator.isValid();
#endif
    jitStubs = adoptPtr(new JITThunks(this));
#endif
}

void TiGlobalData::clearBuiltinStructures()
{
    structureStructure.clear();
    debuggerActivationStructure.clear();
    activationStructure.clear();
    interruptedExecutionErrorStructure.clear();
    terminatedExecutionErrorStructure.clear();
    staticScopeStructure.clear();
    strictEvalActivationStructure.clear();
    stringStructure.clear();
    notAnObjectStructure.clear();
    propertyNameIteratorStructure.clear();
    getterSetterStructure.clear();
    apiWrapperStructure.clear();
    scopeChainNodeStructure.clear();
    executableStructure.clear();
    nativeExecutableStructure.clear();
    evalExecutableStructure.clear();
    programExecutableStructure.clear();
    functionExecutableStructure.clear();
    dummyMarkableCellStructure.clear();
    regExpStructure.clear();
    structureChainStructure.clear();

#if ENABLE(JSC_ZOMBIES)
    zombieStructure.clear();
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

    arrayPrototypeTable->deleteTable();
    arrayConstructorTable->deleteTable();
    booleanPrototypeTable->deleteTable();
    dateTable->deleteTable();
    dateConstructorTable->deleteTable();
    errorPrototypeTable->deleteTable();
    globalObjectTable->deleteTable();
    jsonTable->deleteTable();
    mathTable->deleteTable();
    numberConstructorTable->deleteTable();
    numberPrototypeTable->deleteTable();
    objectConstructorTable->deleteTable();
    objectPrototypeTable->deleteTable();
    regExpTable->deleteTable();
    regExpConstructorTable->deleteTable();
    regExpPrototypeTable->deleteTable();
    stringTable->deleteTable();
    stringConstructorTable->deleteTable();

    fastDelete(const_cast<HashTable*>(arrayConstructorTable));
    fastDelete(const_cast<HashTable*>(arrayPrototypeTable));
    fastDelete(const_cast<HashTable*>(booleanPrototypeTable));
    fastDelete(const_cast<HashTable*>(dateTable));
    fastDelete(const_cast<HashTable*>(dateConstructorTable));
    fastDelete(const_cast<HashTable*>(errorPrototypeTable));
    fastDelete(const_cast<HashTable*>(globalObjectTable));
    fastDelete(const_cast<HashTable*>(jsonTable));
    fastDelete(const_cast<HashTable*>(mathTable));
    fastDelete(const_cast<HashTable*>(numberConstructorTable));
    fastDelete(const_cast<HashTable*>(numberPrototypeTable));
    fastDelete(const_cast<HashTable*>(objectConstructorTable));
    fastDelete(const_cast<HashTable*>(objectPrototypeTable));
    fastDelete(const_cast<HashTable*>(regExpTable));
    fastDelete(const_cast<HashTable*>(regExpConstructorTable));
    fastDelete(const_cast<HashTable*>(regExpPrototypeTable));
    fastDelete(const_cast<HashTable*>(stringTable));
    fastDelete(const_cast<HashTable*>(stringConstructorTable));

    delete parser;
    delete lexer;

    deleteAllValues(opaqueTiClassData);

    delete emptyList;

    delete propertyNames;
    if (globalDataType != Default)
        deleteIdentifierTable(identifierTable);

    delete clientData;
    delete m_regExpCache;
#if ENABLE(REGEXP_TRACING)
    delete m_rtTraceList;
#endif
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
    return create(type);
}

bool TiGlobalData::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

TiGlobalData& TiGlobalData::sharedInstance()
{
    TiGlobalData*& instance = sharedInstanceInternal();
    if (!instance) {
        instance = adoptRef(new TiGlobalData(APIShared, ThreadStackTypeSmall)).leakRef();
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

#if ENABLE(JIT)
NativeExecutable* TiGlobalData::getHostFunction(NativeFunction function)
{
    return jitStubs->hostFunctionStub(this, function);
}
NativeExecutable* TiGlobalData::getHostFunction(NativeFunction function, ThunkGenerator generator)
{
    return jitStubs->hostFunctionStub(this, function, generator);
}
#else
NativeExecutable* TiGlobalData::getHostFunction(NativeFunction function)
{
    return NativeExecutable::create(*this, function, callHostFunctionAsConstructor);
}
#endif

TiGlobalData::ClientData::~ClientData()
{
}

void TiGlobalData::resetDateCache()
{
    cachedUTCOffset = NaN;
    dstOffsetCache.reset();
    cachedDateString = UString();
    cachedDateStringValue = NaN;
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
    
    Recompiler recompiler;
    heap.forEach(recompiler);
}

struct StackPreservingRecompiler {
    HashSet<FunctionExecutable*> currentlyExecutingFunctions;
    void operator()(TiCell* cell)
    {
        if (!cell->inherits(&FunctionExecutable::s_info))
            return;
        FunctionExecutable* executable = static_cast<FunctionExecutable*>(cell);
        if (currentlyExecutingFunctions.contains(executable))
            return;
        executable->discardCode();
    }
};

void TiGlobalData::releaseExecutableMemory()
{
    if (dynamicGlobalObject) {
        StackPreservingRecompiler recompiler;
        HashSet<TiCell*> roots;
        heap.getConservativeRegisterRoots(roots);
        HashSet<TiCell*>::iterator end = roots.end();
        for (HashSet<TiCell*>::iterator ptr = roots.begin(); ptr != end; ++ptr) {
            ScriptExecutable* executable = 0;
            TiCell* cell = *ptr;
            if (cell->inherits(&ScriptExecutable::s_info))
                executable = static_cast<ScriptExecutable*>(*ptr);
            else if (cell->inherits(&TiFunction::s_info)) {
                TiFunction* function = asFunction(*ptr);
                if (function->isHostFunction())
                    continue;
                executable = function->jsExecutable();
            } else
                continue;
            ASSERT(executable->inherits(&ScriptExecutable::s_info));
            executable->unlinkCalls();
            if (executable->inherits(&FunctionExecutable::s_info))
                recompiler.currentlyExecutingFunctions.add(static_cast<FunctionExecutable*>(executable));
                
        }
        heap.forEach(recompiler);
    } else
        recompileAllTiFunctions();

    m_regExpCache->invalidateCode();
    heap.collectAllGarbage();
}

#if ENABLE(ASSEMBLER)
void releaseExecutableMemory(TiGlobalData& globalData)
{
    globalData.releaseExecutableMemory();
}
#endif

#if ENABLE(REGEXP_TRACING)
void TiGlobalData::addRegExpToTrace(RegExp* regExp)
{
    m_rtTraceList->add(regExp);
}

void TiGlobalData::dumpRegExpTrace()
{
    // The first RegExp object is ignored.  It is create by the RegExpPrototype ctor and not used.
    RTTraceList::iterator iter = ++m_rtTraceList->begin();
    
    if (iter != m_rtTraceList->end()) {
        printf("\nRegExp Tracing\n");
        printf("                                                            match()    matches\n");
        printf("Regular Expression                          JIT Address      calls      found\n");
        printf("----------------------------------------+----------------+----------+----------\n");
    
        unsigned reCount = 0;
    
        for (; iter != m_rtTraceList->end(); ++iter, ++reCount)
            (*iter)->printTraceData();

        printf("%d Regular Expressions\n", reCount);
    }
    
    m_rtTraceList->clear();
}
#else
void TiGlobalData::dumpRegExpTrace()
{
}
#endif

} // namespace TI
