/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2013 Apple Inc. All rights reserved.
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
#include "JSStackInlines.h"

#include "ConservativeRoots.h"
#include "Interpreter.h"

namespace TI {

static size_t committedBytesCount = 0;

static Mutex& stackStatisticsMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, staticMutex, ());
    return staticMutex;
}    

JSStack::JSStack(VM& vm, size_t capacity)
    : m_vm(vm)
    , m_end(0)
    , m_topCallFrame(vm.topCallFrame)
{
    ASSERT(capacity && isPageAligned(capacity));

    m_reservation = PageReservation::reserve(roundUpAllocationSize(capacity * sizeof(Register), commitSize), OSAllocator::JSVMStackPages);
    updateStackLimit(highAddress());
    m_commitEnd = highAddress();
    
    m_lastStackTop = getBaseOfStack();

    disableErrorStackReserve();

    m_topCallFrame = 0;
}

JSStack::~JSStack()
{
    void* highAddress = reinterpret_cast<void*>(static_cast<char*>(m_reservation.base()) + m_reservation.size());
    m_reservation.decommit(reinterpret_cast<void*>(m_commitEnd), reinterpret_cast<intptr_t>(highAddress) - reinterpret_cast<intptr_t>(m_commitEnd));
    addToCommittedByteCount(-(reinterpret_cast<intptr_t>(highAddress) - reinterpret_cast<intptr_t>(m_commitEnd)));
    m_reservation.deallocate();
}

bool JSStack::growSlowCase(Register* newEnd)
{
    // If we have already committed enough memory to satisfy this request,
    // just update the end pointer and return.
    if (newEnd >= m_commitEnd) {
        updateStackLimit(newEnd);
        return true;
    }

    // Compute the chunk size of additional memory to commit, and see if we
    // have it is still within our budget. If not, we'll fail to grow and
    // return false.
    long delta = roundUpAllocationSize(reinterpret_cast<char*>(m_commitEnd) - reinterpret_cast<char*>(newEnd), commitSize);
    if (reinterpret_cast<char*>(m_commitEnd) - delta <= reinterpret_cast<char*>(m_useableEnd))
        return false;

    // Otherwise, the growth is still within our budget. Go ahead and commit
    // it and return true.
    m_reservation.commit(reinterpret_cast<char*>(m_commitEnd) - delta, delta);
    addToCommittedByteCount(delta);
    m_commitEnd = reinterpret_cast_ptr<Register*>(reinterpret_cast<char*>(m_commitEnd) - delta);
    updateStackLimit(newEnd);
    return true;
}

void JSStack::gatherConservativeRoots(ConservativeRoots& conservativeRoots)
{
    conservativeRoots.add(getBaseOfStack(), getTopOfStack());
}

void JSStack::gatherConservativeRoots(ConservativeRoots& conservativeRoots, JITStubRoutineSet& jitStubRoutines, CodeBlockSet& codeBlocks)
{
    conservativeRoots.add(getBaseOfStack(), getTopOfStack(), jitStubRoutines, codeBlocks);
}

void JSStack::sanitizeStack()
{
    ASSERT(getTopOfStack() <= getBaseOfStack());
    
    if (m_lastStackTop < getTopOfStack()) {
        char* begin = reinterpret_cast<char*>(m_lastStackTop);
        char* end = reinterpret_cast<char*>(getTopOfStack());
        memset(begin, 0, end - begin);
    }
    
    m_lastStackTop = getTopOfStack();
}

void JSStack::releaseExcessCapacity()
{
    ptrdiff_t delta = reinterpret_cast<uintptr_t>(highAddress()) - reinterpret_cast<uintptr_t>(m_commitEnd);
    m_reservation.decommit(m_commitEnd, delta);
    addToCommittedByteCount(-delta);
    m_commitEnd = highAddress();
}

void JSStack::initializeThreading()
{
    stackStatisticsMutex();
}

size_t JSStack::committedByteCount()
{
    MutexLocker locker(stackStatisticsMutex());
    return committedBytesCount;
}

void JSStack::addToCommittedByteCount(long byteCount)
{
    MutexLocker locker(stackStatisticsMutex());
    ASSERT(static_cast<long>(committedBytesCount) + byteCount > -1);
    committedBytesCount += byteCount;
}

void JSStack::enableErrorStackReserve()
{
    m_useableEnd = reservationEnd();
}

void JSStack::disableErrorStackReserve()
{
    char* useableEnd = reinterpret_cast<char*>(reservationEnd()) + commitSize;
    m_useableEnd = reinterpret_cast_ptr<Register*>(useableEnd);

    // By the time we get here, we are guaranteed to be destructing the last
    // Interpreter::ErrorHandlingMode that enabled this reserve in the first
    // place. That means the stack space beyond m_useableEnd before we
    // enabled the reserve was not previously in use. Hence, it is safe to
    // shrink back to that m_useableEnd.
    if (m_end < m_useableEnd) {
        ASSERT(m_topCallFrame->frameExtent() >= m_useableEnd);
        shrink(m_useableEnd);
    }
}

} // namespace TI
