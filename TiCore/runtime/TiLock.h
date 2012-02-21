/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2005, 2008, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef TiLock_h
#define TiLock_h

#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>

namespace TI {

    // To make it safe to use Ti on multiple threads, it is
    // important to lock before doing anything that allocates a
    // Ti data structure or that interacts with shared state
    // such as the protect count hash table. The simplest way to lock
    // is to create a local TiLock object in the scope where the lock 
    // must be held. The lock is recursive so nesting is ok. The TiLock 
    // object also acts as a convenience short-hand for running important
    // initialization routines.

    // To avoid deadlock, sometimes it is necessary to temporarily
    // release the lock. Since it is recursive you actually have to
    // release all locks held by your thread. This is safe to do if
    // you are executing code that doesn't require the lock, and you
    // reacquire the right number of locks at the end. You can do this
    // by constructing a locally scoped TiLock::DropAllLocks object. The 
    // DropAllLocks object takes care to release the TiLock only if your
    // thread acquired it to begin with.

    // For contexts other than the single shared one, implicit locking is not done,
    // but we still need to perform all the counting in order to keep debug
    // assertions working, so that clients that use the shared context don't break.

    class TiExcState;
    class TiGlobalData;

    enum TiLockBehavior { SilenceAssertionsOnly, LockForReal };

    class TiLock {
        WTF_MAKE_NONCOPYABLE(TiLock);
    public:
        TiLock(TiExcState*);
        TiLock(TiGlobalData*);

        TiLock(TiLockBehavior lockBehavior)
            : m_lockBehavior(lockBehavior)
        {
#ifdef NDEBUG
            // Locking "not for real" is a debug-only feature.
            if (lockBehavior == SilenceAssertionsOnly)
                return;
#endif
            lock(lockBehavior);
        }

        ~TiLock()
        { 
#ifdef NDEBUG
            // Locking "not for real" is a debug-only feature.
            if (m_lockBehavior == SilenceAssertionsOnly)
                return;
#endif
            unlock(m_lockBehavior); 
        }
        
        static void lock(TiLockBehavior);
        static void unlock(TiLockBehavior);
        static void lock(TiExcState*);
        static void unlock(TiExcState*);

        static intptr_t lockCount();
        static bool currentThreadIsHoldingLock();

        TiLockBehavior m_lockBehavior;

        class DropAllLocks {
            WTF_MAKE_NONCOPYABLE(DropAllLocks);
        public:
            DropAllLocks(TiExcState* exec);
            DropAllLocks(TiLockBehavior);
            ~DropAllLocks();
            
        private:
            intptr_t m_lockCount;
            TiLockBehavior m_lockBehavior;
        };
    };

} // namespace

#endif // TiLock_h
