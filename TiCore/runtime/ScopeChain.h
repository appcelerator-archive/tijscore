/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2003, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef ScopeChain_h
#define ScopeChain_h

#include "TiCell.h"
#include "Structure.h"
#include <wtf/FastAllocBase.h>

namespace TI {

    class TiGlobalData;
    class TiGlobalObject;
    class TiObject;
    class MarkStack;
    class ScopeChainIterator;
    typedef MarkStack SlotVisitor;
    
    class ScopeChainNode : public TiCell {
    public:
        ScopeChainNode(ScopeChainNode* next, TiObject* object, TiGlobalData* globalData, TiGlobalObject* globalObject, TiObject* globalThis)
            : TiCell(*globalData, globalData->scopeChainNodeStructure.get())
            , globalData(globalData)
            , next(*globalData, this, next, WriteBarrier<ScopeChainNode>::MayBeNull)
            , object(*globalData, this, object)
            , globalObject(*globalData, this, globalObject)
            , globalThis(*globalData, this, globalThis)
        {
            ASSERT(globalData);
            ASSERT(globalObject);
        }

        TiGlobalData* globalData;
        WriteBarrier<ScopeChainNode> next;
        WriteBarrier<TiObject> object;
        WriteBarrier<TiGlobalObject> globalObject;
        WriteBarrier<TiObject> globalThis;

        ScopeChainNode* push(TiObject*);
        ScopeChainNode* pop();

        ScopeChainIterator begin();
        ScopeChainIterator end();

        int localDepth();

#ifndef NDEBUG        
        void print();
#endif
        
        static Structure* createStructure(TiGlobalData& globalData, TiValue proto) { return Structure::create(globalData, proto, TypeInfo(CompoundType, StructureFlags), AnonymousSlotCount, &s_info); }
        virtual void visitChildren(SlotVisitor&);
        static JS_EXPORTDATA const ClassInfo s_info;

    private:
        static const unsigned StructureFlags = OverridesVisitChildren;
    };

    inline ScopeChainNode* ScopeChainNode::push(TiObject* o)
    {
        ASSERT(o);
        return new (globalData) ScopeChainNode(this, o, globalData, globalObject.get(), globalThis.get());
    }

    inline ScopeChainNode* ScopeChainNode::pop()
    {
        ASSERT(next);
        return next.get();
    }

    class ScopeChainIterator {
    public:
        ScopeChainIterator(ScopeChainNode* node)
            : m_node(node)
        {
        }

        WriteBarrier<TiObject> const & operator*() const { return m_node->object; }
        WriteBarrier<TiObject> const * operator->() const { return &(operator*()); }
    
        ScopeChainIterator& operator++() { m_node = m_node->next.get(); return *this; }

        // postfix ++ intentionally omitted

        bool operator==(const ScopeChainIterator& other) const { return m_node == other.m_node; }
        bool operator!=(const ScopeChainIterator& other) const { return m_node != other.m_node; }

    private:
        ScopeChainNode* m_node;
    };

    inline ScopeChainIterator ScopeChainNode::begin()
    {
        return ScopeChainIterator(this); 
    }

    inline ScopeChainIterator ScopeChainNode::end()
    { 
        return ScopeChainIterator(0); 
    }

    ALWAYS_INLINE TiGlobalData& TiExcState::globalData() const
    {
        ASSERT(scopeChain()->globalData);
        return *scopeChain()->globalData;
    }

    ALWAYS_INLINE TiGlobalObject* TiExcState::lexicalGlobalObject() const
    {
        return scopeChain()->globalObject.get();
    }
    
    ALWAYS_INLINE TiObject* TiExcState::globalThisValue() const
    {
        return scopeChain()->globalThis.get();
    }
    
    ALWAYS_INLINE ScopeChainNode* Register::scopeChain() const
    {
        return static_cast<ScopeChainNode*>(jsValue().asCell());
    }
    
    ALWAYS_INLINE Register& Register::operator=(ScopeChainNode* scopeChain)
    {
        *this = TiValue(scopeChain);
        return *this;
    }

} // namespace TI

#endif // ScopeChain_h
