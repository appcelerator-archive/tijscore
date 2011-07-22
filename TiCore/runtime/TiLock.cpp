/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the NU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA 
 *
 */

#include "config.h"
#include "TiLock.h"

#include "Collector.h"
#include "CallFrame.h"

#if ENABLE(JSC_MULTIPLE_THREADS)
#include <pthread.h>
#endif

namespace TI {

#if ENABLE(JSC_MULTIPLE_THREADS)

// Acquire this mutex before accessing lock-related data.
static pthread_mutex_t JSMutex = PTHREAD_MUTEX_INITIALIZER;

// Thread-specific key that tells whether a thread holds the JSMutex, and how many times it was taken recursively.
pthread_key_t TiLockCount;

static void createTiLockCount()
{
    pthread_key_create(&TiLockCount, 0);
}

pthread_once_t createTiLockCountOnce = PTHREAD_ONCE_INIT;

// Lock nesting count.
intptr_t TiLock::lockCount()
{
    pthread_once(&createTiLockCountOnce, createTiLockCount);

    return reinterpret_cast<intptr_t>(pthread_getspecific(TiLockCount));
}

static void setLockCount(intptr_t count)
{
    ASSERT(count >= 0);
    pthread_setspecific(TiLockCount, reinterpret_cast<void*>(count));
}

TiLock::TiLock(TiExcState* exec)
    : m_lockBehavior(exec->globalData().isSharedInstance() ? LockForReal : SilenceAssertionsOnly)
{
    lock(m_lockBehavior);
}

void TiLock::lock(TiLockBehavior lockBehavior)
{
#ifdef NDEBUG
    // Locking "not for real" is a debug-only feature.
    if (lockBehavior == SilenceAssertionsOnly)
        return;
#endif

    pthread_once(&createTiLockCountOnce, createTiLockCount);

    intptr_t currentLockCount = lockCount();
    if (!currentLockCount && lockBehavior == LockForReal) {
        int result;
        result = pthread_mutex_lock(&JSMutex);
        ASSERT(!result);
    }
    setLockCount(currentLockCount + 1);
}

void TiLock::unlock(TiLockBehavior lockBehavior)
{
    ASSERT(lockCount());

#ifdef NDEBUG
    // Locking "not for real" is a debug-only feature.
    if (lockBehavior == SilenceAssertionsOnly)
        return;
#endif

    intptr_t newLockCount = lockCount() - 1;
    if (!newLockCount && lockBehavior == LockForReal) {
        int result;
        result = pthread_mutex_unlock(&JSMutex);
        ASSERT(!result);
    }
    setLockCount(newLockCount);
}

void TiLock::lock(TiExcState* exec)
{
    lock(exec->globalData().isSharedInstance() ? LockForReal : SilenceAssertionsOnly);
}

void TiLock::unlock(TiExcState* exec)
{
    unlock(exec->globalData().isSharedInstance() ? LockForReal : SilenceAssertionsOnly);
}

bool TiLock::currentThreadIsHoldingLock()
{
    pthread_once(&createTiLockCountOnce, createTiLockCount);
    return !!pthread_getspecific(TiLockCount);
}

// This is fairly nasty.  We allow multiple threads to run on the same
// context, and we do not require any locking semantics in doing so -
// clients of the API may simply use the context from multiple threads
// concurently, and assume this will work.  In order to make this work,
// We lock the context when a thread enters, and unlock it when it leaves.
// However we do not only unlock when the thread returns from its
// entry point (evaluate script or call function), we also unlock the
// context if the thread leaves JSC by making a call out to an external
// function through a callback.
//
// All threads using the context share the same JS stack (the RegisterFile).
// Whenever a thread calls into JSC it starts using the RegisterFile from the
// previous 'high water mark' - the maximum point the stack has ever grown to
// (returned by RegisterFile::end()).  So if a first thread calls out to a
// callback, and a second thread enters JSC, then also exits by calling out
// to a callback, we can be left with stackframes from both threads in the
// RegisterFile.  As such, a problem may occur should the first thread's
// callback complete first, and attempt to return to JSC.  Were we to allow
// this to happen, and were its stack to grow further, then it may potentially
// write over the second thread's call frames.
//
// In avoid JS stack corruption we enforce a policy of only ever allowing two
// threads to use a JS context concurrently, and only allowing the second of
// these threads to execute until it has completed and fully returned from its
// outermost call into JSC.  We enforce this policy using 'lockDropDepth'.  The
// first time a thread exits it will call DropAllLocks - which will do as expected
// and drop locks allowing another thread to enter.  Should another thread, or the
// same thread again, enter JSC (through evaluate script or call function), and exit
// again through a callback, then the locks will not be dropped when DropAllLocks
// is called (since lockDropDepth is non-zero).  Since this thread is still holding
// the locks, only it will re able to re-enter JSC (either be returning from the
// callback, or by re-entering through another call to evaulate script or call
// function).
//
// This policy is slightly more restricive than it needs to be for correctness -
// we could validly allow futher entries into JSC from other threads, we only
// need ensure that callbacks return in the reverse chronological order of the
// order in which they were made - though implementing the less restrictive policy
// would likely increase complexity and overhead.
//
static unsigned lockDropDepth = 0;

TiLock::DropAllLocks::DropAllLocks(TiExcState* exec)
    : m_lockBehavior(exec->globalData().isSharedInstance() ? LockForReal : SilenceAssertionsOnly)
{
    pthread_once(&createTiLockCountOnce, createTiLockCount);

    if (lockDropDepth++) {
        m_lockCount = 0;
        return;
    }

    m_lockCount = TiLock::lockCount();
    for (intptr_t i = 0; i < m_lockCount; i++)
        TiLock::unlock(m_lockBehavior);
}

TiLock::DropAllLocks::DropAllLocks(TiLockBehavior TiLockBehavior)
    : m_lockBehavior(TiLockBehavior)
{
    pthread_once(&createTiLockCountOnce, createTiLockCount);

    if (lockDropDepth++) {
        m_lockCount = 0;
        return;
    }

    // It is necessary to drop even "unreal" locks, because having a non-zero lock count
    // will prevent a real lock from being taken.

    m_lockCount = TiLock::lockCount();
    for (intptr_t i = 0; i < m_lockCount; i++)
        TiLock::unlock(m_lockBehavior);
}

TiLock::DropAllLocks::~DropAllLocks()
{
    for (intptr_t i = 0; i < m_lockCount; i++)
        TiLock::lock(m_lockBehavior);

    --lockDropDepth;
}

#else

TiLock::TiLock(TiExcState*)
    : m_lockBehavior(SilenceAssertionsOnly)
{
}

// If threading support is off, set the lock count to a constant value of 1 so ssertions
// that the lock is held don't fail
intptr_t TiLock::lockCount()
{
    return 1;
}

bool TiLock::currentThreadIsHoldingLock()
{
    return true;
}

void TiLock::lock(TiLockBehavior)
{
}

void TiLock::unlock(TiLockBehavior)
{
}

void TiLock::lock(TiExcState*)
{
}

void TiLock::unlock(TiExcState*)
{
}

TiLock::DropAllLocks::DropAllLocks(TiExcState*)
{
}

TiLock::DropAllLocks::DropAllLocks(TiLockBehavior)
{
}

TiLock::DropAllLocks::~DropAllLocks()
{
}

#endif // USE(MULTIPLE_THREADS)

} // namespace TI
