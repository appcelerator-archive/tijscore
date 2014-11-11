/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

#include "config.h"
#include "MarkedAllocator.h"

#include "DelayedReleaseScope.h"
#include "GCActivityCallback.h"
#include "Heap.h"
#include "IncrementalSweeper.h"
#include "VM.h"
#include <wtf/CurrentTime.h>

namespace TI {

bool MarkedAllocator::isPagedOut(double deadline)
{
    unsigned itersSinceLastTimeCheck = 0;
    MarkedBlock* block = m_blockList.head();
    while (block) {
        block = block->next();
        ++itersSinceLastTimeCheck;
        if (itersSinceLastTimeCheck >= Heap::s_timeCheckResolution) {
            double currentTime = WTI::monotonicallyIncreasingTime();
            if (currentTime > deadline)
                return true;
            itersSinceLastTimeCheck = 0;
        }
    }

    return false;
}

inline void* MarkedAllocator::tryAllocateHelper(size_t bytes)
{
    // We need a while loop to check the free list because the DelayedReleaseScope 
    // could cause arbitrary code to execute and exhaust the free list that we 
    // thought had elements in it.
    while (!m_freeList.head) {
        DelayedReleaseScope delayedReleaseScope(*m_markedSpace);
        if (m_currentBlock) {
            ASSERT(m_currentBlock == m_blocksToSweep);
            m_currentBlock->didConsumeFreeList();
            m_blocksToSweep = m_currentBlock->next();
        }

        for (MarkedBlock*& block = m_blocksToSweep; block; block = block->next()) {
            MarkedBlock::FreeList freeList = block->sweep(MarkedBlock::SweepToFreeList);
            if (!freeList.head) {
                block->didConsumeEmptyFreeList();
                continue;
            }

            if (bytes > block->cellSize()) {
                block->stopAllocating(freeList);
                continue;
            }

            m_currentBlock = block;
            m_freeList = freeList;
            break;
        }
        
        if (!m_freeList.head) {
            m_currentBlock = 0;
            return 0;
        }
    }

    ASSERT(m_freeList.head);
    MarkedBlock::FreeCell* head = m_freeList.head;
    m_freeList.head = head->next;
    ASSERT(head);
    return head;
}
    
inline void* MarkedAllocator::tryAllocate(size_t bytes)
{
    ASSERT(!m_heap->isBusy());
    m_heap->m_operationInProgress = Allocation;
    void* result = tryAllocateHelper(bytes);
    m_heap->m_operationInProgress = NoOperation;
    return result;
}
    
void* MarkedAllocator::allocateSlowCase(size_t bytes)
{
    ASSERT(m_heap->vm()->currentThreadIsHoldingAPILock());
#if COLLECT_ON_EVERY_ALLOCATION
    if (!m_heap->isDeferred())
        m_heap->collectAllGarbage();
    ASSERT(m_heap->m_operationInProgress == NoOperation);
#endif
    
    ASSERT(!m_markedSpace->isIterating());
    ASSERT(!m_freeList.head);
    m_heap->didAllocate(m_freeList.bytes);
    
    void* result = tryAllocate(bytes);
    
    if (LIKELY(result != 0))
        return result;
    
    if (m_heap->collectIfNecessaryOrDefer()) {
        result = tryAllocate(bytes);
        if (result)
            return result;
    }

    ASSERT(!m_heap->shouldCollect());
    
    MarkedBlock* block = allocateBlock(bytes);
    ASSERT(block);
    addBlock(block);
        
    result = tryAllocate(bytes);
    ASSERT(result);
    return result;
}

MarkedBlock* MarkedAllocator::allocateBlock(size_t bytes)
{
    size_t minBlockSize = MarkedBlock::blockSize;
    size_t minAllocationSize = WTI::roundUpToMultipleOf(WTI::pageSize(), sizeof(MarkedBlock) + bytes);
    size_t blockSize = std::max(minBlockSize, minAllocationSize);

    size_t cellSize = m_cellSize ? m_cellSize : WTI::roundUpToMultipleOf<MarkedBlock::atomSize>(bytes);

    if (blockSize == MarkedBlock::blockSize)
        return MarkedBlock::create(m_heap->blockAllocator().allocate<MarkedBlock>(), this, cellSize, m_destructorType);
    return MarkedBlock::create(m_heap->blockAllocator().allocateCustomSize(blockSize, MarkedBlock::blockSize), this, cellSize, m_destructorType);
}

void MarkedAllocator::addBlock(MarkedBlock* block)
{
    // Satisfy the ASSERT in MarkedBlock::sweep.
    DelayedReleaseScope delayedReleaseScope(*m_markedSpace);
    ASSERT(!m_currentBlock);
    ASSERT(!m_freeList.head);
    
    m_blockList.append(block);
    m_blocksToSweep = m_currentBlock = block;
    m_freeList = block->sweep(MarkedBlock::SweepToFreeList);
    m_markedSpace->didAddBlock(block);
}

void MarkedAllocator::removeBlock(MarkedBlock* block)
{
    if (m_currentBlock == block) {
        m_currentBlock = m_currentBlock->next();
        m_freeList = MarkedBlock::FreeList();
    }
    if (m_blocksToSweep == block)
        m_blocksToSweep = m_blocksToSweep->next();
    m_blockList.remove(block);
}

} // namespace TI
