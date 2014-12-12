/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "MarkedSpace.h"

#include "DelayedReleaseScope.h"
#include "IncrementalSweeper.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "JSObject.h"


namespace TI {

class Structure;

class Free {
public:
    typedef MarkedBlock* ReturnType;

    enum FreeMode { FreeOrShrink, FreeAll };

    Free(FreeMode, MarkedSpace*);
    void operator()(MarkedBlock*);
    ReturnType returnValue();
    
private:
    FreeMode m_freeMode;
    MarkedSpace* m_markedSpace;
    DoublyLinkedList<MarkedBlock> m_blocks;
};

inline Free::Free(FreeMode freeMode, MarkedSpace* newSpace)
    : m_freeMode(freeMode)
    , m_markedSpace(newSpace)
{
}

inline void Free::operator()(MarkedBlock* block)
{
    if (m_freeMode == FreeOrShrink)
        m_markedSpace->freeOrShrinkBlock(block);
    else
        m_markedSpace->freeBlock(block);
}

inline Free::ReturnType Free::returnValue()
{
    return m_blocks.head();
}

struct VisitWeakSet : MarkedBlock::VoidFunctor {
    VisitWeakSet(HeapRootVisitor& heapRootVisitor) : m_heapRootVisitor(heapRootVisitor) { }
    void operator()(MarkedBlock* block) { block->visitWeakSet(m_heapRootVisitor); }
private:
    HeapRootVisitor& m_heapRootVisitor;
};

struct ReapWeakSet : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->reapWeakSet(); }
};

MarkedSpace::MarkedSpace(Heap* heap)
    : m_heap(heap)
    , m_capacity(0)
    , m_isIterating(false)
    , m_currentDelayedReleaseScope(nullptr)
{
    for (size_t cellSize = preciseStep; cellSize <= preciseCutoff; cellSize += preciseStep) {
        allocatorFor(cellSize).init(heap, this, cellSize, MarkedBlock::None);
        normalDestructorAllocatorFor(cellSize).init(heap, this, cellSize, MarkedBlock::Normal);
        immortalStructureDestructorAllocatorFor(cellSize).init(heap, this, cellSize, MarkedBlock::ImmortalStructure);
    }

    for (size_t cellSize = impreciseStep; cellSize <= impreciseCutoff; cellSize += impreciseStep) {
        allocatorFor(cellSize).init(heap, this, cellSize, MarkedBlock::None);
        normalDestructorAllocatorFor(cellSize).init(heap, this, cellSize, MarkedBlock::Normal);
        immortalStructureDestructorAllocatorFor(cellSize).init(heap, this, cellSize, MarkedBlock::ImmortalStructure);
    }

    m_normalSpace.largeAllocator.init(heap, this, 0, MarkedBlock::None);
    m_normalDestructorSpace.largeAllocator.init(heap, this, 0, MarkedBlock::Normal);
    m_immortalStructureDestructorSpace.largeAllocator.init(heap, this, 0, MarkedBlock::ImmortalStructure);
}

MarkedSpace::~MarkedSpace()
{
    Free free(Free::FreeAll, this);
    forEachBlock(free);
}

struct LastChanceToFinalize : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->lastChanceToFinalize(); }
};

void MarkedSpace::lastChanceToFinalize()
{
    DelayedReleaseScope delayedReleaseScope(*this);
    stopAllocating();
    forEachBlock<LastChanceToFinalize>();
}

void MarkedSpace::sweep()
{
    m_heap->sweeper()->willFinishSweeping();
    forEachBlock<Sweep>();
}

void MarkedSpace::resetAllocators()
{
    for (size_t cellSize = preciseStep; cellSize <= preciseCutoff; cellSize += preciseStep) {
        allocatorFor(cellSize).reset();
        normalDestructorAllocatorFor(cellSize).reset();
        immortalStructureDestructorAllocatorFor(cellSize).reset();
    }

    for (size_t cellSize = impreciseStep; cellSize <= impreciseCutoff; cellSize += impreciseStep) {
        allocatorFor(cellSize).reset();
        normalDestructorAllocatorFor(cellSize).reset();
        immortalStructureDestructorAllocatorFor(cellSize).reset();
    }

    m_normalSpace.largeAllocator.reset();
    m_normalDestructorSpace.largeAllocator.reset();
    m_immortalStructureDestructorSpace.largeAllocator.reset();
}

void MarkedSpace::visitWeakSets(HeapRootVisitor& heapRootVisitor)
{
    VisitWeakSet visitWeakSet(heapRootVisitor);
    forEachBlock(visitWeakSet);
}

void MarkedSpace::reapWeakSets()
{
    forEachBlock<ReapWeakSet>();
}

template <typename Functor>
void MarkedSpace::forEachAllocator()
{
    Functor functor;
    forEachAllocator(functor);
}

template <typename Functor>
void MarkedSpace::forEachAllocator(Functor& functor)
{
    for (size_t cellSize = preciseStep; cellSize <= preciseCutoff; cellSize += preciseStep) {
        functor(allocatorFor(cellSize));
        functor(normalDestructorAllocatorFor(cellSize));
        functor(immortalStructureDestructorAllocatorFor(cellSize));
    }

    for (size_t cellSize = impreciseStep; cellSize <= impreciseCutoff; cellSize += impreciseStep) {
        functor(allocatorFor(cellSize));
        functor(normalDestructorAllocatorFor(cellSize));
        functor(immortalStructureDestructorAllocatorFor(cellSize));
    }

    functor(m_normalSpace.largeAllocator);
    functor(m_normalDestructorSpace.largeAllocator);
    functor(m_immortalStructureDestructorSpace.largeAllocator);
}

struct StopAllocatingFunctor {
    void operator()(MarkedAllocator& allocator) { allocator.stopAllocating(); }
};

void MarkedSpace::stopAllocating()
{
    ASSERT(!isIterating());
    forEachAllocator<StopAllocatingFunctor>();
}

struct ResumeAllocatingFunctor {
    void operator()(MarkedAllocator& allocator) { allocator.resumeAllocating(); }
};

void MarkedSpace::resumeAllocating()
{
    ASSERT(isIterating());
    DelayedReleaseScope scope(*this);
    forEachAllocator<ResumeAllocatingFunctor>();
}

bool MarkedSpace::isPagedOut(double deadline)
{
    for (size_t cellSize = preciseStep; cellSize <= preciseCutoff; cellSize += preciseStep) {
        if (allocatorFor(cellSize).isPagedOut(deadline) 
            || normalDestructorAllocatorFor(cellSize).isPagedOut(deadline) 
            || immortalStructureDestructorAllocatorFor(cellSize).isPagedOut(deadline))
            return true;
    }

    for (size_t cellSize = impreciseStep; cellSize <= impreciseCutoff; cellSize += impreciseStep) {
        if (allocatorFor(cellSize).isPagedOut(deadline) 
            || normalDestructorAllocatorFor(cellSize).isPagedOut(deadline) 
            || immortalStructureDestructorAllocatorFor(cellSize).isPagedOut(deadline))
            return true;
    }

    if (m_normalSpace.largeAllocator.isPagedOut(deadline)
        || m_normalDestructorSpace.largeAllocator.isPagedOut(deadline)
        || m_immortalStructureDestructorSpace.largeAllocator.isPagedOut(deadline))
        return true;

    return false;
}

void MarkedSpace::freeBlock(MarkedBlock* block)
{
    block->allocator()->removeBlock(block);
    m_capacity -= block->capacity();
    m_blocks.remove(block);
    if (block->capacity() == MarkedBlock::blockSize) {
        m_heap->blockAllocator().deallocate(MarkedBlock::destroy(block));
        return;
    }
    m_heap->blockAllocator().deallocateCustomSize(MarkedBlock::destroy(block));
}

void MarkedSpace::freeOrShrinkBlock(MarkedBlock* block)
{
    if (!block->isEmpty()) {
        block->shrink();
        return;
    }

    freeBlock(block);
}

struct Shrink : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->shrink(); }
};

void MarkedSpace::shrink()
{
    Free freeOrShrink(Free::FreeOrShrink, this);
    forEachBlock(freeOrShrink);
}

static void clearNewlyAllocatedInBlock(MarkedBlock* block)
{
    if (!block)
        return;
    block->clearNewlyAllocated();
}

struct ClearNewlyAllocated : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->clearNewlyAllocated(); }
};

#ifndef NDEBUG
struct VerifyNewlyAllocated : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { ASSERT(!block->clearNewlyAllocated()); }
};
#endif

void MarkedSpace::clearNewlyAllocated()
{
    for (size_t i = 0; i < preciseCount; ++i) {
        clearNewlyAllocatedInBlock(m_normalSpace.preciseAllocators[i].takeLastActiveBlock());
        clearNewlyAllocatedInBlock(m_normalDestructorSpace.preciseAllocators[i].takeLastActiveBlock());
        clearNewlyAllocatedInBlock(m_immortalStructureDestructorSpace.preciseAllocators[i].takeLastActiveBlock());
    }

    for (size_t i = 0; i < impreciseCount; ++i) {
        clearNewlyAllocatedInBlock(m_normalSpace.impreciseAllocators[i].takeLastActiveBlock());
        clearNewlyAllocatedInBlock(m_normalDestructorSpace.impreciseAllocators[i].takeLastActiveBlock());
        clearNewlyAllocatedInBlock(m_immortalStructureDestructorSpace.impreciseAllocators[i].takeLastActiveBlock());
    }

    // We have to iterate all of the blocks in the large allocators because they are
    // canonicalized as they are used up (see MarkedAllocator::tryAllocateHelper)
    // which creates the m_newlyAllocated bitmap.
    ClearNewlyAllocated functor;
    m_normalSpace.largeAllocator.forEachBlock(functor);
    m_normalDestructorSpace.largeAllocator.forEachBlock(functor);
    m_immortalStructureDestructorSpace.largeAllocator.forEachBlock(functor);

#ifndef NDEBUG
    VerifyNewlyAllocated verifyFunctor;
    forEachBlock(verifyFunctor);
#endif
}

void MarkedSpace::willStartIterating()
{
    ASSERT(!isIterating());
    stopAllocating();
    m_isIterating = true;
}

void MarkedSpace::didFinishIterating()
{
    ASSERT(isIterating());
    resumeAllocating();
    m_isIterating = false;
}

} // namespace TI
