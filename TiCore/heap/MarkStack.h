/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MarkStack_h
#define MarkStack_h

#include "HandleTypes.h"
#include "TiValue.h"
#include "Register.h"
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/Noncopyable.h>
#include <wtf/OSAllocator.h>

namespace TI {

    class ConservativeRoots;
    class TiGlobalData;
    class Register;
    template<typename T> class WriteBarrierBase;
    template<typename T> class JITWriteBarrier;
    
    enum MarkSetProperties { MayContainNullValues, NoNullValues };
    
    class MarkStack {
        WTF_MAKE_NONCOPYABLE(MarkStack);
    public:
        MarkStack(void* jsArrayVPtr)
            : m_jsArrayVPtr(jsArrayVPtr)
            , m_shouldUnlinkCalls(false)
#if !ASSERT_DISABLED
            , m_isCheckingForDefaultMarkViolation(false)
            , m_isDraining(false)
#endif
        {
        }

        ~MarkStack()
        {
            ASSERT(m_markSets.isEmpty());
            ASSERT(m_values.isEmpty());
        }

        template<typename T> inline void append(JITWriteBarrier<T>*);
        template<typename T> inline void append(WriteBarrierBase<T>*);
        inline void appendValues(WriteBarrierBase<Unknown>*, size_t count, MarkSetProperties = NoNullValues);
        
        void append(ConservativeRoots&);

        bool addOpaqueRoot(void* root) { return m_opaqueRoots.add(root).second; }
        bool containsOpaqueRoot(void* root) { return m_opaqueRoots.contains(root); }
        int opaqueRootCount() { return m_opaqueRoots.size(); }

        void drain();
        void reset();

        bool shouldUnlinkCalls() const { return m_shouldUnlinkCalls; }
        void setShouldUnlinkCalls(bool shouldUnlinkCalls) { m_shouldUnlinkCalls = shouldUnlinkCalls; }

    private:
        friend class HeapRootVisitor; // Allowed to mark a TiValue* or TiCell** directly.

#if ENABLE(GC_VALIDATION)
        static void validateSet(TiValue*, size_t);
        static void validateValue(TiValue);
#endif

        void append(TiValue*);
        void append(TiValue*, size_t count);
        void append(TiCell**);

        void internalAppend(TiCell*);
        void internalAppend(TiValue);
        void visitChildren(TiCell*);

        struct MarkSet {
            MarkSet(TiValue* values, TiValue* end, MarkSetProperties properties)
                : m_values(values)
                , m_end(end)
                , m_properties(properties)
            {
                ASSERT(values);
            }
            TiValue* m_values;
            TiValue* m_end;
            MarkSetProperties m_properties;
        };

        static void* allocateStack(size_t size) { return OSAllocator::reserveAndCommit(size); }
        static void releaseStack(void* addr, size_t size) { OSAllocator::decommitAndRelease(addr, size); }

        static void initializePagesize();
        static size_t pageSize()
        {
            if (!s_pageSize)
                initializePagesize();
            return s_pageSize;
        }

        template<typename T> struct MarkStackArray {
            MarkStackArray()
                : m_top(0)
                , m_allocated(MarkStack::pageSize())
                , m_capacity(m_allocated / sizeof(T))
            {
                m_data = reinterpret_cast<T*>(allocateStack(m_allocated));
            }

            ~MarkStackArray()
            {
                releaseStack(m_data, m_allocated);
            }

            void expand()
            {
                size_t oldAllocation = m_allocated;
                m_allocated *= 2;
                m_capacity = m_allocated / sizeof(T);
                void* newData = allocateStack(m_allocated);
                memcpy(newData, m_data, oldAllocation);
                releaseStack(m_data, oldAllocation);
                m_data = reinterpret_cast<T*>(newData);
            }

            inline void append(const T& v)
            {
                if (m_top == m_capacity)
                    expand();
                m_data[m_top++] = v;
            }

            inline T removeLast()
            {
                ASSERT(m_top);
                return m_data[--m_top];
            }
            
            inline T& last()
            {
                ASSERT(m_top);
                return m_data[m_top - 1];
            }

            inline bool isEmpty()
            {
                return m_top == 0;
            }

            inline size_t size() { return m_top; }

            inline void shrinkAllocation(size_t size)
            {
                ASSERT(size <= m_allocated);
                ASSERT(0 == (size % MarkStack::pageSize()));
                if (size == m_allocated)
                    return;
#if OS(WINDOWS) || OS(SYMBIAN) || PLATFORM(BREWMP)
                // We cannot release a part of a region with VirtualFree.  To get around this,
                // we'll release the entire region and reallocate the size that we want.
                releaseStack(m_data, m_allocated);
                m_data = reinterpret_cast<T*>(allocateStack(size));
#else
                releaseStack(reinterpret_cast<char*>(m_data) + size, m_allocated - size);
#endif
                m_allocated = size;
                m_capacity = m_allocated / sizeof(T);
            }

        private:
            size_t m_top;
            size_t m_allocated;
            size_t m_capacity;
            T* m_data;
        };

        void* m_jsArrayVPtr;
        MarkStackArray<MarkSet> m_markSets;
        MarkStackArray<TiCell*> m_values;
        static size_t s_pageSize;
        HashSet<void*> m_opaqueRoots; // Handle-owning data structures not visible to the garbage collector.
        bool m_shouldUnlinkCalls;

#if !ASSERT_DISABLED
    public:
        bool m_isCheckingForDefaultMarkViolation;
        bool m_isDraining;
#endif
    };

    typedef MarkStack SlotVisitor;

    inline void MarkStack::append(TiValue* slot, size_t count)
    {
        if (!count)
            return;
#if ENABLE(GC_VALIDATION)
        validateSet(slot, count);
#endif
        m_markSets.append(MarkSet(slot, slot + count, NoNullValues));
    }
    
    ALWAYS_INLINE void MarkStack::append(TiValue* value)
    {
        ASSERT(value);
        internalAppend(*value);
    }

    ALWAYS_INLINE void MarkStack::append(TiCell** value)
    {
        ASSERT(value);
        internalAppend(*value);
    }

    ALWAYS_INLINE void MarkStack::internalAppend(TiValue value)
    {
        ASSERT(value);
#if ENABLE(GC_VALIDATION)
        validateValue(value);
#endif
        if (value.isCell())
            internalAppend(value.asCell());
    }

    // Privileged class for marking TiValues directly. It is only safe to use
    // this class to mark direct heap roots that are marked during every GC pass.
    // All other references should be wrapped in WriteBarriers and marked through
    // the MarkStack.
    class HeapRootVisitor {
    private:
        friend class Heap;
        HeapRootVisitor(SlotVisitor&);
        
    public:
        void mark(TiValue*);
        void mark(TiValue*, size_t);
        void mark(TiString**);
        void mark(TiCell**);
        
        SlotVisitor& visitor();

    private:
        SlotVisitor& m_visitor;
    };

    inline HeapRootVisitor::HeapRootVisitor(SlotVisitor& visitor)
        : m_visitor(visitor)
    {
    }

    inline void HeapRootVisitor::mark(TiValue* slot)
    {
        m_visitor.append(slot);
    }

    inline void HeapRootVisitor::mark(TiValue* slot, size_t count)
    {
        m_visitor.append(slot, count);
    }

    inline void HeapRootVisitor::mark(TiString** slot)
    {
        m_visitor.append(reinterpret_cast<TiCell**>(slot));
    }

    inline void HeapRootVisitor::mark(TiCell** slot)
    {
        m_visitor.append(slot);
    }

    inline SlotVisitor& HeapRootVisitor::visitor()
    {
        return m_visitor;
    }

} // namespace TI

#endif
