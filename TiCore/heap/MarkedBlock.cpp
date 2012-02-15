/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MarkedBlock.h"

#include "TiCell.h"
#include "TiObject.h"
#include "JSZombie.h"
#include "ScopeChain.h"

namespace TI {

MarkedBlock* MarkedBlock::create(TiGlobalData* globalData, size_t cellSize)
{
    PageAllocationAligned allocation = PageAllocationAligned::allocate(blockSize, blockSize, OSAllocator::JSGCHeapPages);
    if (!static_cast<bool>(allocation))
        CRASH();
    return new (allocation.base()) MarkedBlock(allocation, globalData, cellSize);
}

void MarkedBlock::destroy(MarkedBlock* block)
{
    for (size_t i = block->firstAtom(); i < block->m_endAtom; i += block->m_atomsPerCell)
        reinterpret_cast<TiCell*>(&block->atoms()[i])->~TiCell();
    block->m_allocation.deallocate();
}

MarkedBlock::MarkedBlock(const PageAllocationAligned& allocation, TiGlobalData* globalData, size_t cellSize)
    : m_nextAtom(firstAtom())
    , m_allocation(allocation)
    , m_heap(&globalData->heap)
    , m_prev(0)
    , m_next(0)
{
    m_atomsPerCell = (cellSize + atomSize - 1) / atomSize;
    m_endAtom = atomsPerBlock - m_atomsPerCell + 1;

    Structure* dummyMarkableCellStructure = globalData->dummyMarkableCellStructure.get();
    for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell)
        new (&atoms()[i]) TiCell(*globalData, dummyMarkableCellStructure, TiCell::CreatingEarlyCell);
}

void MarkedBlock::sweep()
{
    Structure* dummyMarkableCellStructure = m_heap->globalData()->dummyMarkableCellStructure.get();

    for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
        if (m_marks.get(i))
            continue;

        TiCell* cell = reinterpret_cast<TiCell*>(&atoms()[i]);
#if ENABLE(JSC_ZOMBIES)
        if (cell->structure() && cell->structure() != dummyMarkableCellStructure && !cell->isZombie()) {
            const ClassInfo* info = cell->classInfo();
            cell->~TiCell();
            new (cell) JSZombie(*m_heap->globalData(), info, m_heap->globalData()->zombieStructure.get());
            m_marks.set(i);
        }
#else
        cell->~TiCell();
        new (cell) TiCell(*m_heap->globalData(), dummyMarkableCellStructure);
#endif
    }
}

} // namespace TI
