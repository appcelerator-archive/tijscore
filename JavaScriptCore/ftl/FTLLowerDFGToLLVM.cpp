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
#include "FTLLowerDFGToLLVM.h"

#if ENABLE(FTL_JIT)

#include "CodeBlockWithJITType.h"
#include "DFGAbstractInterpreterInlines.h"
#include "DFGInPlaceAbstractState.h"
#include "FTLAbstractHeapRepository.h"
#include "FTLForOSREntryJITCode.h"
#include "FTLFormattedValue.h"
#include "FTLInlineCacheSize.h"
#include "FTLLoweredNodeValue.h"
#include "FTLOutput.h"
#include "FTLThunks.h"
#include "LinkBuffer.h"
#include "OperandsInlines.h"
#include "Operations.h"
#include "VirtualRegister.h"

#include <wtf/ProcessID.h>

namespace TI { namespace FTL {

using namespace DFG;

static int compileCounter;

// Using this instead of typeCheck() helps to reduce the load on LLVM, by creating
// significantly less dead code.
#define FTL_TYPE_CHECK(lowValue, highValue, typesPassedThrough, failCondition) do { \
        FormattedValue _ftc_lowValue = (lowValue);                      \
        Edge _ftc_highValue = (highValue);                              \
        SpeculatedType _ftc_typesPassedThrough = (typesPassedThrough);  \
        if (!m_interpreter.needsTypeCheck(_ftc_highValue, _ftc_typesPassedThrough)) \
            break;                                                      \
        typeCheck(_ftc_lowValue, _ftc_highValue, _ftc_typesPassedThrough, (failCondition)); \
    } while (false)

class LowerDFGToLLVM {
public:
    LowerDFGToLLVM(State& state)
        : m_graph(state.graph)
        , m_ftlState(state)
        , m_heaps(state.context)
        , m_out(state.context)
        , m_availability(OperandsLike, state.graph.block(0)->variablesAtHead)
        , m_state(state.graph)
        , m_interpreter(state.graph, m_state)
        , m_stackmapIDs(0)
    {
    }
    
    void lower()
    {
        CString name;
        if (verboseCompilationEnabled()) {
            name = toCString(
                "jsBody_", atomicIncrement(&compileCounter), "_", codeBlock()->inferredName(),
                "_", codeBlock()->hash());
        } else
            name = "jsBody";
        
        m_graph.m_dominators.computeIfNecessary(m_graph);
        
        m_ftlState.module =
            llvm->ModuleCreateWithNameInContext(name.data(), m_ftlState.context);
        
        m_ftlState.function = addFunction(
            m_ftlState.module, name.data(), functionType(m_out.int64, m_out.intPtr));
        setFunctionCallingConv(m_ftlState.function, LLVMCCallConv);
        
        m_out.initialize(m_ftlState.module, m_ftlState.function, m_heaps);
        
        m_prologue = appendBasicBlock(m_ftlState.context, m_ftlState.function);
        m_out.appendTo(m_prologue);
        createPhiVariables();
        
        m_callFrame = m_out.param(0);
        m_tagTypeNumber = m_out.constInt64(TagTypeNumber);
        m_tagMask = m_out.constInt64(TagMask);
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            m_highBlock = m_graph.block(blockIndex);
            if (!m_highBlock)
                continue;
            m_blocks.add(m_highBlock, FTL_NEW_BLOCK(m_out, ("Block ", *m_highBlock)));
        }
        
        m_out.appendTo(m_prologue);
        m_out.jump(lowBlock(m_graph.block(0)));
        
        Vector<BasicBlock*> depthFirst;
        m_graph.getBlocksInDepthFirstOrder(depthFirst);
        for (unsigned i = 0; i < depthFirst.size(); ++i)
            compileBlock(depthFirst[i]);
        
        if (Options::dumpLLVMIR())
            dumpModule(m_ftlState.module);
        
        if (verboseCompilationEnabled())
            m_ftlState.dumpState("after lowering");
        if (validationEnabled())
            verifyModule(m_ftlState.module);
    }

private:
    
    void createPhiVariables()
    {
        for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            for (unsigned nodeIndex = block->size(); nodeIndex--;) {
                Node* node = block->at(nodeIndex);
                if (node->op() != Phi)
                    continue;
                LType type;
                switch (node->flags() & NodeResultMask) {
                case NodeResultNumber:
                    type = m_out.doubleType;
                    break;
                case NodeResultInt32:
                    type = m_out.int32;
                    break;
                case NodeResultInt52:
                    type = m_out.int64;
                    break;
                case NodeResultBoolean:
                    type = m_out.boolean;
                    break;
                case NodeResultJS:
                    type = m_out.int64;
                    break;
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                    break;
                }
                m_phis.add(node, buildAlloca(m_out.m_builder, type));
            }
        }
    }
    
    void compileBlock(BasicBlock* block)
    {
        if (!block)
            return;
        
        if (verboseCompilationEnabled())
            dataLog("Compiling block ", *block, "\n");
        
        m_highBlock = block;
        
        LBasicBlock lowBlock = m_blocks.get(m_highBlock);
        
        m_nextHighBlock = 0;
        for (BlockIndex nextBlockIndex = m_highBlock->index + 1; nextBlockIndex < m_graph.numBlocks(); ++nextBlockIndex) {
            m_nextHighBlock = m_graph.block(nextBlockIndex);
            if (m_nextHighBlock)
                break;
        }
        m_nextLowBlock = m_nextHighBlock ? m_blocks.get(m_nextHighBlock) : 0;
        
        // All of this effort to find the next block gives us the ability to keep the
        // generated IR in roughly program order. This ought not affect the performance
        // of the generated code (since we expect LLVM to reorder things) but it will
        // make IR dumps easier to read.
        m_out.appendTo(lowBlock, m_nextLowBlock);
        
        if (Options::ftlCrashes())
            m_out.crashNonTerminal();
        
        if (!m_highBlock->cfaHasVisited) {
            m_out.crash();
            return;
        }
        
        initializeOSRExitStateForBlock();
        
        m_state.reset();
        m_state.beginBasicBlock(m_highBlock);
        
        for (m_nodeIndex = 0; m_nodeIndex < m_highBlock->size(); ++m_nodeIndex) {
            if (!compileNode(m_nodeIndex))
                break;
        }
    }
    
    bool compileNode(unsigned nodeIndex)
    {
        if (!m_state.isValid()) {
            m_out.unreachable();
            return false;
        }
        
        m_node = m_highBlock->at(nodeIndex);
        m_codeOriginForExitProfile = m_node->codeOrigin;
        m_codeOriginForExitTarget = m_node->codeOriginForExitTarget;
        
        if (verboseCompilationEnabled())
            dataLog("Lowering ", m_node, "\n");
        
        bool shouldExecuteEffects = m_interpreter.startExecuting(m_node);
        
        switch (m_node->op()) {
        case Upsilon:
            compileUpsilon();
            break;
        case Phi:
            compilePhi();
            break;
        case JSConstant:
            break;
        case WeakTIConstant:
            compileWeakTIConstant();
            break;
        case GetArgument:
            compileGetArgument();
            break;
        case ExtractOSREntryLocal:
            compileExtractOSREntryLocal();
            break;
        case GetLocal:
            compileGetLocal();
            break;
        case SetLocal:
            compileSetLocal();
            break;
        case MovHint:
            compileMovHint();
            break;
        case ZombieHint:
            compileZombieHint();
            break;
        case Phantom:
            compilePhantom();
            break;
        case ValueAdd:
            compileValueAdd();
            break;
        case ArithAdd:
            compileAddSub();
            break;
        case ArithSub:
            compileAddSub();
            break;
        case ArithMul:
            compileArithMul();
            break;
        case ArithDiv:
            compileArithDivMod();
            break;
        case ArithMod:
            compileArithDivMod();
            break;
        case ArithMin:
        case ArithMax:
            compileArithMinOrMax();
            break;
        case ArithAbs:
            compileArithAbs();
            break;
        case ArithNegate:
            compileArithNegate();
            break;
        case BitAnd:
            compileBitAnd();
            break;
        case BitOr:
            compileBitOr();
            break;
        case BitXor:
            compileBitXor();
            break;
        case BitRShift:
            compileBitRShift();
            break;
        case BitLShift:
            compileBitLShift();
            break;
        case BitURShift:
            compileBitURShift();
            break;
        case UInt32ToNumber:
            compileUInt32ToNumber();
            break;
        case Int32ToDouble:
            compileInt32ToDouble();
            break;
        case CheckStructure:
            compileCheckStructure();
            break;
        case StructureTransitionWatchpoint:
            compileStructureTransitionWatchpoint();
            break;
        case CheckFunction:
            compileCheckFunction();
            break;
        case ArrayifyToStructure:
            compileArrayifyToStructure();
            break;
        case PutStructure:
            compilePutStructure();
            break;
        case PhantomPutStructure:
            compilePhantomPutStructure();
            break;
        case GetById:
            compileGetById();
            break;
        case PutById:
            compilePutById();
            break;
        case GetButterfly:
            compileGetButterfly();
            break;
        case ConstantStoragePointer:
            compileConstantStoragePointer();
            break;
        case GetIndexedPropertyStorage:
            compileGetIndexedPropertyStorage();
            break;
        case CheckArray:
            compileCheckArray();
            break;
        case GetArrayLength:
            compileGetArrayLength();
            break;
        case CheckInBounds:
            compileCheckInBounds();
            break;
        case GetByVal:
            compileGetByVal();
            break;
        case PutByVal:
        case PutByValAlias:
        case PutByValDirect:
            compilePutByVal();
            break;
        case NewObject:
            compileNewObject();
            break;
        case NewArray:
            compileNewArray();
            break;
        case NewArrayBuffer:
            compileNewArrayBuffer();
            break;
        case AllocatePropertyStorage:
            compileAllocatePropertyStorage();
            break;
        case StringCharAt:
            compileStringCharAt();
            break;
        case StringCharCodeAt:
            compileStringCharCodeAt();
            break;
        case GetByOffset:
            compileGetByOffset();
            break;
        case PutByOffset:
            compilePutByOffset();
            break;
        case GetGlobalVar:
            compileGetGlobalVar();
            break;
        case PutGlobalVar:
            compilePutGlobalVar();
            break;
        case NotifyWrite:
            compileNotifyWrite();
            break;
        case GetMyScope:
            compileGetMyScope();
            break;
        case SkipScope:
            compileSkipScope();
            break;
        case GetClosureRegisters:
            compileGetClosureRegisters();
            break;
        case GetClosureVar:
            compileGetClosureVar();
            break;
        case PutClosureVar:
            compilePutClosureVar();
            break;
        case CompareEq:
            compileCompareEq();
            break;
        case CompareEqConstant:
            compileCompareEqConstant();
            break;
        case CompareStrictEq:
            compileCompareStrictEq();
            break;
        case CompareStrictEqConstant:
            compileCompareStrictEqConstant();
            break;
        case CompareLess:
            compileCompareLess();
            break;
        case CompareLessEq:
            compileCompareLessEq();
            break;
        case CompareGreater:
            compileCompareGreater();
            break;
        case CompareGreaterEq:
            compileCompareGreaterEq();
            break;
        case LogicalNot:
            compileLogicalNot();
            break;
        case Call:
        case Construct:
            compileCallOrConstruct();
            break;
        case Jump:
            compileJump();
            break;
        case Branch:
            compileBranch();
            break;
        case Switch:
            compileSwitch();
            break;
        case Return:
            compileReturn();
            break;
        case ForceOSRExit:
            compileForceOSRExit();
            break;
        case InvalidationPoint:
            compileInvalidationPoint();
            break;
        case ValueToInt32:
            compileValueToInt32();
            break;
        case Int52ToValue:
            compileInt52ToValue();
            break;
        case StoreBarrier:
            compileStoreBarrier();
            break;
        case ConditionalStoreBarrier:
            compileConditionalStoreBarrier();
            break;
        case StoreBarrierWithNullCheck:
            compileStoreBarrierWithNullCheck();
            break;
        case Flush:
        case PhantomLocal:
        case SetArgument:
        case LoopHint:
        case VariableWatchpoint:
        case FunctionReentryWatchpoint:
        case TypedArrayWatchpoint:
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        
        if (shouldExecuteEffects)
            m_interpreter.executeEffects(nodeIndex);
        
        return true;
    }

    void compileValueToInt32()
    {
        switch (m_node->child1().useKind()) {
        case Int32Use:
            setInt32(lowInt32(m_node->child1()));
            break;
            
        case MachineIntUse:
            setInt32(m_out.castToInt32(lowStrictInt52(m_node->child1())));
            break;
            
        case NumberUse:
        case NotCellUse: {
            LoweredNodeValue value = m_int32Values.get(m_node->child1().node());
            if (isValid(value)) {
                setInt32(value.value());
                break;
            }
            
            value = m_jsValueValues.get(m_node->child1().node());
            if (isValid(value)) {
                LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 int case"));
                LBasicBlock notIntCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 not int case"));
                LBasicBlock doubleCase = 0;
                LBasicBlock notNumberCase = 0;
                if (m_node->child1().useKind() == NotCellUse) {
                    doubleCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 double case"));
                    notNumberCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 not number case"));
                }
                LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ValueToInt32 continuation"));
                
                Vector<ValueFromBlock> results;
                
                m_out.branch(isNotInt32(value.value()), notIntCase, intCase);
                
                LBasicBlock lastNext = m_out.appendTo(intCase, notIntCase);
                results.append(m_out.anchor(unboxInt32(value.value())));
                m_out.jump(continuation);
                
                if (m_node->child1().useKind() == NumberUse) {
                    m_out.appendTo(notIntCase, continuation);
                    FTL_TYPE_CHECK(
                        jsValueValue(value.value()), m_node->child1(), SpecFullNumber,
                        isCellOrMisc(value.value()));
                    results.append(m_out.anchor(doubleToInt32(unboxDouble(value.value()))));
                    m_out.jump(continuation);
                } else {
                    m_out.appendTo(notIntCase, doubleCase);
                    m_out.branch(isCellOrMisc(value.value()), notNumberCase, doubleCase);
                    
                    m_out.appendTo(doubleCase, notNumberCase);
                    results.append(m_out.anchor(doubleToInt32(unboxDouble(value.value()))));
                    m_out.jump(continuation);
                    
                    m_out.appendTo(notNumberCase, continuation);
                    
                    FTL_TYPE_CHECK(
                        jsValueValue(value.value()), m_node->child1(), ~SpecCell,
                        isCell(value.value()));
                    
                    LValue specialResult = m_out.select(
                        m_out.equal(
                            value.value(),
                            m_out.constInt64(TiValue::encode(jsBoolean(true)))),
                        m_out.int32One, m_out.int32Zero);
                    results.append(m_out.anchor(specialResult));
                    m_out.jump(continuation);
                }
                
                m_out.appendTo(continuation, lastNext);
                setInt32(m_out.phi(m_out.int32, results));
                break;
            }
            
            value = m_doubleValues.get(m_node->child1().node());
            if (isValid(value)) {
                setInt32(doubleToInt32(value.value()));
                break;
            }
            
            terminate(Uncountable);
            break;
        }
            
        case BooleanUse:
            setInt32(m_out.zeroExt(lowBoolean(m_node->child1()), m_out.int32));
            break;
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }

    void compileInt52ToValue()
    {
        setTiValue(lowTiValue(m_node->child1()));
    }

    void compileStoreBarrier()
    {
        emitStoreBarrier(lowCell(m_node->child1()));
    }

    void compileConditionalStoreBarrier()
    {
        LValue base = lowCell(m_node->child1());
        LValue value = lowTiValue(m_node->child2());
        emitStoreBarrier(base, value, m_node->child2());
    }

    void compileStoreBarrierWithNullCheck()
    {
#if ENABLE(GGC)
        LBasicBlock isNotNull = FTL_NEW_BLOCK(m_out, ("Store barrier with null check value not null"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Store barrier continuation"));

        LValue base = lowTiValue(m_node->child1());
        m_out.branch(m_out.isZero64(base), continuation, isNotNull);
        LBasicBlock lastNext = m_out.appendTo(isNotNull, continuation);
        emitStoreBarrier(base);
        m_out.appendTo(continuation, lastNext);
#else
        speculate(m_node->child1());
#endif
    }

    void compileUpsilon()
    {
        LValue destination = m_phis.get(m_node->phi());
        
        switch (m_node->child1().useKind()) {
        case NumberUse:
            m_out.set(lowDouble(m_node->child1()), destination);
            break;
        case Int32Use:
            m_out.set(lowInt32(m_node->child1()), destination);
            break;
        case MachineIntUse:
            m_out.set(lowInt52(m_node->child1()), destination);
            break;
        case BooleanUse:
            m_out.set(lowBoolean(m_node->child1()), destination);
            break;
        case CellUse:
            m_out.set(lowCell(m_node->child1()), destination);
            break;
        case UntypedUse:
            m_out.set(lowTiValue(m_node->child1()), destination);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compilePhi()
    {
        LValue source = m_phis.get(m_node);
        
        switch (m_node->flags() & NodeResultMask) {
        case NodeResultNumber:
            setDouble(m_out.get(source));
            break;
        case NodeResultInt32:
            setInt32(m_out.get(source));
            break;
        case NodeResultInt52:
            setInt52(m_out.get(source));
            break;
        case NodeResultBoolean:
            setBoolean(m_out.get(source));
            break;
        case NodeResultJS:
            setTiValue(m_out.get(source));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileWeakTIConstant()
    {
        setTiValue(weakPointer(m_node->weakConstant()));
    }
    
    void compileGetArgument()
    {
        VariableAccessData* variable = m_node->variableAccessData();
        VirtualRegister operand = variable->machineLocal();
        RELEASE_ASSERT(operand.isArgument());

        LValue jsValue = m_out.load64(addressFor(operand));

        switch (useKindFor(variable->flushFormat())) {
        case Int32Use:
            speculate(BadType, jsValueValue(jsValue), m_node, isNotInt32(jsValue));
            setInt32(unboxInt32(jsValue));
            break;
        case CellUse:
            speculate(BadType, jsValueValue(jsValue), m_node, isNotCell(jsValue));
            setTiValue(jsValue);
            break;
        case BooleanUse:
            speculate(BadType, jsValueValue(jsValue), m_node, isNotBoolean(jsValue));
            setBoolean(unboxBoolean(jsValue));
            break;
        case UntypedUse:
            setTiValue(jsValue);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileExtractOSREntryLocal()
    {
        EncodedTiValue* buffer = static_cast<EncodedTiValue*>(
            m_ftlState.jitCode->ftlForOSREntry()->entryBuffer()->dataBuffer());
        setTiValue(m_out.load64(m_out.absolute(buffer + m_node->unlinkedLocal().toLocal())));
    }
    
    void compileGetLocal()
    {
        // GetLocals arise only for captured variables.
        
        VariableAccessData* variable = m_node->variableAccessData();
        AbstractValue& value = m_state.variables().operand(variable->local());
        
        RELEASE_ASSERT(variable->isCaptured());
        
        if (isInt32Speculation(value.m_type))
            setInt32(m_out.load32(payloadFor(variable->machineLocal())));
        else
            setTiValue(m_out.load64(addressFor(variable->machineLocal())));
    }
    
    void compileSetLocal()
    {
        VariableAccessData* variable = m_node->variableAccessData();
        switch (variable->flushFormat()) {
        case FlushedTiValue: {
            LValue value = lowTiValue(m_node->child1());
            m_out.store64(value, addressFor(variable->machineLocal()));
            break;
        }
            
        case FlushedDouble: {
            LValue value = lowDouble(m_node->child1());
            m_out.storeDouble(value, addressFor(variable->machineLocal()));
            break;
        }
            
        case FlushedInt32: {
            LValue value = lowInt32(m_node->child1());
            m_out.store32(value, payloadFor(variable->machineLocal()));
            break;
        }
            
        case FlushedInt52: {
            LValue value = lowInt52(m_node->child1());
            m_out.store64(value, addressFor(variable->machineLocal()));
            break;
        }
            
        case FlushedCell: {
            LValue value = lowCell(m_node->child1());
            m_out.store64(value, addressFor(variable->machineLocal()));
            break;
        }
            
        case FlushedBoolean: {
            speculateBoolean(m_node->child1());
            m_out.store64(
                lowTiValue(m_node->child1(), ManualOperandSpeculation),
                addressFor(variable->machineLocal()));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        
        m_availability.operand(variable->local()) = Availability(variable->flushedAt());
    }
    
    void compileMovHint()
    {
        ASSERT(m_node->containsMovHint());
        ASSERT(m_node->op() != ZombieHint);
        
        VirtualRegister operand = m_node->unlinkedLocal();
        m_availability.operand(operand) = Availability(m_node->child1().node());
    }
    
    void compileZombieHint()
    {
        m_availability.operand(m_node->unlinkedLocal()) = Availability::unavailable();
    }
    
    void compilePhantom()
    {
        DFG_NODE_DO_TO_CHILDREN(m_graph, m_node, speculate);
    }
    
    void compileValueAdd()
    {
        J_JITOperation_EJJ operation;
        if (!(m_state.forNode(m_node->child1()).m_type & SpecFullNumber)
            && !(m_state.forNode(m_node->child2()).m_type & SpecFullNumber))
            operation = operationValueAddNotNumber;
        else
            operation = operationValueAdd;
        setTiValue(vmCall(
            m_out.operation(operation), m_callFrame,
            lowTiValue(m_node->child1()), lowTiValue(m_node->child2())));
    }
    
    void compileAddSub()
    {
        bool isSub =  m_node->op() == ArithSub;
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            LValue result = isSub ? m_out.sub(left, right) : m_out.add(left, right);

            if (bytecodeCanTruncateInteger(m_node->arithNodeFlags())) {
                setInt32(result);
                break;
            }

            LValue overflow = isSub ? m_out.subWithOverflow32(left, right) : m_out.addWithOverflow32(left, right);

            speculate(Overflow, noValue(), 0, m_out.extractValue(overflow, 1));
            setInt32(result);
            break;
        }
            
        case MachineIntUse: {
            if (!m_state.forNode(m_node->child1()).couldBeType(SpecInt52)
                && !m_state.forNode(m_node->child2()).couldBeType(SpecInt52)) {
                Int52Kind kind;
                LValue left = lowWhicheverInt52(m_node->child1(), kind);
                LValue right = lowInt52(m_node->child2(), kind);
                setInt52(isSub ? m_out.sub(left, right) : m_out.add(left, right), kind);
                break;
            }
            
            LValue left = lowInt52(m_node->child1());
            LValue right = lowInt52(m_node->child2());
            LValue result = isSub ? m_out.sub(left, right) : m_out.add(left, right);

            LValue overflow = isSub ? m_out.subWithOverflow64(left, right) : m_out.addWithOverflow64(left, right);
            speculate(Int52Overflow, noValue(), 0, m_out.extractValue(overflow, 1));
            setInt52(result);
            break;
        }
            
        case NumberUse: {
            LValue C1 = lowDouble(m_node->child1());
            LValue C2 = lowDouble(m_node->child2());

            setDouble(isSub ? m_out.doubleSub(C1, C2) : m_out.doubleAdd(C1, C2));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileArithMul()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            LValue result = m_out.mul(left, right);

            if (!bytecodeCanTruncateInteger(m_node->arithNodeFlags())) {
                LValue overflowResult = m_out.mulWithOverflow32(left, right);
                speculate(Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
            }
            
            if (!bytecodeCanIgnoreNegativeZero(m_node->arithNodeFlags())) {
                LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("ArithMul slow case"));
                LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMul continuation"));
                
                m_out.branch(m_out.notZero32(result), continuation, slowCase);
                
                LBasicBlock lastNext = m_out.appendTo(slowCase, continuation);
                LValue cond = m_out.bitOr(m_out.lessThan(left, m_out.int32Zero), m_out.lessThan(right, m_out.int32Zero));
                speculate(NegativeZero, noValue(), 0, cond);
                m_out.jump(continuation);
                m_out.appendTo(continuation, lastNext);
            }
            
            setInt32(result);
            break;
        }
            
        case MachineIntUse: {
            Int52Kind kind;
            LValue left = lowWhicheverInt52(m_node->child1(), kind);
            LValue right = lowInt52(m_node->child2(), opposite(kind));
            LValue result = m_out.mul(left, right);


            LValue overflowResult = m_out.mulWithOverflow64(left, right);
            speculate(Int52Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));

            if (!bytecodeCanIgnoreNegativeZero(m_node->arithNodeFlags())) {
                LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("ArithMul slow case"));
                LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMul continuation"));
                
                m_out.branch(m_out.notZero64(result), continuation, slowCase);
                
                LBasicBlock lastNext = m_out.appendTo(slowCase, continuation);
                LValue cond = m_out.bitOr(m_out.lessThan(left, m_out.int64Zero), m_out.lessThan(right, m_out.int64Zero));
                speculate(NegativeZero, noValue(), 0, cond);
                m_out.jump(continuation);
                m_out.appendTo(continuation, lastNext);
            }
            
            setInt52(result);
            break;
        }
            
        case NumberUse: {
            setDouble(
                m_out.doubleMul(lowDouble(m_node->child1()), lowDouble(m_node->child2())));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }

    void compileArithDivMod()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue numerator = lowInt32(m_node->child1());
            LValue denominator = lowInt32(m_node->child2());
            
            LBasicBlock unsafeDenominator = FTL_NEW_BLOCK(m_out, ("ArithDivMod unsafe denominator"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithDivMod continuation"));
            LBasicBlock done = FTL_NEW_BLOCK(m_out, ("ArithDivMod done"));
            
            Vector<ValueFromBlock, 3> results;
            
            LValue adjustedDenominator = m_out.add(denominator, m_out.int32One);
            
            m_out.branch(m_out.above(adjustedDenominator, m_out.int32One), continuation, unsafeDenominator);
            
            LBasicBlock lastNext = m_out.appendTo(unsafeDenominator, continuation);
            
            LValue neg2ToThe31 = m_out.constInt32(-2147483647-1);
            
            if (bytecodeUsesAsNumber(m_node->arithNodeFlags())) {
                LValue cond = m_out.bitOr(m_out.isZero32(denominator), m_out.equal(numerator, neg2ToThe31));
                speculate(Overflow, noValue(), 0, cond);
                m_out.jump(continuation);
            } else {
                // This is the case where we convert the result to an int after we're done. So,
                // if the denominator is zero, then the result should be zero.
                // If the denominator is not zero (i.e. it's -1 because we're guarded by the
                // check above) and the numerator is -2^31 then the result should be -2^31.
                
                LBasicBlock divByZero = FTL_NEW_BLOCK(m_out, ("ArithDiv divide by zero"));
                LBasicBlock notDivByZero = FTL_NEW_BLOCK(m_out, ("ArithDiv not divide by zero"));
                LBasicBlock neg2ToThe31ByNeg1 = FTL_NEW_BLOCK(m_out, ("ArithDiv -2^31/-1"));
                
                m_out.branch(m_out.isZero32(denominator), divByZero, notDivByZero);
                
                m_out.appendTo(divByZero, notDivByZero);
                results.append(m_out.anchor(m_out.int32Zero));
                m_out.jump(done);
                
                m_out.appendTo(notDivByZero, neg2ToThe31ByNeg1);
                m_out.branch(m_out.equal(numerator, neg2ToThe31), neg2ToThe31ByNeg1, continuation);
                
                m_out.appendTo(neg2ToThe31ByNeg1, continuation);
                results.append(m_out.anchor(neg2ToThe31));
                m_out.jump(done);
            }
            
            m_out.appendTo(continuation, done);
            
            if (!bytecodeCanIgnoreNegativeZero(m_node->arithNodeFlags())) {
                LBasicBlock zeroNumerator = FTL_NEW_BLOCK(m_out, ("ArithDivMod zero numerator"));
                LBasicBlock numeratorContinuation = FTL_NEW_BLOCK(m_out, ("ArithDivMod numerator continuation"));
                
                m_out.branch(m_out.isZero32(numerator), zeroNumerator, numeratorContinuation);
                
                LBasicBlock innerLastNext = m_out.appendTo(zeroNumerator, numeratorContinuation);
                
                speculate(
                    NegativeZero, noValue(), 0, m_out.lessThan(denominator, m_out.int32Zero));
                
                m_out.jump(numeratorContinuation);
                
                m_out.appendTo(numeratorContinuation, innerLastNext);
            }
            
            LValue divModResult = m_node->op() == ArithDiv
                ? m_out.div(numerator, denominator)
                : m_out.rem(numerator, denominator);
            
            if (bytecodeUsesAsNumber(m_node->arithNodeFlags())) {
                speculate(
                    Overflow, noValue(), 0,
                    m_out.notEqual(m_out.mul(divModResult, denominator), numerator));
            }
            
            results.append(m_out.anchor(divModResult));
            m_out.jump(done);
            
            m_out.appendTo(done, lastNext);
            
            setInt32(m_out.phi(m_out.int32, results));
            break;
        }
            
        case NumberUse: {
            LValue C1 = lowDouble(m_node->child1());
            LValue C2 = lowDouble(m_node->child2());
            setDouble(m_node->op() == ArithDiv ? m_out.doubleDiv(C1, C2) : m_out.doubleRem(C1, C2));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileArithMinOrMax()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            
            setInt32(
                m_out.select(
                    m_node->op() == ArithMin
                        ? m_out.lessThan(left, right)
                        : m_out.lessThan(right, left),
                    left, right));
            break;
        }
            
        case NumberUse: {
            LValue left = lowDouble(m_node->child1());
            LValue right = lowDouble(m_node->child2());
            
            LBasicBlock notLessThan = FTL_NEW_BLOCK(m_out, ("ArithMin/ArithMax not less than"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMin/ArithMax continuation"));
            
            Vector<ValueFromBlock, 2> results;
            
            results.append(m_out.anchor(left));
            m_out.branch(
                m_node->op() == ArithMin
                    ? m_out.doubleLessThan(left, right)
                    : m_out.doubleGreaterThan(left, right),
                continuation, notLessThan);
            
            LBasicBlock lastNext = m_out.appendTo(notLessThan, continuation);
            results.append(m_out.anchor(m_out.select(
                m_node->op() == ArithMin
                    ? m_out.doubleGreaterThanOrEqual(left, right)
                    : m_out.doubleLessThanOrEqual(left, right),
                right, m_out.constDouble(0.0 / 0.0))));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setDouble(m_out.phi(m_out.doubleType, results));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileArithAbs()
    {
        switch (m_node->child1().useKind()) {
        case Int32Use: {
            LValue value = lowInt32(m_node->child1());
            
            LValue mask = m_out.aShr(value, m_out.constInt32(31));
            LValue result = m_out.bitXor(mask, m_out.add(mask, value));
            
            speculate(Overflow, noValue(), 0, m_out.equal(result, m_out.constInt32(1 << 31)));
            
            setInt32(result);
            break;
        }
            
        case NumberUse: {
            setDouble(m_out.doubleAbs(lowDouble(m_node->child1())));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileArithNegate()
    {
        switch (m_node->child1().useKind()) {
        case Int32Use: {
            LValue value = lowInt32(m_node->child1());
            
            LValue result = m_out.neg(value);
            if (!bytecodeCanTruncateInteger(m_node->arithNodeFlags())) {
                if (bytecodeCanIgnoreNegativeZero(m_node->arithNodeFlags())) {
                    // We don't have a negate-with-overflow intrinsic. Hopefully this
                    // does the trick, though.
                    LValue overflowResult = m_out.subWithOverflow32(m_out.int32Zero, value);
                    speculate(Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
                } else
                    speculate(Overflow, noValue(), 0, m_out.testIsZero32(value, m_out.constInt32(0x7fffffff)));

            }

            setInt32(result);
            break;
        }
            
        case MachineIntUse: {
            if (!m_state.forNode(m_node->child1()).couldBeType(SpecInt52)) {
                Int52Kind kind;
                LValue value = lowWhicheverInt52(m_node->child1(), kind);
                LValue result = m_out.neg(value);
                if (!bytecodeCanIgnoreNegativeZero(m_node->arithNodeFlags()))
                    speculate(NegativeZero, noValue(), 0, m_out.isZero64(result));
                setInt52(result, kind);
                break;
            }
            
            LValue value = lowInt52(m_node->child1());
            LValue overflowResult = m_out.subWithOverflow64(m_out.int64Zero, value);
            speculate(Int52Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
            LValue result = m_out.neg(value);
            speculate(NegativeZero, noValue(), 0, m_out.isZero64(result));
            setInt52(result);
            break;
        }
            
        case NumberUse: {
            setDouble(m_out.doubleNeg(lowDouble(m_node->child1())));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileBitAnd()
    {
        setInt32(m_out.bitAnd(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileBitOr()
    {
        setInt32(m_out.bitOr(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileBitXor()
    {
        setInt32(m_out.bitXor(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileBitRShift()
    {
        setInt32(m_out.aShr(
            lowInt32(m_node->child1()),
            m_out.bitAnd(lowInt32(m_node->child2()), m_out.constInt32(31))));
    }
    
    void compileBitLShift()
    {
        setInt32(m_out.shl(
            lowInt32(m_node->child1()),
            m_out.bitAnd(lowInt32(m_node->child2()), m_out.constInt32(31))));
    }
    
    void compileBitURShift()
    {
        setInt32(m_out.lShr(
            lowInt32(m_node->child1()),
            m_out.bitAnd(lowInt32(m_node->child2()), m_out.constInt32(31))));
    }
    
    void compileUInt32ToNumber()
    {
        LValue value = lowInt32(m_node->child1());

        if (!nodeCanSpeculateInt32(m_node->arithNodeFlags())) {
            setDouble(m_out.unsignedToDouble(value));
            return;
        }
        
        speculate(Overflow, noValue(), 0, m_out.lessThan(value, m_out.int32Zero));
        setInt32(value);
    }
    
    void compileInt32ToDouble()
    {
        setDouble(lowDouble(m_node->child1()));
    }
    
    void compileCheckStructure()
    {
        LValue cell = lowCell(m_node->child1());
        
        ExitKind exitKind;
        if (m_node->child1()->op() == WeakTIConstant)
            exitKind = BadWeakConstantCache;
        else
            exitKind = BadCache;
        
        LValue structure = m_out.loadPtr(cell, m_heaps.JSCell_structure);
        
        if (m_node->structureSet().size() == 1) {
            speculate(
                exitKind, jsValueValue(cell), 0,
                m_out.notEqual(structure, weakPointer(m_node->structureSet()[0])));
            return;
        }
        
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CheckStructure continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(continuation);
        for (unsigned i = 0; i < m_node->structureSet().size() - 1; ++i) {
            LBasicBlock nextStructure = FTL_NEW_BLOCK(m_out, ("CheckStructure nextStructure"));
            m_out.branch(
                m_out.equal(structure, weakPointer(m_node->structureSet()[i])),
                continuation, nextStructure);
            m_out.appendTo(nextStructure);
        }
        
        speculate(
            exitKind, jsValueValue(cell), 0,
            m_out.notEqual(structure, weakPointer(m_node->structureSet().last())));
        
        m_out.jump(continuation);
        m_out.appendTo(continuation, lastNext);
    }
    
    void compileStructureTransitionWatchpoint()
    {
        addWeakReference(m_node->structure());
        speculateCell(m_node->child1());
    }
    
    void compileCheckFunction()
    {
        LValue cell = lowCell(m_node->child1());
        
        speculate(
            BadFunction, jsValueValue(cell), m_node->child1().node(),
            m_out.notEqual(cell, weakPointer(m_node->function())));
    }
    
    void compileArrayifyToStructure()
    {
        LValue cell = lowCell(m_node->child1());
        LValue property = !!m_node->child2() ? lowInt32(m_node->child2()) : 0;
        
        LBasicBlock unexpectedStructure = FTL_NEW_BLOCK(m_out, ("ArrayifyToStructure unexpected structure"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArrayifyToStructure continuation"));
        
        LValue structure = m_out.loadPtr(cell, m_heaps.JSCell_structure);
        
        m_out.branch(
            m_out.notEqual(structure, weakPointer(m_node->structure())),
            unexpectedStructure, continuation);
        
        LBasicBlock lastNext = m_out.appendTo(unexpectedStructure, continuation);
        
        if (property) {
            switch (m_node->arrayMode().type()) {
            case Array::Int32:
            case Array::Double:
            case Array::Contiguous:
                speculate(
                    Uncountable, noValue(), 0,
                    m_out.aboveOrEqual(property, m_out.constInt32(MIN_SPARSE_ARRAY_INDEX)));
                break;
            default:
                break;
            }
        }
        
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
            vmCall(m_out.operation(operationEnsureInt32), m_callFrame, cell);
            break;
        case Array::Double:
            vmCall(m_out.operation(operationEnsureDouble), m_callFrame, cell);
            break;
        case Array::Contiguous:
            if (m_node->arrayMode().conversion() == Array::RageConvert)
                vmCall(m_out.operation(operationRageEnsureContiguous), m_callFrame, cell);
            else
                vmCall(m_out.operation(operationEnsureContiguous), m_callFrame, cell);
            break;
        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage:
            vmCall(m_out.operation(operationEnsureArrayStorage), m_callFrame, cell);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        
        structure = m_out.loadPtr(cell, m_heaps.JSCell_structure);
        speculate(
            BadIndexingType, jsValueValue(cell), 0,
            m_out.notEqual(structure, weakPointer(m_node->structure())));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void compilePutStructure()
    {
        m_ftlState.jitCode->common.notifyCompilingStructureTransition(m_graph.m_plan, codeBlock(), m_node);
        
        m_out.store64(
            m_out.constIntPtr(m_node->structureTransitionData().newStructure),
            lowCell(m_node->child1()), m_heaps.JSCell_structure);
    }
    
    void compilePhantomPutStructure()
    {
        m_ftlState.jitCode->common.notifyCompilingStructureTransition(m_graph.m_plan, codeBlock(), m_node);
    }
    
    void compileGetById()
    {
        // UntypedUse is a bit harder to reason about and I'm not sure how best to do it, yet.
        // Basically we need to emit a cell branch that takes you to the slow path, but the slow
        // path is generated by the IC generator so we can't jump to it from here. And the IC
        // generator currently doesn't know how to emit such a branch. So, for now, we just
        // restrict this to CellUse.
        ASSERT(m_node->child1().useKind() == CellUse);

        LValue base = lowCell(m_node->child1());
        StringImpl* uid = m_graph.identifiers()[m_node->identifierNumber()];

        // Arguments: id, bytes, target, numArgs, args...
        unsigned stackmapID = m_stackmapIDs++;
        
        if (Options::verboseCompilation())
            dataLog("    Emitting GetById patchpoint with stackmap #", stackmapID, "\n");
        
        LValue call = m_out.call(
            m_out.patchpointInt64Intrinsic(),
            m_out.constInt32(stackmapID), m_out.constInt32(sizeOfGetById()),
            constNull(m_out.ref8), m_out.constInt32(2), m_callFrame, base);
        setInstructionCallingConvention(call, LLVMAnyRegCallConv);
        setTiValue(call);
        
        m_ftlState.getByIds.append(GetByIdDescriptor(stackmapID, m_node->codeOrigin, uid));
    }
    
    void compilePutById()
    {
        // See above; CellUse is easier so we do only that for now.
        ASSERT(m_node->child1().useKind() == CellUse);
        
        LValue base = lowCell(m_node->child1());
        LValue value = lowTiValue(m_node->child2());
        StringImpl* uid = m_graph.identifiers()[m_node->identifierNumber()];

        // Arguments: id, bytes, target, numArgs, args...
        unsigned stackmapID = m_stackmapIDs++;

        if (Options::verboseCompilation())
            dataLog("    Emitting PutById patchpoint with stackmap #", stackmapID, "\n");
        
        LValue call = m_out.call(
            m_out.patchpointVoidIntrinsic(),
            m_out.constInt32(stackmapID), m_out.constInt32(sizeOfPutById()),
            constNull(m_out.ref8), m_out.constInt32(3), m_callFrame, base, value);
        setInstructionCallingConvention(call, LLVMAnyRegCallConv);
        
        m_ftlState.putByIds.append(PutByIdDescriptor(
            stackmapID, m_node->codeOrigin, uid,
            m_graph.executableFor(m_node->codeOrigin)->ecmaMode(),
            m_node->op() == PutByIdDirect ? Direct : NotDirect));
    }
    
    void compileGetButterfly()
    {
        setStorage(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.JSObject_butterfly));
    }
    
    void compileConstantStoragePointer()
    {
        setStorage(m_out.constIntPtr(m_node->storagePointer()));
    }
    
    void compileGetIndexedPropertyStorage()
    {
        LValue cell = lowCell(m_node->child1());
        
        if (m_node->arrayMode().type() == Array::String) {
            LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("GetIndexedPropertyStorage String slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetIndexedPropertyStorage String continuation"));
            
            ValueFromBlock fastResult = m_out.anchor(
                m_out.loadPtr(cell, m_heaps.JSString_value));
            
            m_out.branch(m_out.notNull(fastResult.value()), continuation, slowPath);
            
            LBasicBlock lastNext = m_out.appendTo(slowPath, continuation);
            
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(m_out.operation(operationResolveRope), m_callFrame, cell));
            
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            
            setStorage(m_out.loadPtr(m_out.phi(m_out.intPtr, fastResult, slowResult), m_heaps.StringImpl_data));
            return;
        }
        
        setStorage(m_out.loadPtr(cell, m_heaps.JSArrayBufferView_vector));
    }
    
    void compileCheckArray()
    {
        Edge edge = m_node->child1();
        LValue cell = lowCell(edge);
        
        if (m_node->arrayMode().alreadyChecked(m_graph, m_node, m_state.forNode(edge)))
            return;
        
        speculate(
            BadIndexingType, jsValueValue(cell), 0,
            m_out.bitNot(isArrayType(cell, m_node->arrayMode())));
    }
    
    void compileGetArrayLength()
    {
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            setInt32(m_out.load32(lowStorage(m_node->child2()), m_heaps.Butterfly_publicLength));
            return;
        }
            
        case Array::String: {
            LValue string = lowCell(m_node->child1());
            setInt32(m_out.load32(string, m_heaps.JSString_length));
            return;
        }
            
        default:
            if (isTypedView(m_node->arrayMode().typedArrayType())) {
                setInt32(
                    m_out.load32(lowCell(m_node->child1()), m_heaps.JSArrayBufferView_length));
                return;
            }
            
            RELEASE_ASSERT_NOT_REACHED();
            return;
        }
    }
    
    void compileCheckInBounds()
    {
        speculate(
            OutOfBounds, noValue(), 0,
            m_out.aboveOrEqual(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileGetByVal()
    {
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            IndexedAbstractHeap& heap = m_node->arrayMode().type() == Array::Int32 ?
                m_heaps.indexedInt32Properties : m_heaps.indexedContiguousProperties;
            
            if (m_node->arrayMode().isInBounds()) {
                LValue result = m_out.load64(baseIndex(heap, storage, index, m_node->child2()));
                speculate(LoadFromHole, noValue(), 0, m_out.isZero64(result));
                setTiValue(result);
                return;
            }
            
            LValue base = lowCell(m_node->child1());
            
            LBasicBlock fastCase = FTL_NEW_BLOCK(m_out, ("GetByVal int/contiguous fast case"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("GetByVal int/contiguous slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal int/contiguous continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(
                    index, m_out.load32(storage, m_heaps.Butterfly_publicLength)),
                slowCase, fastCase);
            
            LBasicBlock lastNext = m_out.appendTo(fastCase, slowCase);
            
            ValueFromBlock fastResult = m_out.anchor(
                m_out.load64(baseIndex(heap, storage, index, m_node->child2())));
            m_out.branch(m_out.isZero64(fastResult.value()), slowCase, continuation);
            
            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(m_out.operation(operationGetByValArrayInt), m_callFrame, base, index));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setTiValue(m_out.phi(m_out.int64, fastResult, slowResult));
            return;
        }
            
        case Array::Double: {
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            IndexedAbstractHeap& heap = m_heaps.indexedDoubleProperties;
            
            if (m_node->arrayMode().isInBounds()) {
                LValue result = m_out.loadDouble(
                    baseIndex(heap, storage, index, m_node->child2()));
                
                if (!m_node->arrayMode().isSaneChain()) {
                    speculate(
                        LoadFromHole, noValue(), 0,
                        m_out.doubleNotEqualOrUnordered(result, result));
                }
                setDouble(result);
                break;
            }
            
            LValue base = lowCell(m_node->child1());
            
            LBasicBlock inBounds = FTL_NEW_BLOCK(m_out, ("GetByVal double in bounds"));
            LBasicBlock boxPath = FTL_NEW_BLOCK(m_out, ("GetByVal double boxing"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("GetByVal double slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal double continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(
                    index, m_out.load32(storage, m_heaps.Butterfly_publicLength)),
                slowCase, inBounds);
            
            LBasicBlock lastNext = m_out.appendTo(inBounds, boxPath);
            LValue doubleValue = m_out.loadDouble(
                baseIndex(heap, storage, index, m_node->child2()));
            m_out.branch(
                m_out.doubleNotEqualOrUnordered(doubleValue, doubleValue), slowCase, boxPath);
            
            m_out.appendTo(boxPath, slowCase);
            ValueFromBlock fastResult = m_out.anchor(boxDouble(doubleValue));
            m_out.jump(continuation);
            
            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(m_out.operation(operationGetByValArrayInt), m_callFrame, base, index));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setTiValue(m_out.phi(m_out.int64, fastResult, slowResult));
            return;
        }
            
        case Array::Generic: {
            setTiValue(vmCall(
                m_out.operation(operationGetByVal), m_callFrame,
                lowTiValue(m_node->child1()), lowTiValue(m_node->child2())));
            return;
        }
            
        case Array::String: {
            compileStringCharAt();
            return;
        }
            
        default: {
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            TypedArrayType type = m_node->arrayMode().typedArrayType();
            
            if (isTypedView(type)) {
                TypedPointer pointer = TypedPointer(
                    m_heaps.typedArrayProperties,
                    m_out.add(
                        storage,
                        m_out.shl(
                            m_out.zeroExt(index, m_out.intPtr),
                            m_out.constIntPtr(logElementSize(type)))));
                
                if (isInt(type)) {
                    LValue result;
                    switch (elementSize(type)) {
                    case 1:
                        result = m_out.load8(pointer);
                        break;
                    case 2:
                        result = m_out.load16(pointer);
                        break;
                    case 4:
                        result = m_out.load32(pointer);
                        break;
                    default:
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                    
                    if (elementSize(type) < 4) {
                        if (isSigned(type))
                            result = m_out.signExt(result, m_out.int32);
                        else
                            result = m_out.zeroExt(result, m_out.int32);
                        setInt32(result);
                        return;
                    }
                    
                    if (isSigned(type)) {
                        setInt32(result);
                        return;
                    }
                    
                    if (m_node->shouldSpeculateInt32()) {
                        speculate(
                            Overflow, noValue(), 0, m_out.lessThan(result, m_out.int32Zero));
                        setInt32(result);
                        return;
                    }
                    
                    setDouble(m_out.unsignedToFP(result, m_out.doubleType));
                    return;
                }
            
                ASSERT(isFloat(type));
                
                LValue result;
                switch (type) {
                case TypeFloat32:
                    result = m_out.fpCast(m_out.loadFloat(pointer), m_out.doubleType);
                    break;
                case TypeFloat64:
                    result = m_out.loadDouble(pointer);
                    break;
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                }
                
                result = m_out.select(
                    m_out.doubleEqual(result, result), result, m_out.constDouble(QNaN));
                setDouble(result);
                return;
            }
            
            RELEASE_ASSERT_NOT_REACHED();
            return;
        } }
    }
    
    void compilePutByVal()
    {
        Edge child1 = m_graph.varArgChild(m_node, 0);
        Edge child2 = m_graph.varArgChild(m_node, 1);
        Edge child3 = m_graph.varArgChild(m_node, 2);
        Edge child4 = m_graph.varArgChild(m_node, 3);
        
        switch (m_node->arrayMode().type()) {
        case Array::Generic: {
            V_JITOperation_EJJJ operation;
            if (m_node->op() == PutByValDirect) {
                if (m_graph.isStrictModeFor(m_node->codeOrigin))
                    operation = operationPutByValDirectStrict;
                else
                    operation = operationPutByValDirectNonStrict;
            } else {
                if (m_graph.isStrictModeFor(m_node->codeOrigin))
                    operation = operationPutByValStrict;
                else
                    operation = operationPutByValNonStrict;
            }
                
            vmCall(
                m_out.operation(operation), m_callFrame,
                lowTiValue(child1), lowTiValue(child2), lowTiValue(child3));
            return;
        }
            
        default:
            break;
        }

        LValue base = lowCell(child1);
        LValue index = lowInt32(child2);
        LValue storage = lowStorage(child4);
        
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal continuation"));
            LBasicBlock outerLastNext = m_out.appendTo(m_out.m_block, continuation);
            
            switch (m_node->arrayMode().type()) {
            case Array::Int32:
            case Array::Contiguous: {
                LValue value = lowTiValue(child3, ManualOperandSpeculation);
                
                if (m_node->arrayMode().type() == Array::Int32)
                    FTL_TYPE_CHECK(jsValueValue(value), child3, SpecInt32, isNotInt32(value));
                
                TypedPointer elementPointer = m_out.baseIndex(
                    m_node->arrayMode().type() == Array::Int32 ?
                    m_heaps.indexedInt32Properties : m_heaps.indexedContiguousProperties,
                    storage, m_out.zeroExt(index, m_out.intPtr),
                    m_state.forNode(child2).m_value);
                
                if (m_node->op() == PutByValAlias) {
                    m_out.store64(value, elementPointer);
                    break;
                }
                
                contiguousPutByValOutOfBounds(
                    codeBlock()->isStrictMode()
                    ? operationPutByValBeyondArrayBoundsStrict
                    : operationPutByValBeyondArrayBoundsNonStrict,
                    base, storage, index, value, continuation);
                
                m_out.store64(value, elementPointer);
                break;
            }
                
            case Array::Double: {
                LValue value = lowDouble(child3);
                
                FTL_TYPE_CHECK(
                    doubleValue(value), child3, SpecFullRealNumber,
                    m_out.doubleNotEqualOrUnordered(value, value));
                
                TypedPointer elementPointer = m_out.baseIndex(
                    m_heaps.indexedDoubleProperties,
                    storage, m_out.zeroExt(index, m_out.intPtr),
                    m_state.forNode(child2).m_value);
                
                if (m_node->op() == PutByValAlias) {
                    m_out.storeDouble(value, elementPointer);
                    break;
                }
                
                contiguousPutByValOutOfBounds(
                    codeBlock()->isStrictMode()
                    ? operationPutDoubleByValBeyondArrayBoundsStrict
                    : operationPutDoubleByValBeyondArrayBoundsNonStrict,
                    base, storage, index, value, continuation);
                
                m_out.storeDouble(value, elementPointer);
                break;
            }
                
            default:
                RELEASE_ASSERT_NOT_REACHED();
            }

            m_out.jump(continuation);
            m_out.appendTo(continuation, outerLastNext);
            return;
        }
            
        default:
            TypedArrayType type = m_node->arrayMode().typedArrayType();
            
            if (isTypedView(type)) {
                TypedPointer pointer = TypedPointer(
                    m_heaps.typedArrayProperties,
                    m_out.add(
                        storage,
                        m_out.shl(
                            m_out.zeroExt(index, m_out.intPtr),
                            m_out.constIntPtr(logElementSize(type)))));
                
                if (isInt(type)) {
                    LValue intValue;
                    switch (child3.useKind()) {
                    case MachineIntUse:
                    case Int32Use: {
                        if (child3.useKind() == Int32Use)
                            intValue = lowInt32(child3);
                        else
                            intValue = m_out.castToInt32(lowStrictInt52(child3));

                        if (isClamped(type)) {
                            ASSERT(elementSize(type) == 1);
                            
                            LBasicBlock atLeastZero = FTL_NEW_BLOCK(m_out, ("PutByVal int clamp atLeastZero"));
                            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal int clamp continuation"));
                            
                            Vector<ValueFromBlock, 2> intValues;
                            intValues.append(m_out.anchor(m_out.int32Zero));
                            m_out.branch(
                                m_out.lessThan(intValue, m_out.int32Zero),
                                continuation, atLeastZero);
                            
                            LBasicBlock lastNext = m_out.appendTo(atLeastZero, continuation);
                            
                            intValues.append(m_out.anchor(m_out.select(
                                m_out.greaterThan(intValue, m_out.constInt32(255)),
                                m_out.constInt32(255),
                                intValue)));
                            m_out.jump(continuation);
                            
                            m_out.appendTo(continuation, lastNext);
                            intValue = m_out.phi(m_out.int32, intValues);
                        }
                        break;
                    }
                        
                    case NumberUse: {
                        LValue doubleValue = lowDouble(child3);
                        
                        if (isClamped(type)) {
                            ASSERT(elementSize(type) == 1);
                            
                            LBasicBlock atLeastZero = FTL_NEW_BLOCK(m_out, ("PutByVal double clamp atLeastZero"));
                            LBasicBlock withinRange = FTL_NEW_BLOCK(m_out, ("PutByVal double clamp withinRange"));
                            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal double clamp continuation"));
                            
                            Vector<ValueFromBlock, 3> intValues;
                            intValues.append(m_out.anchor(m_out.int32Zero));
                            m_out.branch(
                                m_out.doubleLessThanOrUnordered(doubleValue, m_out.doubleZero),
                                continuation, atLeastZero);
                            
                            LBasicBlock lastNext = m_out.appendTo(atLeastZero, withinRange);
                            intValues.append(m_out.anchor(m_out.constInt32(255)));
                            m_out.branch(
                                m_out.doubleGreaterThan(doubleValue, m_out.constDouble(255)),
                                continuation, withinRange);
                            
                            m_out.appendTo(withinRange, continuation);
                            intValues.append(m_out.anchor(m_out.fpToInt32(doubleValue)));
                            m_out.jump(continuation);
                            
                            m_out.appendTo(continuation, lastNext);
                            intValue = m_out.phi(m_out.int32, intValues);
                        } else
                            intValue = doubleToInt32(doubleValue);
                        break;
                    }
                        
                    default:
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                    
                    switch (elementSize(type)) {
                    case 1:
                        m_out.store8(m_out.intCast(intValue, m_out.int8), pointer);
                        break;
                    case 2:
                        m_out.store16(m_out.intCast(intValue, m_out.int16), pointer);
                        break;
                    case 4:
                        m_out.store32(intValue, pointer);
                        break;
                    default:
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                    
                    return;
                }
                
                ASSERT(isFloat(type));
                
                LValue value = lowDouble(child3);
                switch (type) {
                case TypeFloat32:
                    m_out.storeFloat(m_out.fpCast(value, m_out.floatType), pointer);
                    break;
                case TypeFloat64:
                    m_out.storeDouble(value, pointer);
                    break;
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                }
                return;
            }
            
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    void compileNewObject()
    {
        Structure* structure = m_node->structure();
        size_t allocationSize = JSFinalObject::allocationSize(structure->inlineCapacity());
        MarkedAllocator* allocator = &vm().heap.allocatorForObjectWithoutDestructor(allocationSize);
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("NewObject slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("NewObject continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        ValueFromBlock fastResult = m_out.anchor(allocateObject(
            m_out.constIntPtr(allocator), m_out.constIntPtr(structure), m_out.intPtrZero, slowPath));
        
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        ValueFromBlock slowResult = m_out.anchor(vmCall(
            m_out.operation(operationNewObject), m_callFrame, m_out.constIntPtr(structure)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setTiValue(m_out.phi(m_out.intPtr, fastResult, slowResult));
    }
    
    void compileNewArray()
    {
        // First speculate appropriately on all of the children. Do this unconditionally up here
        // because some of the slow paths may otherwise forget to do it. It's sort of arguable
        // that doing the speculations up here might be unprofitable for RA - so we can consider
        // sinking this to below the allocation fast path if we find that this has a lot of
        // register pressure.
        for (unsigned operandIndex = 0; operandIndex < m_node->numChildren(); ++operandIndex)
            speculate(m_graph.varArgChild(m_node, operandIndex));
        
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->codeOrigin);
        Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(
            m_node->indexingType());
        
        RELEASE_ASSERT(structure->indexingType() == m_node->indexingType());
        
        if (!globalObject->isHavingABadTime() && !hasArrayStorage(m_node->indexingType())) {
            unsigned numElements = m_node->numChildren();
            
            ArrayValues arrayValues = allocateJSArray(structure, numElements);
            
            for (unsigned operandIndex = 0; operandIndex < m_node->numChildren(); ++operandIndex) {
                Edge edge = m_graph.varArgChild(m_node, operandIndex);
                
                switch (m_node->indexingType()) {
                case ALL_BLANK_INDEXING_TYPES:
                case ALL_UNDECIDED_INDEXING_TYPES:
                    CRASH();
                    break;
                    
                case ALL_DOUBLE_INDEXING_TYPES:
                    m_out.storeDouble(
                        lowDouble(edge),
                        arrayValues.butterfly, m_heaps.indexedDoubleProperties[operandIndex]);
                    break;
                    
                case ALL_INT32_INDEXING_TYPES:
                case ALL_CONTIGUOUS_INDEXING_TYPES:
                    m_out.store64(
                        lowTiValue(edge, ManualOperandSpeculation),
                        arrayValues.butterfly,
                        m_heaps.forIndexingType(m_node->indexingType())->at(operandIndex));
                    break;
                    
                default:
                    CRASH();
                }
            }
            
            setTiValue(arrayValues.array);
            return;
        }
        
        if (!m_node->numChildren()) {
            setTiValue(vmCall(
                m_out.operation(operationNewEmptyArray), m_callFrame,
                m_out.constIntPtr(structure)));
            return;
        }
        
        size_t scratchSize = sizeof(EncodedTiValue) * m_node->numChildren();
        ASSERT(scratchSize);
        ScratchBuffer* scratchBuffer = vm().scratchBufferForSize(scratchSize);
        EncodedTiValue* buffer = static_cast<EncodedTiValue*>(scratchBuffer->dataBuffer());
        
        for (unsigned operandIndex = 0; operandIndex < m_node->numChildren(); ++operandIndex) {
            Edge edge = m_graph.varArgChild(m_node, operandIndex);
            m_out.store64(
                lowTiValue(edge, ManualOperandSpeculation),
                m_out.absolute(buffer + operandIndex));
        }
        
        m_out.storePtr(
            m_out.constIntPtr(scratchSize), m_out.absolute(scratchBuffer->activeLengthPtr()));
        
        LValue result = vmCall(
            m_out.operation(operationNewArray), m_callFrame,
            m_out.constIntPtr(structure), m_out.constIntPtr(buffer),
            m_out.constIntPtr(m_node->numChildren()));
        
        m_out.storePtr(m_out.intPtrZero, m_out.absolute(scratchBuffer->activeLengthPtr()));
        
        setTiValue(result);
    }
    
    void compileNewArrayBuffer()
    {
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->codeOrigin);
        Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(
            m_node->indexingType());
        
        RELEASE_ASSERT(structure->indexingType() == m_node->indexingType());
        
        if (!globalObject->isHavingABadTime() && !hasArrayStorage(m_node->indexingType())) {
            unsigned numElements = m_node->numConstants();
            
            ArrayValues arrayValues = allocateJSArray(structure, numElements);
            
            TiValue* data = codeBlock()->constantBuffer(m_node->startConstant());
            for (unsigned index = 0; index < m_node->numConstants(); ++index) {
                int64_t value;
                if (hasDouble(m_node->indexingType()))
                    value = bitwise_cast<int64_t>(data[index].asNumber());
                else
                    value = TiValue::encode(data[index]);
                
                m_out.store64(
                    m_out.constInt64(value),
                    arrayValues.butterfly,
                    m_heaps.forIndexingType(m_node->indexingType())->at(index));
            }
            
            setTiValue(arrayValues.array);
            return;
        }
        
        setTiValue(vmCall(
            m_out.operation(operationNewArrayBuffer), m_callFrame,
            m_out.constIntPtr(structure), m_out.constIntPtr(m_node->startConstant()),
            m_out.constIntPtr(m_node->numConstants())));
    }
    
    void compileAllocatePropertyStorage()
    {
        StructureTransitionData& data = m_node->structureTransitionData();
        
        LValue object = lowCell(m_node->child1());
        
        if (data.previousStructure->couldHaveIndexingHeader()) {
            setStorage(vmCall(
                m_out.operation(
                    operationReallocateButterflyToHavePropertyStorageWithInitialCapacity),
                m_callFrame, object));
            return;
        }
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("AllocatePropertyStorage slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("AllocatePropertyStorage continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        LValue endOfStorage = allocateBasicStorageAndGetEnd(
            m_out.constIntPtr(initialOutOfLineCapacity * sizeof(TiValue)), slowPath);
        
        ValueFromBlock fastButterfly = m_out.anchor(
            m_out.add(m_out.constIntPtr(sizeof(IndexingHeader)), endOfStorage));
        
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        ValueFromBlock slowButterfly = m_out.anchor(vmCall(
            m_out.operation(operationAllocatePropertyStorageWithInitialCapacity), m_callFrame));
        
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        LValue result = m_out.phi(m_out.intPtr, fastButterfly, slowButterfly);
        m_out.storePtr(result, object, m_heaps.JSObject_butterfly);
        
        setStorage(result);
    }
    
    void compileStringCharAt()
    {
        LValue base = lowCell(m_node->child1());
        LValue index = lowInt32(m_node->child2());
        LValue storage = lowStorage(m_node->child3());
            
        LBasicBlock fastPath = FTL_NEW_BLOCK(m_out, ("GetByVal String fast path"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("GetByVal String slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal String continuation"));
            
        m_out.branch(
            m_out.aboveOrEqual(
                index, m_out.load32(base, m_heaps.JSString_length)),
            slowPath, fastPath);
            
        LBasicBlock lastNext = m_out.appendTo(fastPath, slowPath);
            
        LValue stringImpl = m_out.loadPtr(base, m_heaps.JSString_value);
            
        LBasicBlock is8Bit = FTL_NEW_BLOCK(m_out, ("GetByVal String 8-bit case"));
        LBasicBlock is16Bit = FTL_NEW_BLOCK(m_out, ("GetByVal String 16-bit case"));
        LBasicBlock bitsContinuation = FTL_NEW_BLOCK(m_out, ("GetByVal String bitness continuation"));
        LBasicBlock bigCharacter = FTL_NEW_BLOCK(m_out, ("GetByVal String big character"));
            
        m_out.branch(
            m_out.testIsZero32(
                m_out.load32(stringImpl, m_heaps.StringImpl_hashAndFlags),
                m_out.constInt32(StringImpl::flagIs8Bit())),
            is16Bit, is8Bit);
            
        m_out.appendTo(is8Bit, is16Bit);
            
        ValueFromBlock char8Bit = m_out.anchor(m_out.zeroExt(
            m_out.load8(m_out.baseIndex(
                m_heaps.characters8,
                storage, m_out.zeroExt(index, m_out.intPtr),
                m_state.forNode(m_node->child2()).m_value)),
            m_out.int32));
        m_out.jump(bitsContinuation);
            
        m_out.appendTo(is16Bit, bigCharacter);
            
        ValueFromBlock char16Bit = m_out.anchor(m_out.zeroExt(
            m_out.load16(m_out.baseIndex(
                m_heaps.characters16,
                storage, m_out.zeroExt(index, m_out.intPtr),
                m_state.forNode(m_node->child2()).m_value)),
            m_out.int32));
        m_out.branch(m_out.aboveOrEqual(char16Bit.value(), m_out.constInt32(0x100)), bigCharacter, bitsContinuation);
            
        m_out.appendTo(bigCharacter, bitsContinuation);
            
        Vector<ValueFromBlock, 4> results;
        results.append(m_out.anchor(vmCall(
            m_out.operation(operationSingleCharacterString),
            m_callFrame, char16Bit.value())));
        m_out.jump(continuation);
            
        m_out.appendTo(bitsContinuation, slowPath);
            
        LValue character = m_out.phi(m_out.int32, char8Bit, char16Bit);
            
        LValue smallStrings = m_out.constIntPtr(vm().smallStrings.singleCharacterStrings());
            
        results.append(m_out.anchor(m_out.loadPtr(m_out.baseIndex(
            m_heaps.singleCharacterStrings, smallStrings,
            m_out.zeroExt(character, m_out.intPtr)))));
        m_out.jump(continuation);
            
        m_out.appendTo(slowPath, continuation);
            
        if (m_node->arrayMode().isInBounds()) {
            speculate(OutOfBounds, noValue(), 0, m_out.booleanTrue);
            results.append(m_out.anchor(m_out.intPtrZero));
        } else {
            JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->codeOrigin);
                
            if (globalObject->stringPrototypeChainIsSane()) {
                LBasicBlock negativeIndex = FTL_NEW_BLOCK(m_out, ("GetByVal String negative index"));
                    
                results.append(m_out.anchor(m_out.constInt64(TiValue::encode(jsUndefined()))));
                m_out.branch(m_out.lessThan(index, m_out.int32Zero), negativeIndex, continuation);
                    
                m_out.appendTo(negativeIndex, continuation);
            }
                
            results.append(m_out.anchor(vmCall(
                m_out.operation(operationGetByValStringInt), m_callFrame, base, index)));
        }
            
        m_out.jump(continuation);
            
        m_out.appendTo(continuation, lastNext);
        setTiValue(m_out.phi(m_out.int64, results));
    }
    
    void compileStringCharCodeAt()
    {
        LBasicBlock is8Bit = FTL_NEW_BLOCK(m_out, ("StringCharCodeAt 8-bit case"));
        LBasicBlock is16Bit = FTL_NEW_BLOCK(m_out, ("StringCharCodeAt 16-bit case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("StringCharCodeAt continuation"));

        LValue base = lowCell(m_node->child1());
        LValue index = lowInt32(m_node->child2());
        LValue storage = lowStorage(m_node->child3());
        
        speculate(
            Uncountable, noValue(), 0,
            m_out.aboveOrEqual(index, m_out.load32(base, m_heaps.JSString_length)));
        
        LValue stringImpl = m_out.loadPtr(base, m_heaps.JSString_value);
        
        m_out.branch(
            m_out.testIsZero32(
                m_out.load32(stringImpl, m_heaps.StringImpl_hashAndFlags),
                m_out.constInt32(StringImpl::flagIs8Bit())),
            is16Bit, is8Bit);
            
        LBasicBlock lastNext = m_out.appendTo(is8Bit, is16Bit);
            
        ValueFromBlock char8Bit = m_out.anchor(m_out.zeroExt(
            m_out.load8(m_out.baseIndex(
                m_heaps.characters8,
                storage, m_out.zeroExt(index, m_out.intPtr),
                m_state.forNode(m_node->child2()).m_value)),
            m_out.int32));
        m_out.jump(continuation);
            
        m_out.appendTo(is16Bit, continuation);
            
        ValueFromBlock char16Bit = m_out.anchor(m_out.zeroExt(
            m_out.load16(m_out.baseIndex(
                m_heaps.characters16,
                storage, m_out.zeroExt(index, m_out.intPtr),
                m_state.forNode(m_node->child2()).m_value)),
            m_out.int32));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        setInt32(m_out.phi(m_out.int32, char8Bit, char16Bit));
    }
    
    void compileGetByOffset()
    {
        StorageAccessData& data =
            m_graph.m_storageAccessData[m_node->storageAccessDataIndex()];
        
        setTiValue(
            m_out.load64(
                m_out.address(
                    m_heaps.properties[data.identifierNumber],
                    lowStorage(m_node->child1()),
                    offsetRelativeToBase(data.offset))));
    }
    
    void compilePutByOffset()
    {
        StorageAccessData& data =
            m_graph.m_storageAccessData[m_node->storageAccessDataIndex()];
        
        m_out.store64(
            lowTiValue(m_node->child3()),
            m_out.address(
                m_heaps.properties[data.identifierNumber],
                lowStorage(m_node->child1()),
                offsetRelativeToBase(data.offset)));
    }
    
    void compileGetGlobalVar()
    {
        setTiValue(m_out.load64(m_out.absolute(m_node->registerPointer())));
    }
    
    void compilePutGlobalVar()
    {
        m_out.store64(
            lowTiValue(m_node->child1()), m_out.absolute(m_node->registerPointer()));
    }
    
    void compileNotifyWrite()
    {
        VariableWatchpointSet* set = m_node->variableWatchpointSet();
        
        LValue value = lowTiValue(m_node->child1());
        
        LBasicBlock isNotInvalidated = FTL_NEW_BLOCK(m_out, ("NotifyWrite not invalidated case"));
        LBasicBlock isClear = FTL_NEW_BLOCK(m_out, ("NotifyWrite clear case"));
        LBasicBlock isWatched = FTL_NEW_BLOCK(m_out, ("NotifyWrite watched case"));
        LBasicBlock invalidate = FTL_NEW_BLOCK(m_out, ("NotifyWrite invalidate case"));
        LBasicBlock invalidateFast = FTL_NEW_BLOCK(m_out, ("NotifyWrite invalidate fast case"));
        LBasicBlock invalidateSlow = FTL_NEW_BLOCK(m_out, ("NotifyWrite invalidate slow case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("NotifyWrite continuation"));
        
        LValue state = m_out.load8(m_out.absolute(set->addressOfState()));
        
        m_out.branch(
            m_out.equal(state, m_out.constInt8(IsInvalidated)),
            continuation, isNotInvalidated);
        
        LBasicBlock lastNext = m_out.appendTo(isNotInvalidated, isClear);

        LValue isClearValue;
        if (set->state() == ClearWatchpoint)
            isClearValue = m_out.equal(state, m_out.constInt8(ClearWatchpoint));
        else
            isClearValue = m_out.booleanFalse;
        m_out.branch(isClearValue, isClear, isWatched);
        
        m_out.appendTo(isClear, isWatched);
        
        m_out.store64(value, m_out.absolute(set->addressOfInferredValue()));
        m_out.store8(m_out.constInt8(IsWatched), m_out.absolute(set->addressOfState()));
        m_out.jump(continuation);
        
        m_out.appendTo(isWatched, invalidate);
        
        m_out.branch(
            m_out.equal(value, m_out.load64(m_out.absolute(set->addressOfInferredValue()))),
            continuation, invalidate);
        
        m_out.appendTo(invalidate, invalidateFast);
        
        m_out.branch(
            m_out.notZero8(m_out.load8(m_out.absolute(set->addressOfSetIsNotEmpty()))),
            invalidateSlow, invalidateFast);
        
        m_out.appendTo(invalidateFast, invalidateSlow);
        
        m_out.store64(
            m_out.constInt64(TiValue::encode(TiValue())),
            m_out.absolute(set->addressOfInferredValue()));
        m_out.store8(m_out.constInt8(IsInvalidated), m_out.absolute(set->addressOfState()));
        m_out.jump(continuation);
        
        m_out.appendTo(invalidateSlow, continuation);
        
        vmCall(m_out.operation(operationInvalidate), m_callFrame, m_out.constIntPtr(set));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void compileGetMyScope()
    {
        setTiValue(m_out.loadPtr(addressFor(
            m_node->codeOrigin.stackOffset() + JSStack::ScopeChain)));
    }
    
    void compileSkipScope()
    {
        setTiValue(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.JSScope_next));
    }
    
    void compileGetClosureRegisters()
    {
        if (WriteBarrierBase<Unknown>* registers = m_graph.tryGetRegisters(m_node->child1().node())) {
            setStorage(m_out.constIntPtr(registers));
            return;
        }
        
        setStorage(m_out.loadPtr(
            lowCell(m_node->child1()), m_heaps.JSVariableObject_registers));
    }
    
    void compileGetClosureVar()
    {
        setTiValue(m_out.load64(
            addressFor(lowStorage(m_node->child1()), m_node->varNumber())));
    }
    
    void compilePutClosureVar()
    {
        m_out.store64(
            lowTiValue(m_node->child3()),
            addressFor(lowStorage(m_node->child2()), m_node->varNumber()));
    }
    
    void compileCompareEq()
    {
        if (m_node->isBinaryUseKind(Int32Use)
            || m_node->isBinaryUseKind(MachineIntUse)
            || m_node->isBinaryUseKind(NumberUse)
            || m_node->isBinaryUseKind(ObjectUse)) {
            compileCompareStrictEq();
            return;
        }
        
        if (m_node->isBinaryUseKind(UntypedUse)) {
            nonSpeculativeCompare(LLVMIntEQ, operationCompareEq);
            return;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    void compileCompareEqConstant()
    {
        ASSERT(m_graph.valueOfJSConstant(m_node->child2().node()).isNull());
        setBoolean(
            equalNullOrUndefined(
                m_node->child1(), AllCellsAreFalse, EqualNullOrUndefined));
    }
    
    void compileCompareStrictEq()
    {
        if (m_node->isBinaryUseKind(Int32Use)) {
            setBoolean(
                m_out.equal(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(MachineIntUse)) {
            Int52Kind kind;
            LValue left = lowWhicheverInt52(m_node->child1(), kind);
            LValue right = lowInt52(m_node->child2(), kind);
            setBoolean(m_out.equal(left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(NumberUse)) {
            setBoolean(
                m_out.doubleEqual(lowDouble(m_node->child1()), lowDouble(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(ObjectUse)) {
            setBoolean(
                m_out.equal(
                    lowNonNullObject(m_node->child1()),
                    lowNonNullObject(m_node->child2())));
            return;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    void compileCompareStrictEqConstant()
    {
        TiValue constant = m_graph.valueOfJSConstant(m_node->child2().node());

        if (constant.isUndefinedOrNull()
            && !masqueradesAsUndefinedWatchpointIsStillValid()) {
            if (constant.isNull()) {
                setBoolean(equalNullOrUndefined(m_node->child1(), AllCellsAreFalse, EqualNull));
                return;
            }
        
            ASSERT(constant.isUndefined());
            setBoolean(equalNullOrUndefined(m_node->child1(), AllCellsAreFalse, EqualUndefined));
            return;
        }
        
        setBoolean(
            m_out.equal(
                lowTiValue(m_node->child1()),
                m_out.constInt64(TiValue::encode(constant))));
    }
    
    void compileCompareLess()
    {
        compare(LLVMIntSLT, LLVMRealOLT, operationCompareLess);
    }
    
    void compileCompareLessEq()
    {
        compare(LLVMIntSLE, LLVMRealOLE, operationCompareLessEq);
    }
    
    void compileCompareGreater()
    {
        compare(LLVMIntSGT, LLVMRealOGT, operationCompareGreater);
    }
    
    void compileCompareGreaterEq()
    {
        compare(LLVMIntSGE, LLVMRealOGE, operationCompareGreaterEq);
    }
    
    void compileLogicalNot()
    {
        setBoolean(m_out.bitNot(boolify(m_node->child1())));
    }
    
    void compileCallOrConstruct()
    {
        // FIXME: This is unacceptably slow.
        // https://bugs.webkit.org/show_bug.cgi?id=113621
        
        J_JITOperation_E function =
            m_node->op() == Call ? operationFTLCall : operationFTLConstruct;
        
        int dummyThisArgument = m_node->op() == Call ? 0 : 1;
        
        int numPassedArgs = m_node->numChildren() - 1;
        
        LValue calleeFrame = m_out.add(
            m_callFrame,
            m_out.constIntPtr(sizeof(Register) * virtualRegisterForLocal(m_graph.frameRegisterCount()).offset()));
        
        m_out.store32(
            m_out.constInt32(numPassedArgs + dummyThisArgument),
            payloadFor(calleeFrame, JSStack::ArgumentCount));
        m_out.store64(m_callFrame, calleeFrame, m_heaps.CallFrame_callerFrame);
        m_out.store64(
            lowTiValue(m_graph.varArgChild(m_node, 0)),
            addressFor(calleeFrame, JSStack::Callee));
        
        for (int i = 0; i < numPassedArgs; ++i) {
            m_out.store64(
                lowTiValue(m_graph.varArgChild(m_node, 1 + i)),
                addressFor(calleeFrame, virtualRegisterForArgument(i + dummyThisArgument).offset()));
        }
        
        setTiValue(vmCall(m_out.operation(function), calleeFrame));
    }
    
    void compileJump()
    {
        m_out.jump(lowBlock(m_node->takenBlock()));
    }
    
    void compileBranch()
    {
        m_out.branch(
            boolify(m_node->child1()),
            lowBlock(m_node->takenBlock()),
            lowBlock(m_node->notTakenBlock()));
    }
    
    void compileSwitch()
    {
        SwitchData* data = m_node->switchData();
        switch (data->kind) {
        case SwitchImm: {
            Vector<ValueFromBlock, 2> intValues;
            LBasicBlock switchOnInts = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm int case"));
            
            LBasicBlock lastNext = m_out.appendTo(m_out.m_block, switchOnInts);
            
            switch (m_node->child1().useKind()) {
            case Int32Use: {
                intValues.append(m_out.anchor(lowInt32(m_node->child1())));
                m_out.jump(switchOnInts);
                break;
            }
                
            case UntypedUse: {
                LBasicBlock isInt = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm is int"));
                LBasicBlock isNotInt = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm is not int"));
                LBasicBlock isDouble = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm is double"));
                
                LValue boxedValue = lowTiValue(m_node->child1());
                m_out.branch(isNotInt32(boxedValue), isNotInt, isInt);
                
                LBasicBlock innerLastNext = m_out.appendTo(isInt, isNotInt);
                
                intValues.append(m_out.anchor(unboxInt32(boxedValue)));
                m_out.jump(switchOnInts);
                
                m_out.appendTo(isNotInt, isDouble);
                m_out.branch(
                    isCellOrMisc(boxedValue), lowBlock(data->fallThrough), isDouble);
                
                m_out.appendTo(isDouble, innerLastNext);
                LValue doubleValue = unboxDouble(boxedValue);
                LValue intInDouble = m_out.fpToInt32(doubleValue);
                intValues.append(m_out.anchor(intInDouble));
                m_out.branch(
                    m_out.doubleEqual(m_out.intToDouble(intInDouble), doubleValue),
                    switchOnInts, lowBlock(data->fallThrough));
                break;
            }
                
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            
            m_out.appendTo(switchOnInts, lastNext);
            buildSwitch(data, m_out.int32, m_out.phi(m_out.int32, intValues));
            return;
        }
        
        case SwitchChar: {
            LValue stringValue;
            
            switch (m_node->child1().useKind()) {
            case StringUse: {
                stringValue = lowString(m_node->child1());
                break;
            }
                
            case UntypedUse: {
                LValue unboxedValue = lowTiValue(m_node->child1());
                
                LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar is cell"));
                LBasicBlock isStringCase = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar is string"));
                
                m_out.branch(
                    isNotCell(unboxedValue), lowBlock(data->fallThrough), isCellCase);
                
                LBasicBlock lastNext = m_out.appendTo(isCellCase, isStringCase);
                LValue cellValue = unboxedValue;
                m_out.branch(isNotString(cellValue), lowBlock(data->fallThrough), isStringCase);
                
                m_out.appendTo(isStringCase, lastNext);
                stringValue = cellValue;
                break;
            }
                
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            
            LBasicBlock lengthIs1 = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar length is 1"));
            LBasicBlock needResolution = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar resolution"));
            LBasicBlock resolved = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar resolved"));
            LBasicBlock is8Bit = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar 8bit"));
            LBasicBlock is16Bit = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar 16bit"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar continuation"));
            
            m_out.branch(
                m_out.notEqual(
                    m_out.load32(stringValue, m_heaps.JSString_length),
                    m_out.int32One),
                lowBlock(data->fallThrough), lengthIs1);
            
            LBasicBlock lastNext = m_out.appendTo(lengthIs1, needResolution);
            Vector<ValueFromBlock, 2> values;
            LValue fastValue = m_out.loadPtr(stringValue, m_heaps.JSString_value);
            values.append(m_out.anchor(fastValue));
            m_out.branch(m_out.isNull(fastValue), needResolution, resolved);
            
            m_out.appendTo(needResolution, resolved);
            values.append(m_out.anchor(
                vmCall(m_out.operation(operationResolveRope), m_callFrame, stringValue)));
            m_out.jump(resolved);
            
            m_out.appendTo(resolved, is8Bit);
            LValue value = m_out.phi(m_out.intPtr, values);
            LValue characterData = m_out.loadPtr(value, m_heaps.StringImpl_data);
            m_out.branch(
                m_out.testNonZero32(
                    m_out.load32(value, m_heaps.StringImpl_hashAndFlags),
                    m_out.constInt32(StringImpl::flagIs8Bit())),
                is8Bit, is16Bit);
            
            Vector<ValueFromBlock, 2> characters;
            m_out.appendTo(is8Bit, is16Bit);
            characters.append(m_out.anchor(
                m_out.zeroExt(m_out.load8(characterData, m_heaps.characters8[0]), m_out.int16)));
            m_out.jump(continuation);
            
            m_out.appendTo(is16Bit, continuation);
            characters.append(m_out.anchor(m_out.load16(characterData, m_heaps.characters16[0])));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            buildSwitch(data, m_out.int16, m_out.phi(m_out.int16, characters));
            return;
        }
        
        case SwitchString:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    void compileReturn()
    {
        // FIXME: have a real epilogue when we switch to using our calling convention.
        // https://bugs.webkit.org/show_bug.cgi?id=113621
        m_out.ret(lowTiValue(m_node->child1()));
    }
    
    void compileForceOSRExit()
    {
        terminate(InadequateCoverage);
    }
    
    void compileInvalidationPoint()
    {
        if (verboseCompilationEnabled())
            dataLog("    Invalidation point with availability: ", m_availability, "\n");
        
        m_ftlState.jitCode->osrExit.append(OSRExit(
            UncountableInvalidation, InvalidValueFormat, MethodOfGettingAValueProfile(),
            m_codeOriginForExitTarget, m_codeOriginForExitProfile,
            m_availability.numberOfArguments(), m_availability.numberOfLocals()));
        m_ftlState.finalizer->osrExit.append(OSRExitCompilationInfo());
        
        OSRExit& exit = m_ftlState.jitCode->osrExit.last();
        OSRExitCompilationInfo& info = m_ftlState.finalizer->osrExit.last();
        
        ExitArgumentList arguments;
        
        buildExitArguments(exit, arguments, FormattedValue(), exit.m_codeOrigin);
        callStackmap(exit, arguments);
        
        info.m_isInvalidationPoint = true;
    }
    
    TypedPointer baseIndex(IndexedAbstractHeap& heap, LValue storage, LValue index, Edge edge)
    {
        return m_out.baseIndex(
            heap, storage, m_out.zeroExt(index, m_out.intPtr),
            m_state.forNode(edge).m_value);
    }
    
    void compare(
        LIntPredicate intCondition, LRealPredicate realCondition,
        S_JITOperation_EJJ helperFunction)
    {
        if (m_node->isBinaryUseKind(Int32Use)) {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            setBoolean(m_out.icmp(intCondition, left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(MachineIntUse)) {
            Int52Kind kind;
            LValue left = lowWhicheverInt52(m_node->child1(), kind);
            LValue right = lowInt52(m_node->child2(), kind);
            setBoolean(m_out.icmp(intCondition, left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(NumberUse)) {
            LValue left = lowDouble(m_node->child1());
            LValue right = lowDouble(m_node->child2());
            setBoolean(m_out.fcmp(realCondition, left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(UntypedUse)) {
            nonSpeculativeCompare(intCondition, helperFunction);
            return;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    void nonSpeculativeCompare(LIntPredicate intCondition, S_JITOperation_EJJ helperFunction)
    {
        LValue left = lowTiValue(m_node->child1());
        LValue right = lowTiValue(m_node->child2());
        
        LBasicBlock leftIsInt = FTL_NEW_BLOCK(m_out, ("CompareEq untyped left is int"));
        LBasicBlock fastPath = FTL_NEW_BLOCK(m_out, ("CompareEq untyped fast path"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("CompareEq untyped slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CompareEq untyped continuation"));
        
        m_out.branch(isNotInt32(left), slowPath, leftIsInt);
        
        LBasicBlock lastNext = m_out.appendTo(leftIsInt, fastPath);
        m_out.branch(isNotInt32(right), slowPath, fastPath);
        
        m_out.appendTo(fastPath, slowPath);
        ValueFromBlock fastResult = m_out.anchor(
            m_out.icmp(intCondition, unboxInt32(left), unboxInt32(right)));
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        ValueFromBlock slowResult = m_out.anchor(m_out.notNull(vmCall(
            m_out.operation(helperFunction), m_callFrame, left, right)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setBoolean(m_out.phi(m_out.boolean, fastResult, slowResult));
    }
    
    LValue allocateCell(LValue allocator, LValue structure, LBasicBlock slowPath)
    {
        LBasicBlock success = FTL_NEW_BLOCK(m_out, ("object allocation success"));
        
        LValue result = m_out.loadPtr(
            allocator, m_heaps.MarkedAllocator_freeListHead);
        
        m_out.branch(m_out.notNull(result), success, slowPath);
        
        m_out.appendTo(success);
        
        m_out.storePtr(
            m_out.loadPtr(result, m_heaps.JSCell_freeListNext),
            allocator, m_heaps.MarkedAllocator_freeListHead);
        
        m_out.storePtr(structure, result, m_heaps.JSCell_structure);
        
        return result;
    }
    
    LValue allocateObject(
        LValue allocator, LValue structure, LValue butterfly, LBasicBlock slowPath)
    {
        LValue result = allocateCell(allocator, structure, slowPath);
        m_out.storePtr(butterfly, result, m_heaps.JSObject_butterfly);
        return result;
    }
    
    template<typename ClassType>
    LValue allocateObject(LValue structure, LValue butterfly, LBasicBlock slowPath)
    {
        MarkedAllocator* allocator;
        size_t size = ClassType::allocationSize(0);
        if (ClassType::needsDestruction && ClassType::hasImmortalStructure)
            allocator = &vm().heap.allocatorForObjectWithImmortalStructureDestructor(size);
        else if (ClassType::needsDestruction)
            allocator = &vm().heap.allocatorForObjectWithNormalDestructor(size);
        else
            allocator = &vm().heap.allocatorForObjectWithoutDestructor(size);
        return allocateObject(m_out.constIntPtr(allocator), structure, butterfly, slowPath);
    }
    
    // Returns a pointer to the end of the allocation.
    LValue allocateBasicStorageAndGetEnd(LValue size, LBasicBlock slowPath)
    {
        CopiedAllocator& allocator = vm().heap.storageAllocator();
        
        LBasicBlock success = FTL_NEW_BLOCK(m_out, ("storage allocation success"));
        
        LValue remaining = m_out.loadPtr(m_out.absolute(&allocator.m_currentRemaining));
        LValue newRemaining = m_out.sub(remaining, size);
        
        m_out.branch(m_out.lessThan(newRemaining, m_out.intPtrZero), slowPath, success);
        
        m_out.appendTo(success);
        
        m_out.storePtr(newRemaining, m_out.absolute(&allocator.m_currentRemaining));
        return m_out.sub(
            m_out.loadPtr(m_out.absolute(&allocator.m_currentPayloadEnd)), newRemaining);
    }
    
    struct ArrayValues {
        ArrayValues()
            : array(0)
            , butterfly(0)
        {
        }
        
        ArrayValues(LValue array, LValue butterfly)
            : array(array)
            , butterfly(butterfly)
        {
        }
        
        LValue array;
        LValue butterfly;
    };
    ArrayValues allocateJSArray(
        Structure* structure, unsigned numElements, LBasicBlock slowPath)
    {
        ASSERT(
            hasUndecided(structure->indexingType())
            || hasInt32(structure->indexingType())
            || hasDouble(structure->indexingType())
            || hasContiguous(structure->indexingType()));
        
        unsigned vectorLength = std::max(BASE_VECTOR_LEN, numElements);
        
        LValue endOfStorage = allocateBasicStorageAndGetEnd(
            m_out.constIntPtr(sizeof(TiValue) * vectorLength + sizeof(IndexingHeader)),
            slowPath);
        
        LValue butterfly = m_out.sub(
            endOfStorage, m_out.constIntPtr(sizeof(TiValue) * vectorLength));
        
        LValue object = allocateObject<JSArray>(
            m_out.constIntPtr(structure), butterfly, slowPath);
        
        m_out.store32(m_out.constInt32(numElements), butterfly, m_heaps.Butterfly_publicLength);
        m_out.store32(m_out.constInt32(vectorLength), butterfly, m_heaps.Butterfly_vectorLength);
        
        if (hasDouble(structure->indexingType())) {
            for (unsigned i = numElements; i < vectorLength; ++i) {
                m_out.store64(
                    m_out.constInt64(bitwise_cast<int64_t>(QNaN)),
                    butterfly, m_heaps.indexedDoubleProperties[i]);
            }
        }
        
        return ArrayValues(object, butterfly);
    }
    
    ArrayValues allocateJSArray(Structure* structure, unsigned numElements)
    {
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("JSArray allocation slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("JSArray allocation continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        ArrayValues fastValues = allocateJSArray(structure, numElements, slowPath);
        ValueFromBlock fastArray = m_out.anchor(fastValues.array);
        ValueFromBlock fastButterfly = m_out.anchor(fastValues.butterfly);
        
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        ValueFromBlock slowArray = m_out.anchor(vmCall(
            m_out.operation(operationNewArrayWithSize), m_callFrame,
            m_out.constIntPtr(structure), m_out.constInt32(numElements)));
        ValueFromBlock slowButterfly = m_out.anchor(
            m_out.loadPtr(slowArray.value(), m_heaps.JSObject_butterfly));

        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        return ArrayValues(
            m_out.phi(m_out.intPtr, fastArray, slowArray),
            m_out.phi(m_out.intPtr, fastButterfly, slowButterfly));
    }
    
    LValue typedArrayLength(Edge baseEdge, ArrayMode arrayMode, LValue base)
    {
        if (JSArrayBufferView* view = m_graph.tryGetFoldableView(baseEdge.node(), arrayMode))
            return m_out.constInt32(view->length());
        return m_out.load32(base, m_heaps.JSArrayBufferView_length);
    }
    
    LValue typedArrayLength(Edge baseEdge, ArrayMode arrayMode)
    {
        return typedArrayLength(baseEdge, arrayMode, lowCell(baseEdge));
    }
    
    LValue boolify(Edge edge)
    {
        switch (edge.useKind()) {
        case BooleanUse:
            return lowBoolean(m_node->child1());
        case Int32Use:
            return m_out.notZero32(lowInt32(m_node->child1()));
        case NumberUse:
            return m_out.doubleNotEqual(lowDouble(edge), m_out.doubleZero);
        case ObjectOrOtherUse:
            return m_out.bitNot(
                equalNullOrUndefined(
                    edge, CellCaseSpeculatesObject, SpeculateNullOrUndefined,
                    ManualOperandSpeculation));
        case StringUse: {
            LValue stringValue = lowString(m_node->child1());
            LValue length = m_out.load32(stringValue, m_heaps.JSString_length);
            return m_out.notEqual(length, m_out.int32Zero);
        }
        case UntypedUse: {
            LValue value = lowTiValue(m_node->child1());
            
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped slow case"));
            LBasicBlock fastCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped fast case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Boolify untyped continuation"));
            
            m_out.branch(isNotBoolean(value), slowCase, fastCase);
            
            LBasicBlock lastNext = m_out.appendTo(fastCase, slowCase);
            ValueFromBlock fastResult = m_out.anchor(unboxBoolean(value));
            m_out.jump(continuation);
            
            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(m_out.notNull(vmCall(
                m_out.operation(operationConvertTiValueToBoolean), m_callFrame, value)));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            return m_out.phi(m_out.boolean, fastResult, slowResult);
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return 0;
        }
    }
    
    enum StringOrObjectMode {
        AllCellsAreFalse,
        CellCaseSpeculatesObject
    };
    enum EqualNullOrUndefinedMode {
        EqualNull,
        EqualUndefined,
        EqualNullOrUndefined,
        SpeculateNullOrUndefined
    };
    LValue equalNullOrUndefined(
        Edge edge, StringOrObjectMode cellMode, EqualNullOrUndefinedMode primitiveMode,
        OperandSpeculationMode operandMode = AutomaticOperandSpeculation)
    {
        bool validWatchpoint = masqueradesAsUndefinedWatchpointIsStillValid();
        
        LValue value = lowTiValue(edge, operandMode);
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined cell case"));
        LBasicBlock primitiveCase = FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined primitive case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined continuation"));
        
        m_out.branch(isNotCell(value), primitiveCase, cellCase);
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, primitiveCase);
        
        Vector<ValueFromBlock, 3> results;
        
        switch (cellMode) {
        case AllCellsAreFalse:
            break;
        case CellCaseSpeculatesObject:
            FTL_TYPE_CHECK(
                jsValueValue(value), edge, (~SpecCell) | SpecObject,
                m_out.equal(
                    m_out.loadPtr(value, m_heaps.JSCell_structure),
                    m_out.constIntPtr(vm().stringStructure.get())));
            break;
        }
        
        if (validWatchpoint) {
            results.append(m_out.anchor(m_out.booleanFalse));
            m_out.jump(continuation);
        } else {
            LBasicBlock masqueradesCase =
                FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined masquerades case"));
                
            LValue structure = m_out.loadPtr(value, m_heaps.JSCell_structure);
            
            results.append(m_out.anchor(m_out.booleanFalse));
            
            m_out.branch(
                m_out.testNonZero8(
                    m_out.load8(structure, m_heaps.Structure_typeInfoFlags),
                    m_out.constInt8(MasqueradesAsUndefined)),
                masqueradesCase, continuation);
            
            m_out.appendTo(masqueradesCase, primitiveCase);
            
            results.append(m_out.anchor(
                m_out.equal(
                    m_out.constIntPtr(m_graph.globalObjectFor(m_node->codeOrigin)),
                    m_out.loadPtr(structure, m_heaps.Structure_globalObject))));
            m_out.jump(continuation);
        }
        
        m_out.appendTo(primitiveCase, continuation);
        
        LValue primitiveResult;
        switch (primitiveMode) {
        case EqualNull:
            primitiveResult = m_out.equal(value, m_out.constInt64(ValueNull));
            break;
        case EqualUndefined:
            primitiveResult = m_out.equal(value, m_out.constInt64(ValueUndefined));
            break;
        case EqualNullOrUndefined:
            primitiveResult = m_out.equal(
                m_out.bitAnd(value, m_out.constInt64(~TagBitUndefined)),
                m_out.constInt64(ValueNull));
            break;
        case SpeculateNullOrUndefined:
            FTL_TYPE_CHECK(
                jsValueValue(value), edge, SpecCell | SpecOther,
                m_out.notEqual(
                    m_out.bitAnd(value, m_out.constInt64(~TagBitUndefined)),
                    m_out.constInt64(ValueNull)));
            primitiveResult = m_out.booleanTrue;
            break;
        }
        results.append(m_out.anchor(primitiveResult));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        return m_out.phi(m_out.boolean, results);
    }
    
    template<typename FunctionType>
    void contiguousPutByValOutOfBounds(
        FunctionType slowPathFunction, LValue base, LValue storage, LValue index, LValue value,
        LBasicBlock continuation)
    {
        LValue isNotInBounds = m_out.aboveOrEqual(
            index, m_out.load32(storage, m_heaps.Butterfly_publicLength));
        if (!m_node->arrayMode().isInBounds()) {
            LBasicBlock notInBoundsCase =
                FTL_NEW_BLOCK(m_out, ("PutByVal not in bounds"));
            LBasicBlock performStore =
                FTL_NEW_BLOCK(m_out, ("PutByVal perform store"));
                
            m_out.branch(isNotInBounds, notInBoundsCase, performStore);
                
            LBasicBlock lastNext = m_out.appendTo(notInBoundsCase, performStore);
                
            LValue isOutOfBounds = m_out.aboveOrEqual(
                index, m_out.load32(storage, m_heaps.Butterfly_vectorLength));
                
            if (!m_node->arrayMode().isOutOfBounds())
                speculate(OutOfBounds, noValue(), 0, isOutOfBounds);
            else {
                LBasicBlock outOfBoundsCase =
                    FTL_NEW_BLOCK(m_out, ("PutByVal out of bounds"));
                LBasicBlock holeCase =
                    FTL_NEW_BLOCK(m_out, ("PutByVal hole case"));
                    
                m_out.branch(isOutOfBounds, outOfBoundsCase, holeCase);
                    
                LBasicBlock innerLastNext = m_out.appendTo(outOfBoundsCase, holeCase);
                    
                vmCall(
                    m_out.operation(slowPathFunction),
                    m_callFrame, base, index, value);
                    
                m_out.jump(continuation);
                    
                m_out.appendTo(holeCase, innerLastNext);
            }
            
            m_out.store32(
                m_out.add(index, m_out.int32One),
                storage, m_heaps.Butterfly_publicLength);
                
            m_out.jump(performStore);
            m_out.appendTo(performStore, lastNext);
        }
    }
    
    void buildSwitch(SwitchData* data, LType type, LValue switchValue)
    {
        Vector<SwitchCase> cases;
        for (unsigned i = 0; i < data->cases.size(); ++i) {
            cases.append(SwitchCase(
                constInt(type, data->cases[i].value.switchLookupValue()),
                lowBlock(data->cases[i].target)));
        }
        
        m_out.switchInstruction(switchValue, cases, lowBlock(data->fallThrough));
    }
    
    LValue doubleToInt32(LValue doubleValue, double low, double high, bool isSigned = true)
    {
        // FIXME: Optimize double-to-int conversions.
        // <rdar://problem/14938465>
        
        LBasicBlock greatEnough = FTL_NEW_BLOCK(m_out, ("doubleToInt32 greatEnough"));
        LBasicBlock withinRange = FTL_NEW_BLOCK(m_out, ("doubleToInt32 withinRange"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("doubleToInt32 slowPath"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("doubleToInt32 continuation"));
        
        Vector<ValueFromBlock, 2> results;
        
        m_out.branch(
            m_out.doubleGreaterThanOrEqual(doubleValue, m_out.constDouble(low)),
            greatEnough, slowPath);
        
        LBasicBlock lastNext = m_out.appendTo(greatEnough, withinRange);
        m_out.branch(
            m_out.doubleLessThanOrEqual(doubleValue, m_out.constDouble(high)),
            withinRange, slowPath);
        
        m_out.appendTo(withinRange, slowPath);
        LValue fastResult;
        if (isSigned)
            fastResult = m_out.fpToInt32(doubleValue);
        else
            fastResult = m_out.fpToUInt32(doubleValue);
        results.append(m_out.anchor(fastResult));
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        results.append(m_out.anchor(m_out.call(m_out.operation(toInt32), doubleValue)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int32, results);
    }
    
    LValue doubleToInt32(LValue doubleValue)
    {
        if (Output::hasSensibleDoubleToInt())
            return sensibleDoubleToInt32(doubleValue);
        
        double limit = pow(2, 31) - 1;
        return doubleToInt32(doubleValue, -limit, limit);
    }
    
    LValue sensibleDoubleToInt32(LValue doubleValue)
    {
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("sensible doubleToInt32 slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("sensible doubleToInt32 continuation"));
        
        ValueFromBlock fastResult = m_out.anchor(
            m_out.sensibleDoubleToInt(doubleValue));
        m_out.branch(
            m_out.equal(fastResult.value(), m_out.constInt32(0x80000000)),
            slowPath, continuation);
        
        LBasicBlock lastNext = m_out.appendTo(slowPath, continuation);
        ValueFromBlock slowResult = m_out.anchor(
            m_out.call(m_out.operation(toInt32), doubleValue));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int32, fastResult, slowResult);
    }
    
    void speculate(
        ExitKind kind, FormattedValue lowValue, Node* highValue, LValue failCondition)
    {
        appendOSRExit(kind, lowValue, highValue, failCondition);
    }
    
    void terminate(ExitKind kind)
    {
        speculate(kind, noValue(), 0, m_out.booleanTrue);
    }
    
    void typeCheck(
        FormattedValue lowValue, Edge highValue, SpeculatedType typesPassedThrough,
        LValue failCondition)
    {
        appendTypeCheck(lowValue, highValue, typesPassedThrough, failCondition);
    }
    
    void appendTypeCheck(
        FormattedValue lowValue, Edge highValue, SpeculatedType typesPassedThrough,
        LValue failCondition)
    {
        if (!m_interpreter.needsTypeCheck(highValue, typesPassedThrough))
            return;
        ASSERT(mayHaveTypeCheck(highValue.useKind()));
        appendOSRExit(BadType, lowValue, highValue.node(), failCondition);
        m_interpreter.filter(highValue, typesPassedThrough);
    }
    
    LValue lowInt32(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || (edge.useKind() == Int32Use || edge.useKind() == KnownInt32Use));
        
        if (edge->hasConstant()) {
            TiValue value = m_graph.valueOfJSConstant(edge.node());
            if (!value.isInt32()) {
                terminate(Uncountable);
                return m_out.int32Zero;
            }
            return m_out.constInt32(value.asInt32());
        }
        
        LoweredNodeValue value = m_int32Values.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_strictInt52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToInt32(edge, value.value());
        
        value = m_int52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToInt32(edge, int52ToStrictInt52(value.value()));
        
        value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue boxedResult = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(boxedResult), edge, SpecInt32, isNotInt32(boxedResult));
            LValue result = unboxInt32(boxedResult);
            setInt32(edge.node(), result);
            return result;
        }

        RELEASE_ASSERT(!(m_state.forNode(edge).m_type & SpecInt32));
        terminate(Uncountable);
        return m_out.int32Zero;
    }
    
    enum Int52Kind { StrictInt52, Int52 };
    LValue lowInt52(Edge edge, Int52Kind kind, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == MachineIntUse);
        
        if (edge->hasConstant()) {
            TiValue value = m_graph.valueOfJSConstant(edge.node());
            if (!value.isMachineInt()) {
                terminate(Uncountable);
                return m_out.int64Zero;
            }
            int64_t result = value.asMachineInt();
            if (kind == Int52)
                result <<= TiValue::int52ShiftAmount;
            return m_out.constInt64(result);
        }
        
        LoweredNodeValue value;
        
        switch (kind) {
        case Int52:
            value = m_int52Values.get(edge.node());
            if (isValid(value))
                return value.value();
            
            value = m_strictInt52Values.get(edge.node());
            if (isValid(value))
                return strictInt52ToInt52(value.value());
            break;
            
        case StrictInt52:
            value = m_strictInt52Values.get(edge.node());
            if (isValid(value))
                return value.value();
            
            value = m_int52Values.get(edge.node());
            if (isValid(value))
                return int52ToStrictInt52(value.value());
            break;
        }
        
        value = m_int32Values.get(edge.node());
        if (isValid(value)) {
            return setInt52WithStrictValue(
                edge.node(), m_out.signExt(value.value(), m_out.int64), kind);
        }
        
        RELEASE_ASSERT(!(m_state.forNode(edge).m_type & SpecInt52));
        
        value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue boxedResult = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(boxedResult), edge, SpecMachineInt, isNotInt32(boxedResult));
            return setInt52WithStrictValue(
                edge.node(), m_out.signExt(unboxInt32(boxedResult), m_out.int64), kind);
        }
        
        RELEASE_ASSERT(!(m_state.forNode(edge).m_type & SpecMachineInt));
        terminate(Uncountable);
        return m_out.int64Zero;
    }
    
    LValue lowInt52(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        return lowInt52(edge, Int52, mode);
    }
    
    LValue lowStrictInt52(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        return lowInt52(edge, StrictInt52, mode);
    }
    
    bool betterUseStrictInt52(Node* node)
    {
        return !isValid(m_int52Values.get(node));
    }
    bool betterUseStrictInt52(Edge edge)
    {
        return betterUseStrictInt52(edge.node());
    }
    template<typename T>
    Int52Kind bestInt52Kind(T node)
    {
        return betterUseStrictInt52(node) ? StrictInt52 : Int52;
    }
    Int52Kind opposite(Int52Kind kind)
    {
        switch (kind) {
        case Int52:
            return StrictInt52;
        case StrictInt52:
            return Int52;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    LValue lowWhicheverInt52(Edge edge, Int52Kind& kind, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        kind = bestInt52Kind(edge);
        return lowInt52(edge, kind, mode);
    }
    
    LValue lowCell(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || DFG::isCell(edge.useKind()));
        
        if (edge->op() == JSConstant) {
            TiValue value = m_graph.valueOfJSConstant(edge.node());
            if (!value.isCell()) {
                terminate(Uncountable);
                return m_out.intPtrZero;
            }
            return m_out.constIntPtr(value.asCell());
        }
        
        LoweredNodeValue value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue uncheckedValue = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(uncheckedValue), edge, SpecCell, isNotCell(uncheckedValue));
            return uncheckedValue;
        }
        
        RELEASE_ASSERT(!(m_state.forNode(edge).m_type & SpecCell));
        terminate(Uncountable);
        return m_out.intPtrZero;
    }
    
    LValue lowObject(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == ObjectUse);
        
        LValue result = lowCell(edge, mode);
        speculateObject(edge, result);
        return result;
    }
    
    LValue lowString(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == StringUse || edge.useKind() == KnownStringUse);
        
        LValue result = lowCell(edge, mode);
        speculateString(edge, result);
        return result;
    }
    
    LValue lowNonNullObject(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == ObjectUse);
        
        LValue result = lowCell(edge, mode);
        speculateNonNullObject(edge, result);
        return result;
    }
    
    LValue lowBoolean(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == BooleanUse);
        
        if (edge->hasConstant()) {
            TiValue value = m_graph.valueOfJSConstant(edge.node());
            if (!value.isBoolean()) {
                terminate(Uncountable);
                return m_out.booleanFalse;
            }
            return m_out.constBool(value.asBoolean());
        }
        
        LoweredNodeValue value = m_booleanValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue unboxedResult = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(unboxedResult), edge, SpecBoolean, isNotBoolean(unboxedResult));
            LValue result = unboxBoolean(unboxedResult);
            setBoolean(edge.node(), result);
            return result;
        }
        
        RELEASE_ASSERT(!(m_state.forNode(edge).m_type & SpecBoolean));
        terminate(Uncountable);
        return m_out.booleanFalse;
    }
    
    LValue lowDouble(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || isDouble(edge.useKind()));
        
        if (edge->hasConstant()) {
            TiValue value = m_graph.valueOfJSConstant(edge.node());
            if (!value.isNumber()) {
                terminate(Uncountable);
                return m_out.doubleZero;
            }
            return m_out.constDouble(value.asNumber());
        }
        
        LoweredNodeValue value = m_doubleValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_int32Values.get(edge.node());
        if (isValid(value)) {
            LValue result = m_out.intToDouble(value.value());
            setDouble(edge.node(), result);
            return result;
        }
        
        value = m_strictInt52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToDouble(edge, value.value());
        
        value = m_int52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToDouble(edge, int52ToStrictInt52(value.value()));
        
        value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue boxedResult = value.value();
            
            LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("Double unboxing int case"));
            LBasicBlock doubleCase = FTL_NEW_BLOCK(m_out, ("Double unboxing double case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Double unboxing continuation"));
            
            m_out.branch(isNotInt32(boxedResult), doubleCase, intCase);
            
            LBasicBlock lastNext = m_out.appendTo(intCase, doubleCase);
            
            ValueFromBlock intToDouble = m_out.anchor(
                m_out.intToDouble(unboxInt32(boxedResult)));
            m_out.jump(continuation);
            
            m_out.appendTo(doubleCase, continuation);
            
            FTL_TYPE_CHECK(
                jsValueValue(boxedResult), edge, SpecFullNumber, isCellOrMisc(boxedResult));
            
            ValueFromBlock unboxedDouble = m_out.anchor(unboxDouble(boxedResult));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            
            LValue result = m_out.phi(m_out.doubleType, intToDouble, unboxedDouble);
            
            setDouble(edge.node(), result);
            return result;
        }
        
        RELEASE_ASSERT(!(m_state.forNode(edge).m_type & SpecFullNumber));
        terminate(Uncountable);
        return m_out.doubleZero;
    }
    
    LValue lowTiValue(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == UntypedUse);
        
        if (edge->hasConstant())
            return m_out.constInt64(TiValue::encode(m_graph.valueOfJSConstant(edge.node())));
        
        LoweredNodeValue value = m_jsValueValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_int32Values.get(edge.node());
        if (isValid(value)) {
            LValue result = boxInt32(value.value());
            setTiValue(edge.node(), result);
            return result;
        }
        
        value = m_strictInt52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToTiValue(value.value());
        
        value = m_int52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToTiValue(int52ToStrictInt52(value.value()));
        
        value = m_booleanValues.get(edge.node());
        if (isValid(value)) {
            LValue result = boxBoolean(value.value());
            setTiValue(edge.node(), result);
            return result;
        }
        
        value = m_doubleValues.get(edge.node());
        if (isValid(value)) {
            LValue result = boxDouble(value.value());
            setTiValue(edge.node(), result);
            return result;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
    
    LValue lowStorage(Edge edge)
    {
        LoweredNodeValue value = m_storageValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        LValue result = lowCell(edge);
        setStorage(edge.node(), result);
        return result;
    }
    
    LValue strictInt52ToInt32(Edge edge, LValue value)
    {
        LValue result = m_out.castToInt32(value);
        FTL_TYPE_CHECK(
            noValue(), edge, SpecInt32,
            m_out.notEqual(m_out.signExt(result, m_out.int64), value));
        setInt32(edge.node(), result);
        return result;
    }
    
    LValue strictInt52ToDouble(Edge edge, LValue value)
    {
        LValue result = m_out.intToDouble(value);
        setDouble(edge.node(), result);
        return result;
    }
    
    LValue strictInt52ToTiValue(LValue value)
    {
        LBasicBlock isInt32 = FTL_NEW_BLOCK(m_out, ("strictInt52ToTiValue isInt32 case"));
        LBasicBlock isDouble = FTL_NEW_BLOCK(m_out, ("strictInt52ToTiValue isDouble case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("strictInt52ToTiValue continuation"));
        
        Vector<ValueFromBlock, 2> results;
            
        LValue int32Value = m_out.castToInt32(value);
        m_out.branch(
            m_out.equal(m_out.signExt(int32Value, m_out.int64), value),
            isInt32, isDouble);
        
        LBasicBlock lastNext = m_out.appendTo(isInt32, isDouble);
        
        results.append(m_out.anchor(boxInt32(int32Value)));
        m_out.jump(continuation);
        
        m_out.appendTo(isDouble, continuation);
        
        results.append(m_out.anchor(boxDouble(m_out.intToDouble(value))));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int64, results);
    }
    
    LValue setInt52WithStrictValue(Node* node, LValue value, Int52Kind kind)
    {
        switch (kind) {
        case StrictInt52:
            setStrictInt52(node, value);
            return value;
            
        case Int52:
            value = strictInt52ToInt52(value);
            setInt52(node, value);
            return value;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }

    LValue strictInt52ToInt52(LValue value)
    {
        return m_out.shl(value, m_out.constInt64(TiValue::int52ShiftAmount));
    }
    
    LValue int52ToStrictInt52(LValue value)
    {
        return m_out.aShr(value, m_out.constInt64(TiValue::int52ShiftAmount));
    }
    
    LValue isNotInt32(LValue jsValue)
    {
        return m_out.below(jsValue, m_tagTypeNumber);
    }
    LValue unboxInt32(LValue jsValue)
    {
        return m_out.castToInt32(jsValue);
    }
    LValue boxInt32(LValue value)
    {
        return m_out.add(m_out.zeroExt(value, m_out.int64), m_tagTypeNumber);
    }
    
    LValue isCellOrMisc(LValue jsValue)
    {
        return m_out.testIsZero64(jsValue, m_tagTypeNumber);
    }
    LValue unboxDouble(LValue jsValue)
    {
        return m_out.bitCast(m_out.add(jsValue, m_tagTypeNumber), m_out.doubleType);
    }
    LValue boxDouble(LValue doubleValue)
    {
        return m_out.sub(m_out.bitCast(doubleValue, m_out.int64), m_tagTypeNumber);
    }
    
    LValue isNotCell(LValue jsValue)
    {
        return m_out.testNonZero64(jsValue, m_tagMask);
    }
    
    LValue isCell(LValue jsValue)
    {
        return m_out.testIsZero64(jsValue, m_tagMask);
    }
    
    LValue isNotBoolean(LValue jsValue)
    {
        return m_out.testNonZero64(
            m_out.bitXor(jsValue, m_out.constInt64(ValueFalse)),
            m_out.constInt64(~1));
    }
    LValue unboxBoolean(LValue jsValue)
    {
        // We want to use a cast that guarantees that LLVM knows that even the integer
        // value is just 0 or 1. But for now we do it the dumb way.
        return m_out.notZero64(m_out.bitAnd(jsValue, m_out.constInt64(1)));
    }
    LValue boxBoolean(LValue value)
    {
        return m_out.select(
            value, m_out.constInt64(ValueTrue), m_out.constInt64(ValueFalse));
    }

    void speculate(Edge edge)
    {
        switch (edge.useKind()) {
        case UntypedUse:
            break;
        case KnownInt32Use:
        case KnownNumberUse:
            ASSERT(!m_interpreter.needsTypeCheck(edge));
            break;
        case Int32Use:
            speculateInt32(edge);
            break;
        case CellUse:
            speculateCell(edge);
            break;
        case KnownCellUse:
            ASSERT(!m_interpreter.needsTypeCheck(edge));
            break;
        case ObjectUse:
            speculateObject(edge);
            break;
        case ObjectOrOtherUse:
            speculateObjectOrOther(edge);
            break;
        case FinalObjectUse:
            speculateFinalObject(edge);
            break;
        case StringUse:
            speculateString(edge);
            break;
        case RealNumberUse:
            speculateRealNumber(edge);
            break;
        case NumberUse:
            speculateNumber(edge);
            break;
        case MachineIntUse:
            speculateMachineInt(edge);
            break;
        case BooleanUse:
            speculateBoolean(edge);
            break;
        default:
            dataLog("Unsupported speculation use kind: ", edge.useKind(), "\n");
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    void speculate(Node*, Edge edge)
    {
        speculate(edge);
    }
    
    void speculateInt32(Edge edge)
    {
        lowInt32(edge);
    }
    
    void speculateCell(Edge edge)
    {
        lowCell(edge);
    }
    
    LValue isObject(LValue cell)
    {
        return m_out.notEqual(
            m_out.loadPtr(cell, m_heaps.JSCell_structure),
            m_out.constIntPtr(vm().stringStructure.get()));
    }
    
    LValue isNotString(LValue cell)
    {
        return isObject(cell);
    }
    
    LValue isString(LValue cell)
    {
        return m_out.equal(
            m_out.loadPtr(cell, m_heaps.JSCell_structure),
            m_out.constIntPtr(vm().stringStructure.get()));
    }
    
    LValue isNotObject(LValue cell)
    {
        return isString(cell);
    }
    
    LValue isArrayType(LValue cell, ArrayMode arrayMode)
    {
        switch (arrayMode.type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            LValue indexingType = m_out.load8(
                m_out.loadPtr(cell, m_heaps.JSCell_structure),
                m_heaps.Structure_indexingType);
            
            switch (arrayMode.arrayClass()) {
            case Array::OriginalArray:
                RELEASE_ASSERT_NOT_REACHED();
                return 0;
                
            case Array::Array:
                return m_out.equal(
                    m_out.bitAnd(indexingType, m_out.constInt8(IsArray | IndexingShapeMask)),
                    m_out.constInt8(IsArray | arrayMode.shapeMask()));
                
            case Array::NonArray:
            case Array::OriginalNonArray:
                return m_out.equal(
                    m_out.bitAnd(indexingType, m_out.constInt8(IsArray | IndexingShapeMask)),
                    m_out.constInt8(arrayMode.shapeMask()));
                
            case Array::PossiblyArray:
                return m_out.equal(
                    m_out.bitAnd(indexingType, m_out.constInt8(IndexingShapeMask)),
                    m_out.constInt8(arrayMode.shapeMask()));
            }
            
            RELEASE_ASSERT_NOT_REACHED();
        }
            
        default:
            return hasClassInfo(cell, classInfoForType(arrayMode.typedArrayType()));
        }
    }
    
    LValue hasClassInfo(LValue cell, const ClassInfo* classInfo)
    {
        return m_out.equal(
            m_out.loadPtr(
                m_out.loadPtr(cell, m_heaps.JSCell_structure),
                m_heaps.Structure_classInfo),
            m_out.constIntPtr(classInfo));
    }
    
    LValue isType(LValue cell, JSType type)
    {
        return m_out.equal(
            m_out.load8(
                m_out.loadPtr(cell, m_heaps.JSCell_structure),
                m_heaps.Structure_typeInfoType),
            m_out.constInt8(type));
    }
    
    LValue isNotType(LValue cell, JSType type)
    {
        return m_out.bitNot(isType(cell, type));
    }
    
    void speculateObject(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(jsValueValue(cell), edge, SpecObject, isNotObject(cell));
    }
    
    void speculateObject(Edge edge)
    {
        speculateObject(edge, lowCell(edge));
    }
    
    void speculateObjectOrOther(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowTiValue(edge);
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("speculateObjectOrOther cell case"));
        LBasicBlock primitiveCase = FTL_NEW_BLOCK(m_out, ("speculateObjectOrOther primitive case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("speculateObjectOrOther continuation"));
        
        m_out.branch(isNotCell(value), primitiveCase, cellCase);
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, primitiveCase);
        
        FTL_TYPE_CHECK(
            jsValueValue(value), edge, (~SpecCell) | SpecObject,
            m_out.equal(
                m_out.loadPtr(value, m_heaps.JSCell_structure),
                m_out.constIntPtr(vm().stringStructure.get())));
        
        m_out.jump(continuation);
        
        m_out.appendTo(primitiveCase, continuation);
        
        FTL_TYPE_CHECK(
            jsValueValue(value), edge, SpecCell | SpecOther,
            m_out.notEqual(
                m_out.bitAnd(value, m_out.constInt64(~TagBitUndefined)),
                m_out.constInt64(ValueNull)));
        
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void speculateFinalObject(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(
            jsValueValue(cell), edge, SpecFinalObject, isNotType(cell, FinalObjectType));
    }
    
    void speculateFinalObject(Edge edge)
    {
        speculateFinalObject(edge, lowCell(edge));
    }
    
    void speculateString(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(jsValueValue(cell), edge, SpecString, isNotString(cell));
    }
    
    void speculateString(Edge edge)
    {
        speculateString(edge, lowCell(edge));
    }
    
    void speculateNonNullObject(Edge edge, LValue cell)
    {
        LValue structure = m_out.loadPtr(cell, m_heaps.JSCell_structure);
        FTL_TYPE_CHECK(
            jsValueValue(cell), edge, SpecObject, 
            m_out.equal(structure, m_out.constIntPtr(vm().stringStructure.get())));
        if (masqueradesAsUndefinedWatchpointIsStillValid())
            return;
        
        speculate(
            BadType, jsValueValue(cell), edge.node(),
            m_out.testNonZero8(
                m_out.load8(structure, m_heaps.Structure_typeInfoFlags),
                m_out.constInt8(MasqueradesAsUndefined)));
    }
    
    void speculateNumber(Edge edge)
    {
        // Do an early return here because lowDouble() can create a lot of control flow.
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        lowDouble(edge);
    }
    
    void speculateRealNumber(Edge edge)
    {
        // Do an early return here because lowDouble() can create a lot of control flow.
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowDouble(edge);
        FTL_TYPE_CHECK(
            doubleValue(value), edge, SpecFullRealNumber,
            m_out.doubleNotEqualOrUnordered(value, value));
    }
    
    void speculateMachineInt(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        Int52Kind kind;
        lowWhicheverInt52(edge, kind);
    }
    
    void speculateBoolean(Edge edge)
    {
        lowBoolean(edge);
    }
    
    bool masqueradesAsUndefinedWatchpointIsStillValid()
    {
        return m_graph.masqueradesAsUndefinedWatchpointIsStillValid(m_node->codeOrigin);
    }
    
    LValue loadMarkByte(LValue base)
    {
        LValue markedBlock = m_out.bitAnd(base, m_out.constInt64(MarkedBlock::blockMask));
        LValue baseOffset = m_out.bitAnd(base, m_out.constInt64(~MarkedBlock::blockMask));
        LValue markByteIndex = m_out.lShr(baseOffset, m_out.constInt64(MarkedBlock::atomShiftAmount + MarkedBlock::markByteShiftAmount));
        return m_out.load8(m_out.baseIndex(m_heaps.MarkedBlock_markBits, markedBlock, markByteIndex, ScaleOne, MarkedBlock::offsetOfMarks()));
    }

    void emitStoreBarrier(LValue base, LValue value, Edge& valueEdge)
    {
#if ENABLE(GGC)
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Store barrier continuation"));
        LBasicBlock isCell = FTL_NEW_BLOCK(m_out, ("Store barrier is cell block"));

        if (m_state.forNode(valueEdge.node()).couldBeType(SpecCell))
            m_out.branch(isNotCell(value), continuation, isCell);
        else
            m_out.jump(isCell);

        LBasicBlock lastNext = m_out.appendTo(isCell, continuation);
        emitStoreBarrier(base);
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
#else
        UNUSED_PARAM(base);
        UNUSED_PARAM(value);
        UNUSED_PARAM(valueEdge);
#endif
    }

    void emitStoreBarrier(LValue base)
    {
#if ENABLE(GGC)
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Store barrier continuation"));
        LBasicBlock isMarked = FTL_NEW_BLOCK(m_out, ("Store barrier is marked block"));
        LBasicBlock bufferHasSpace = FTL_NEW_BLOCK(m_out, ("Store barrier buffer is full"));
        LBasicBlock bufferIsFull = FTL_NEW_BLOCK(m_out, ("Store barrier buffer is full"));

        // Check the mark byte. 
        m_out.branch(m_out.isZero8(loadMarkByte(base)), continuation, isMarked);

        // Append to the write barrier buffer.
        LBasicBlock lastNext = m_out.appendTo(isMarked, bufferHasSpace);
        LValue currentBufferIndex = m_out.load32(m_out.absolute(&vm().heap.writeBarrierBuffer().m_currentIndex));
        LValue bufferCapacity = m_out.load32(m_out.absolute(&vm().heap.writeBarrierBuffer().m_capacity));
        m_out.branch(m_out.lessThan(currentBufferIndex, bufferCapacity), bufferHasSpace, bufferIsFull);

        // Buffer has space, store to it.
        m_out.appendTo(bufferHasSpace, bufferIsFull);
        LValue writeBarrierBufferBase = m_out.loadPtr(m_out.absolute(&vm().heap.writeBarrierBuffer().m_buffer));
        m_out.storePtr(base, m_out.baseIndex(m_heaps.WriteBarrierBuffer_bufferContents, writeBarrierBufferBase, m_out.zeroExt(currentBufferIndex, m_out.intPtr), ScalePtr));
        m_out.store32(m_out.add(currentBufferIndex, m_out.constInt32(1)), m_out.absolute(&vm().heap.writeBarrierBuffer().m_currentIndex));
        m_out.jump(continuation);

        // Buffer is out of space, flush it.
        m_out.appendTo(bufferIsFull, continuation);
        vmCall(m_out.operation(operationFlushWriteBarrierBuffer), m_callFrame, base);
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
#else
        UNUSED_PARAM(base);
#endif
    }

    enum ExceptionCheckMode { NoExceptions, CheckExceptions };
    
    LValue vmCall(LValue function, ExceptionCheckMode mode = CheckExceptions)
    {
        callPreflight();
        LValue result = m_out.call(function);
        callCheck(mode);
        return result;
    }
    LValue vmCall(LValue function, LValue arg1, ExceptionCheckMode mode = CheckExceptions)
    {
        callPreflight();
        LValue result = m_out.call(function, arg1);
        callCheck(mode);
        return result;
    }
    LValue vmCall(LValue function, LValue arg1, LValue arg2, ExceptionCheckMode mode = CheckExceptions)
    {
        callPreflight();
        LValue result = m_out.call(function, arg1, arg2);
        callCheck(mode);
        return result;
    }
    LValue vmCall(LValue function, LValue arg1, LValue arg2, LValue arg3, ExceptionCheckMode mode = CheckExceptions)
    {
        callPreflight();
        LValue result = m_out.call(function, arg1, arg2, arg3);
        callCheck(mode);
        return result;
    }
    LValue vmCall(LValue function, LValue arg1, LValue arg2, LValue arg3, LValue arg4, ExceptionCheckMode mode = CheckExceptions)
    {
        callPreflight();
        LValue result = m_out.call(function, arg1, arg2, arg3, arg4);
        callCheck(mode);
        return result;
    }
    
    void callPreflight(CodeOrigin codeOrigin)
    {
        m_out.store32(
            m_out.constInt32(
                CallFrame::Location::encodeAsCodeOriginIndex(
                    m_ftlState.jitCode->common.addCodeOrigin(codeOrigin))),
            tagFor(JSStack::ArgumentCount));
    }
    void callPreflight()
    {
        callPreflight(m_node->codeOrigin);
    }
    
    void callCheck(ExceptionCheckMode mode = CheckExceptions)
    {
        if (mode == NoExceptions)
            return;
        
        LBasicBlock didHaveException = FTL_NEW_BLOCK(m_out, ("Did have exception"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Exception check continuation"));
        
        m_out.branch(
            m_out.notZero64(m_out.load64(m_out.absolute(vm().addressOfException()))),
            didHaveException, continuation);
        
        LBasicBlock lastNext = m_out.appendTo(didHaveException, continuation);
        // FIXME: Handle exceptions. https://bugs.webkit.org/show_bug.cgi?id=113622
        m_out.crash();
        
        m_out.appendTo(continuation, lastNext);
    }
    
    LBasicBlock lowBlock(BasicBlock* block)
    {
        return m_blocks.get(block);
    }
    
    void initializeOSRExitStateForBlock()
    {
        m_availability = m_highBlock->ssa->availabilityAtHead;
    }
    
    void appendOSRExit(
        ExitKind kind, FormattedValue lowValue, Node* highValue, LValue failCondition)
    {
        if (verboseCompilationEnabled())
            dataLog("    OSR exit #", m_ftlState.jitCode->osrExit.size(), " with availability: ", m_availability, "\n");

        ASSERT(m_ftlState.jitCode->osrExit.size() == m_ftlState.finalizer->osrExit.size());
        
        m_ftlState.jitCode->osrExit.append(OSRExit(
            kind, lowValue.format(), m_graph.methodOfGettingAValueProfileFor(highValue),
            m_codeOriginForExitTarget, m_codeOriginForExitProfile,
            m_availability.numberOfArguments(), m_availability.numberOfLocals()));
        m_ftlState.finalizer->osrExit.append(OSRExitCompilationInfo());
        
        OSRExit& exit = m_ftlState.jitCode->osrExit.last();

        LBasicBlock lastNext = 0;
        LBasicBlock continuation = 0;
        
        LBasicBlock failCase = FTL_NEW_BLOCK(m_out, ("OSR exit failCase for ", m_node));
        continuation = FTL_NEW_BLOCK(m_out, ("OSR exit continuation for ", m_node));
        
        m_out.branch(failCondition, failCase, continuation);
        
        lastNext = m_out.appendTo(failCase, continuation);
        
        emitOSRExitCall(exit, lowValue);
        
        m_out.unreachable();
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void emitOSRExitCall(OSRExit& exit, FormattedValue lowValue)
    {
        ExitArgumentList arguments;
        
        CodeOrigin codeOrigin = exit.m_codeOrigin;
        
        buildExitArguments(exit, arguments, lowValue, codeOrigin);
        
        callStackmap(exit, arguments);
    }
    
    void buildExitArguments(
        OSRExit& exit, ExitArgumentList& arguments, FormattedValue lowValue,
        CodeOrigin codeOrigin)
    {
        arguments.append(m_callFrame);
        if (!!lowValue)
            arguments.append(lowValue.value());
        
        for (unsigned i = 0; i < exit.m_values.size(); ++i) {
            int operand = exit.m_values.operandForIndex(i);
            bool isLive = m_graph.isLiveInBytecode(VirtualRegister(operand), codeOrigin);
            if (!isLive) {
                exit.m_values[i] = ExitValue::dead();
                continue;
            }
            
            Availability availability = m_availability[i];
            FlushedAt flush = availability.flushedAt();
            switch (flush.format()) {
            case DeadFlush:
            case ConflictingFlush:
                if (availability.hasNode()) {
                    addExitArgumentForNode(exit, arguments, i, availability.node());
                    break;
                }
                
                if (Options::validateFTLOSRExitLiveness()) {
                    dataLog("Expected r", operand, " to be available but it wasn't.\n");
                    RELEASE_ASSERT_NOT_REACHED();
                }
                
                // This means that the DFG's DCE proved that the value is dead in bytecode
                // even though the bytecode liveness analysis thinks it's live. This is
                // acceptable since the DFG's DCE is by design more aggressive while still
                // being sound.
                exit.m_values[i] = ExitValue::dead();
                break;

            case FlushedTiValue:
            case FlushedCell:
            case FlushedBoolean:
                exit.m_values[i] = ExitValue::inJSStack(flush.virtualRegister());
                break;
                
            case FlushedInt32:
                exit.m_values[i] = ExitValue::inJSStackAsInt32(flush.virtualRegister());
                break;
                
            case FlushedInt52:
                exit.m_values[i] = ExitValue::inJSStackAsInt52(flush.virtualRegister());
                break;
                
            case FlushedDouble:
                exit.m_values[i] = ExitValue::inJSStackAsDouble(flush.virtualRegister());
                break;
                
            case FlushedArguments:
                // FIXME: implement PhantomArguments.
                // https://bugs.webkit.org/show_bug.cgi?id=113986
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        }
        
        if (verboseCompilationEnabled())
            dataLog("        Exit values: ", exit.m_values, "\n");
    }
    
    void callStackmap(OSRExit& exit, ExitArgumentList& arguments)
    {
        exit.m_stackmapID = m_stackmapIDs++;
        arguments.insert(0, m_out.constInt32(MacroAssembler::maxJumpReplacementSize()));
        arguments.insert(0, m_out.constInt32(exit.m_stackmapID));
        
        m_out.call(m_out.stackmapIntrinsic(), arguments);
    }
    
    void addExitArgumentForNode(
        OSRExit& exit, ExitArgumentList& arguments, unsigned index, Node* node)
    {
        ASSERT(node->shouldGenerate());
        ASSERT(node->hasResult());

        if (tryToSetConstantExitArgument(exit, index, node))
            return;
        
        LoweredNodeValue value = m_int32Values.get(node);
        if (isValid(value)) {
            addExitArgument(exit, arguments, index, ValueFormatInt32, value.value());
            return;
        }
        
        value = m_int52Values.get(node);
        if (isValid(value)) {
            addExitArgument(exit, arguments, index, ValueFormatInt52, value.value());
            return;
        }
        
        value = m_strictInt52Values.get(node);
        if (isValid(value)) {
            addExitArgument(exit, arguments, index, ValueFormatStrictInt52, value.value());
            return;
        }
        
        value = m_booleanValues.get(node);
        if (isValid(value)) {
            LValue valueToPass = m_out.zeroExt(value.value(), m_out.int32);
            addExitArgument(exit, arguments, index, ValueFormatBoolean, valueToPass);
            return;
        }
        
        value = m_jsValueValues.get(node);
        if (isValid(value)) {
            addExitArgument(exit, arguments, index, ValueFormatTiValue, value.value());
            return;
        }
        
        value = m_doubleValues.get(node);
        if (isValid(value)) {
            addExitArgument(exit, arguments, index, ValueFormatDouble, value.value());
            return;
        }

        dataLog("Cannot find value for node: ", node, "\n");
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    bool tryToSetConstantExitArgument(OSRExit& exit, unsigned index, Node* node)
    {
        if (!node)
            return false;
        
        switch (node->op()) {
        case JSConstant:
        case WeakTIConstant:
            exit.m_values[index] = ExitValue::constant(m_graph.valueOfJSConstant(node));
            return true;
        case PhantomArguments:
            // FIXME: implement PhantomArguments.
            // https://bugs.webkit.org/show_bug.cgi?id=113986
            RELEASE_ASSERT_NOT_REACHED();
            return true;
        default:
            return false;
        }
    }
    
    void addExitArgument(
        OSRExit& exit, ExitArgumentList& arguments, unsigned index, ValueFormat format,
        LValue value)
    {
        exit.m_values[index] = ExitValue::exitArgument(ExitArgument(format, arguments.size()));
        arguments.append(value);
    }
    
    void setInt32(Node* node, LValue value)
    {
        m_int32Values.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setInt52(Node* node, LValue value)
    {
        m_int52Values.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setStrictInt52(Node* node, LValue value)
    {
        m_strictInt52Values.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setInt52(Node* node, LValue value, Int52Kind kind)
    {
        switch (kind) {
        case Int52:
            setInt52(node, value);
            return;
            
        case StrictInt52:
            setStrictInt52(node, value);
            return;
        }
        
        RELEASE_ASSERT_NOT_REACHED();
    }
    void setTiValue(Node* node, LValue value)
    {
        m_jsValueValues.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setBoolean(Node* node, LValue value)
    {
        m_booleanValues.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setStorage(Node* node, LValue value)
    {
        m_storageValues.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setDouble(Node* node, LValue value)
    {
        m_doubleValues.set(node, LoweredNodeValue(value, m_highBlock));
    }

    void setInt32(LValue value)
    {
        setInt32(m_node, value);
    }
    void setInt52(LValue value)
    {
        setInt52(m_node, value);
    }
    void setStrictInt52(LValue value)
    {
        setStrictInt52(m_node, value);
    }
    void setInt52(LValue value, Int52Kind kind)
    {
        setInt52(m_node, value, kind);
    }
    void setTiValue(LValue value)
    {
        setTiValue(m_node, value);
    }
    void setBoolean(LValue value)
    {
        setBoolean(m_node, value);
    }
    void setStorage(LValue value)
    {
        setStorage(m_node, value);
    }
    void setDouble(LValue value)
    {
        setDouble(m_node, value);
    }
    
    bool isValid(const LoweredNodeValue& value)
    {
        if (!value)
            return false;
        if (!m_graph.m_dominators.dominates(value.block(), m_highBlock))
            return false;
        return true;
    }
    
    void addWeakReference(JSCell* target)
    {
        m_graph.m_plan.weakReferences.addLazily(target);
    }
    
    LValue weakPointer(JSCell* pointer)
    {
        addWeakReference(pointer);
        return m_out.constIntPtr(pointer);
    }
    
    TypedPointer addressFor(LValue base, int operand, ptrdiff_t offset = 0)
    {
        return m_out.address(base, m_heaps.variables[operand], offset);
    }
    TypedPointer payloadFor(LValue base, int operand)
    {
        return addressFor(base, operand, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }
    TypedPointer tagFor(LValue base, int operand)
    {
        return addressFor(base, operand, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }
    TypedPointer addressFor(int operand)
    {
        return addressFor(m_callFrame, operand);
    }
    TypedPointer addressFor(VirtualRegister operand)
    {
        return addressFor(m_callFrame, operand.offset());
    }
    TypedPointer payloadFor(int operand)
    {
        return payloadFor(m_callFrame, operand);
    }
    TypedPointer payloadFor(VirtualRegister operand)
    {
        return payloadFor(m_callFrame, operand.offset());
    }
    TypedPointer tagFor(int operand)
    {
        return tagFor(m_callFrame, operand);
    }
    TypedPointer tagFor(VirtualRegister operand)
    {
        return tagFor(m_callFrame, operand.offset());
    }
    
    VM& vm() { return m_graph.m_vm; }
    CodeBlock* codeBlock() { return m_graph.m_codeBlock; }
    
    Graph& m_graph;
    State& m_ftlState;
    AbstractHeapRepository m_heaps;
    Output m_out;
    
    LBasicBlock m_prologue;
    HashMap<BasicBlock*, LBasicBlock> m_blocks;
    
    LValue m_callFrame;
    LValue m_tagTypeNumber;
    LValue m_tagMask;
    
    HashMap<Node*, LoweredNodeValue> m_int32Values;
    HashMap<Node*, LoweredNodeValue> m_strictInt52Values;
    HashMap<Node*, LoweredNodeValue> m_int52Values;
    HashMap<Node*, LoweredNodeValue> m_jsValueValues;
    HashMap<Node*, LoweredNodeValue> m_booleanValues;
    HashMap<Node*, LoweredNodeValue> m_storageValues;
    HashMap<Node*, LoweredNodeValue> m_doubleValues;
    
    HashMap<Node*, LValue> m_phis;
    
    Operands<Availability> m_availability;
    
    InPlaceAbstractState m_state;
    AbstractInterpreter<InPlaceAbstractState> m_interpreter;
    BasicBlock* m_highBlock;
    BasicBlock* m_nextHighBlock;
    LBasicBlock m_nextLowBlock;
    
    CodeOrigin m_codeOriginForExitTarget;
    CodeOrigin m_codeOriginForExitProfile;
    unsigned m_nodeIndex;
    Node* m_node;
    
    uint32_t m_stackmapIDs;
};

void lowerDFGToLLVM(State& state)
{
    LowerDFGToLLVM lowering(state);
    lowering.lower();
}

} } // namespace TI::FTL

#endif // ENABLE(FTL_JIT)

