/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef HandleStack_h
#define HandleStack_h

#include <wtf/Assertions.h>
#include <wtf/BlockStack.h>
#include "Handle.h"

namespace TI {

class LocalScope;
class HeapRootVisitor;

class HandleStack {
public:
    class Frame {
    public:
        HandleSlot m_next;
        HandleSlot m_end;
    };

    HandleStack();
    
    void enterScope(Frame&);
    void leaveScope(Frame&);

    HandleSlot push();

    void visit(HeapRootVisitor&);

private:
    void grow();
    void zapTo(Frame&);
    HandleSlot findFirstAfter(HandleSlot);

#ifndef NDEBUG
    size_t m_scopeDepth;
#endif
    BlockStack<TiValue> m_blockStack;
    Frame m_frame;
};

inline void HandleStack::enterScope(Frame& lastFrame)
{
#ifndef NDEBUG
    ++m_scopeDepth;
#endif

    lastFrame = m_frame;
}



inline void HandleStack::zapTo(Frame& lastFrame)
{
#ifdef NDEBUG
    UNUSED_PARAM(lastFrame);
#else
    const Vector<HandleSlot>& blocks = m_blockStack.blocks();
    
    if (lastFrame.m_end != m_frame.m_end) { // Zapping to a frame in a different block.
        int i = blocks.size() - 1;
        for ( ; blocks[i] + m_blockStack.blockLength != lastFrame.m_end; --i) {
            for (int j = m_blockStack.blockLength - 1; j >= 0; --j)
                blocks[i][j] = TiValue();
        }
        
        for (HandleSlot it = blocks[i] + m_blockStack.blockLength - 1; it != lastFrame.m_next - 1; --it)
            *it = TiValue();
        
        return;
    }
    
    for (HandleSlot it = m_frame.m_next - 1; it != lastFrame.m_next - 1; --it)
        *it = TiValue();
#endif
}

inline void HandleStack::leaveScope(Frame& lastFrame)
{
#ifndef NDEBUG
    --m_scopeDepth;
#endif

    zapTo(lastFrame);

    if (lastFrame.m_end != m_frame.m_end) // Popping to a frame in a different block.
        m_blockStack.shrink(lastFrame.m_end);

    m_frame = lastFrame;
}

inline HandleSlot HandleStack::push()
{
    ASSERT(m_scopeDepth); // Creating a Local outside of a LocalScope is a memory leak.
    if (m_frame.m_next == m_frame.m_end)
        grow();
    return m_frame.m_next++;
}

}

#endif
