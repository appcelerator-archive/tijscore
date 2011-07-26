/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef APIShims_h
#define APIShims_h

#include "CallFrame.h"
#include "TiLock.h"
#include <wtf/WTFThreadData.h>

namespace TI {

class APIEntryShimWithoutLock {
protected:
    APIEntryShimWithoutLock(TiGlobalData* globalData, bool registerThread)
        : m_globalData(globalData)
        , m_entryIdentifierTable(wtfThreadData().setCurrentIdentifierTable(globalData->identifierTable))
    {
        if (registerThread)
            globalData->heap.registerThread();
        m_globalData->timeoutChecker.start();
    }

    ~APIEntryShimWithoutLock()
    {
        m_globalData->timeoutChecker.stop();
        wtfThreadData().setCurrentIdentifierTable(m_entryIdentifierTable);
    }

private:
    TiGlobalData* m_globalData;
    IdentifierTable* m_entryIdentifierTable;
};

class APIEntryShim : public APIEntryShimWithoutLock {
public:
    // Normal API entry
    APIEntryShim(TiExcState* exec, bool registerThread = true)
        : APIEntryShimWithoutLock(&exec->globalData(), registerThread)
        , m_lock(exec)
    {
    }

    // TiPropertyNameAccumulator only has a globalData.
    APIEntryShim(TiGlobalData* globalData, bool registerThread = true)
        : APIEntryShimWithoutLock(globalData, registerThread)
        , m_lock(globalData->isSharedInstance() ? LockForReal : SilenceAssertionsOnly)
    {
    }

private:
    TiLock m_lock;
};

class APICallbackShim {
public:
    APICallbackShim(TiExcState* exec)
        : m_dropAllLocks(exec)
        , m_globalData(&exec->globalData())
    {
        wtfThreadData().resetCurrentIdentifierTable();
    }

    ~APICallbackShim()
    {
        wtfThreadData().setCurrentIdentifierTable(m_globalData->identifierTable);
    }

private:
    TiLock::DropAllLocks m_dropAllLocks;
    TiGlobalData* m_globalData;
};

}

#endif
