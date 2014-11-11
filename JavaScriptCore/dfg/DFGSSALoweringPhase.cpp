/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DFGSSALoweringPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBasicBlockInlines.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "Operations.h"

namespace TI { namespace DFG {

class SSALoweringPhase : public Phase {
    static const bool verbose = false;
    
public:
    SSALoweringPhase(Graph& graph)
        : Phase(graph, "SSA lowering")
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        RELEASE_ASSERT(m_graph.m_form == SSA);
        
        for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
            m_block = m_graph.block(blockIndex);
            if (!m_block)
                continue;
            for (m_nodeIndex = 0; m_nodeIndex < m_block->size(); ++m_nodeIndex) {
                m_node = m_block->at(m_nodeIndex);
                handleNode();
            }
            m_insertionSet.execute(m_block);
        }

        return true;
    }

private:
    void handleNode()
    {
        switch (m_node->op()) {
        case GetByVal:
            lowerBoundsCheck(m_node->child1(), m_node->child2(), m_node->child3());
            break;
            
        case PutByVal:
        case PutByValDirect:
            lowerBoundsCheck(
                m_graph.varArgChild(m_node, 0),
                m_graph.varArgChild(m_node, 1),
                m_graph.varArgChild(m_node, 3));
            break;
            
        default:
            break;
        }
    }
    
    void lowerBoundsCheck(Edge base, Edge index, Edge storage)
    {
        if (!m_node->arrayMode().permitsBoundsCheckLowering())
            return;
        
        if (!m_node->arrayMode().lengthNeedsStorage())
            storage = Edge();
        
        Node* length = m_insertionSet.insertNode(
            m_nodeIndex, SpecInt32, GetArrayLength, m_node->codeOrigin,
            OpInfo(m_node->arrayMode().asWord()), base, storage);
        m_insertionSet.insertNode(
            m_nodeIndex, SpecInt32, CheckInBounds, m_node->codeOrigin,
            index, Edge(length, KnownInt32Use));
    }
    
    InsertionSet m_insertionSet;
    BasicBlock* m_block;
    unsigned m_nodeIndex;
    Node* m_node;
};

bool performSSALowering(Graph& graph)
{
    SamplingRegion samplingRegion("DFG SSA Lowering Phase");
    return runPhase<SSALoweringPhase>(graph);
}

} } // namespace TI::DFG

#endif // ENABLE(DFG_JIT)

