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
#include "FTLCapabilities.h"

#if ENABLE(FTL_JIT)

namespace TI { namespace FTL {

using namespace DFG;

inline CapabilityLevel canCompile(Node* node)
{
    // NOTE: If we ever have phantom arguments, we can compile them but we cannot
    // OSR enter.
    
    switch (node->op()) {
    case JSConstant:
    case WeakTIConstant:
    case GetLocal:
    case SetLocal:
    case MovHint:
    case ZombieHint:
    case Phantom:
    case Flush:
    case PhantomLocal:
    case SetArgument:
    case Return:
    case BitAnd:
    case BitOr:
    case BitXor:
    case BitRShift:
    case BitLShift:
    case BitURShift:
    case CheckStructure:
    case StructureTransitionWatchpoint:
    case ArrayifyToStructure:
    case PutStructure:
    case PhantomPutStructure:
    case GetButterfly:
    case NewObject:
    case NewArray:
    case NewArrayBuffer:
    case GetByOffset:
    case PutByOffset:
    case GetGlobalVar:
    case PutGlobalVar:
    case ValueAdd:
    case ArithAdd:
    case ArithSub:
    case ArithMul:
    case ArithDiv:
    case ArithMod:
    case ArithMin:
    case ArithMax:
    case ArithAbs:
    case ArithNegate:
    case UInt32ToNumber:
    case Int32ToDouble:
    case CompareEqConstant:
    case CompareStrictEqConstant:
    case Jump:
    case ForceOSRExit:
    case Phi:
    case Upsilon:
    case ExtractOSREntryLocal:
    case LoopHint:
    case Call:
    case Construct:
    case GetMyScope:
    case SkipScope:
    case GetClosureRegisters:
    case GetClosureVar:
    case PutClosureVar:
    case Int52ToValue:
    case InvalidationPoint:
    case StringCharAt:
    case CheckFunction:
    case StringCharCodeAt:
    case AllocatePropertyStorage:
    case FunctionReentryWatchpoint:
    case TypedArrayWatchpoint:
    case VariableWatchpoint:
    case NotifyWrite:
    case StoreBarrier:
    case ConditionalStoreBarrier:
    case StoreBarrierWithNullCheck:
    case ValueToInt32:
    case Branch:
    case LogicalNot:
    case CheckInBounds:
    case ConstantStoragePointer:
    case Check:
        // These are OK.
        break;
    case GetById:
    case PutById:
        if (node->child1().useKind() == CellUse)
            break;
        return CannotCompile;
    case GetIndexedPropertyStorage:
        if (node->arrayMode().type() == Array::String)
            break;
        if (isTypedView(node->arrayMode().typedArrayType()))
            break;
        return CannotCompile;
    case CheckArray:
        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous:
            break;
        default:
            if (isTypedView(node->arrayMode().typedArrayType()))
                break;
            return CannotCompile;
        }
        break;
    case GetArrayLength:
        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous:
        case Array::String:
            break;
        default:
            if (isTypedView(node->arrayMode().typedArrayType()))
                break;
            return CannotCompile;
        }
        break;
    case GetByVal:
        switch (node->arrayMode().type()) {
        case Array::ForceExit:
        case Array::Generic:
        case Array::String:
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous:
            break;
        default:
            if (isTypedView(node->arrayMode().typedArrayType()))
                return CanCompileAndOSREnter;
            return CannotCompile;
        }
        break;
    case PutByVal:
    case PutByValAlias:
    case PutByValDirect:
        switch (node->arrayMode().type()) {
        case Array::ForceExit:
        case Array::Generic:
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous:
            break;
        default:
            if (isTypedView(node->arrayMode().typedArrayType()))
                return CanCompileAndOSREnter;
            return CannotCompile;
        }
        break;
    case CompareEq:
        if (node->isBinaryUseKind(Int32Use))
            break;
        if (node->isBinaryUseKind(MachineIntUse))
            break;
        if (node->isBinaryUseKind(NumberUse))
            break;
        if (node->isBinaryUseKind(ObjectUse))
            break;
        if (node->isBinaryUseKind(UntypedUse))
            break;
        return CannotCompile;
    case CompareStrictEq:
        if (node->isBinaryUseKind(Int32Use))
            break;
        if (node->isBinaryUseKind(MachineIntUse))
            break;
        if (node->isBinaryUseKind(NumberUse))
            break;
        if (node->isBinaryUseKind(ObjectUse))
            break;
        return CannotCompile;
    case CompareLess:
    case CompareLessEq:
    case CompareGreater:
    case CompareGreaterEq:
        if (node->isBinaryUseKind(Int32Use))
            break;
        if (node->isBinaryUseKind(MachineIntUse))
            break;
        if (node->isBinaryUseKind(NumberUse))
            break;
        if (node->isBinaryUseKind(UntypedUse))
            break;
        return CannotCompile;
    case Switch:
        switch (node->switchData()->kind) {
        case SwitchImm:
        case SwitchChar:
            break;
        default:
            return CannotCompile;
        }
        break;
    default:
        // Don't know how to handle anything else.
        return CannotCompile;
    }
    return CanCompileAndOSREnter;
}

CapabilityLevel canCompile(Graph& graph)
{
    if (graph.m_codeBlock->codeType() != FunctionCode) {
        if (verboseCompilationEnabled())
            dataLog("FTL rejecting code block that doesn't belong to a function.\n");
        return CannotCompile;
    }
    
    CapabilityLevel result = CanCompileAndOSREnter;
    
    for (BlockIndex blockIndex = graph.numBlocks(); blockIndex--;) {
        BasicBlock* block = graph.block(blockIndex);
        if (!block)
            continue;
        
        // We don't care if we can compile blocks that the CFA hasn't visited.
        if (!block->cfaHasVisited)
            continue;
        
        for (unsigned nodeIndex = 0; nodeIndex < block->size(); ++nodeIndex) {
            Node* node = block->at(nodeIndex);
            
            for (unsigned childIndex = graph.numChildren(node); childIndex--;) {
                Edge edge = graph.child(node, childIndex);
                if (!edge)
                    continue;
                switch (edge.useKind()) {
                case UntypedUse:
                case Int32Use:
                case KnownInt32Use:
                case MachineIntUse:
                case NumberUse:
                case KnownNumberUse:
                case RealNumberUse:
                case BooleanUse:
                case CellUse:
                case KnownCellUse:
                case ObjectUse:
                case ObjectOrOtherUse:
                case StringUse:
                case FinalObjectUse:
                    // These are OK.
                    break;
                default:
                    // Don't know how to handle anything else.
                    if (verboseCompilationEnabled()) {
                        dataLog("FTL rejecting node because of bad use kind: ", edge.useKind(), " in node:\n");
                        graph.dump(WTI::dataFile(), "    ", node);
                    }
                    return CannotCompile;
                }
            }
            
            switch (canCompile(node)) {
            case CannotCompile: 
                if (verboseCompilationEnabled()) {
                    dataLog("FTL rejecting node:\n");
                    graph.dump(WTI::dataFile(), "    ", node);
                }
                return CannotCompile;
                
            case CanCompile:
                if (result == CanCompileAndOSREnter && verboseCompilationEnabled()) {
                    dataLog("FTL disabling OSR entry because of node:\n");
                    graph.dump(WTI::dataFile(), "    ", node);
                }
                result = CanCompile;
                break;
                
            case CanCompileAndOSREnter:
                break;
            }
            
            if (node->op() == ForceOSRExit)
                break;
        }
    }
    
    return result;
}

} } // namespace TI::FTL

#endif // ENABLE(FTL_JIT)

