/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
#include "JSActivation.h"

#include "Arguments.h"
#include "Interpreter.h"
#include "TiFunction.h"

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(JSActivation);

const ClassInfo JSActivation::s_info = { "JSActivation", &Base::s_info, 0, 0 };

JSActivation::JSActivation(CallFrame* callFrame, FunctionExecutable* functionExecutable)
    : Base(callFrame->globalData(), callFrame->globalData().activationStructure.get(), functionExecutable->symbolTable(), callFrame->registers())
    , m_numParametersMinusThis(static_cast<int>(functionExecutable->parameterCount()))
    , m_numCapturedVars(functionExecutable->capturedVariableCount())
    , m_requiresDynamicChecks(functionExecutable->usesEval())
    , m_argumentsRegister(functionExecutable->generatedBytecode().argumentsRegister())
{
    ASSERT(inherits(&s_info));

    // We have to manually ref and deref the symbol table as JSVariableObject
    // doesn't know about SharedSymbolTable
    static_cast<SharedSymbolTable*>(m_symbolTable)->ref();
}

JSActivation::~JSActivation()
{
    static_cast<SharedSymbolTable*>(m_symbolTable)->deref();
}

void JSActivation::visitChildren(SlotVisitor& visitor)
{
    ASSERT_GC_OBJECT_INHERITS(this, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(visitor);

    // No need to mark our registers if they're still in the RegisterFile.
    WriteBarrier<Unknown>* registerArray = m_registerArray.get();
    if (!registerArray)
        return;

    visitor.appendValues(registerArray, m_numParametersMinusThis);

    // Skip the call frame, which sits between the parameters and vars.
    visitor.appendValues(registerArray + m_numParametersMinusThis + RegisterFile::CallFrameHeaderSize, m_numCapturedVars, MayContainNullValues);
}

inline bool JSActivation::symbolTableGet(const Identifier& propertyName, PropertySlot& slot)
{
    SymbolTableEntry entry = symbolTable().inlineGet(propertyName.impl());
    if (entry.isNull())
        return false;
    if (entry.getIndex() >= m_numCapturedVars)
        return false;

    slot.setValue(registerAt(entry.getIndex()).get());
    return true;
}

inline bool JSActivation::symbolTablePut(TiGlobalData& globalData, const Identifier& propertyName, TiValue value)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    
    SymbolTableEntry entry = symbolTable().inlineGet(propertyName.impl());
    if (entry.isNull())
        return false;
    if (entry.isReadOnly())
        return true;
    if (entry.getIndex() >= m_numCapturedVars)
        return false;

    registerAt(entry.getIndex()).set(globalData, this, value);
    return true;
}

void JSActivation::getOwnPropertyNames(TiExcState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    SymbolTable::const_iterator end = symbolTable().end();
    for (SymbolTable::const_iterator it = symbolTable().begin(); it != end; ++it) {
        if (it->second.getAttributes() & DontEnum && mode != IncludeDontEnumProperties)
            continue;
        if (it->second.getIndex() >= m_numCapturedVars)
            continue;
        propertyNames.add(Identifier(exec, it->first.get()));
    }
    // Skip the JSVariableObject implementation of getOwnPropertyNames
    TiObject::getOwnPropertyNames(exec, propertyNames, mode);
}

inline bool JSActivation::symbolTablePutWithAttributes(TiGlobalData& globalData, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    
    SymbolTable::iterator iter = symbolTable().find(propertyName.impl());
    if (iter == symbolTable().end())
        return false;
    SymbolTableEntry& entry = iter->second;
    ASSERT(!entry.isNull());
    if (entry.getIndex() >= m_numCapturedVars)
        return false;

    entry.setAttributes(attributes);
    registerAt(entry.getIndex()).set(globalData, this, value);
    return true;
}

bool JSActivation::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (propertyName == exec->propertyNames().arguments) {
        slot.setCustom(this, getArgumentsGetter());
        return true;
    }

    if (symbolTableGet(propertyName, slot))
        return true;

    if (WriteBarrierBase<Unknown>* location = getDirectLocation(exec->globalData(), propertyName)) {
        slot.setValue(location->get());
        return true;
    }

    // We don't call through to TiObject because there's no way to give an 
    // activation object getter properties or a prototype.
    ASSERT(!hasGetterSetterProperties());
    ASSERT(prototype().isNull());
    return false;
}

void JSActivation::put(TiExcState* exec, const Identifier& propertyName, TiValue value, PutPropertySlot& slot)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (symbolTablePut(exec->globalData(), propertyName, value))
        return;

    // We don't call through to TiObject because __proto__ and getter/setter 
    // properties are non-standard extensions that other implementations do not
    // expose in the activation object.
    ASSERT(!hasGetterSetterProperties());
    putDirect(exec->globalData(), propertyName, value, 0, true, slot);
}

// FIXME: Make this function honor ReadOnly (const) and DontEnum
void JSActivation::putWithAttributes(TiExcState* exec, const Identifier& propertyName, TiValue value, unsigned attributes)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (symbolTablePutWithAttributes(exec->globalData(), propertyName, value, attributes))
        return;

    // We don't call through to TiObject because __proto__ and getter/setter 
    // properties are non-standard extensions that other implementations do not
    // expose in the activation object.
    ASSERT(!hasGetterSetterProperties());
    PutPropertySlot slot;
    TiObject::putWithAttributes(exec, propertyName, value, attributes, true, slot);
}

bool JSActivation::deleteProperty(TiExcState* exec, const Identifier& propertyName)
{
    if (propertyName == exec->propertyNames().arguments)
        return false;

    return Base::deleteProperty(exec, propertyName);
}

TiObject* JSActivation::toThisObject(TiExcState* exec) const
{
    return exec->globalThisValue();
}

TiValue JSActivation::toStrictThisObject(TiExcState*) const
{
    return jsNull();
}
    
bool JSActivation::isDynamicScope(bool& requiresDynamicChecks) const
{
    requiresDynamicChecks = m_requiresDynamicChecks;
    return false;
}

TiValue JSActivation::argumentsGetter(TiExcState*, TiValue slotBase, const Identifier&)
{
    JSActivation* activation = asActivation(slotBase);
    CallFrame* callFrame = CallFrame::create(reinterpret_cast<Register*>(activation->m_registers));
    int argumentsRegister = activation->m_argumentsRegister;
    if (TiValue arguments = callFrame->uncheckedR(argumentsRegister).jsValue())
        return arguments;
    int realArgumentsRegister = unmodifiedArgumentsRegister(argumentsRegister);

    TiValue arguments = TiValue(new (callFrame) Arguments(callFrame));
    callFrame->uncheckedR(argumentsRegister) = arguments;
    callFrame->uncheckedR(realArgumentsRegister) = arguments;
    
    ASSERT(callFrame->uncheckedR(realArgumentsRegister).jsValue().inherits(&Arguments::s_info));
    return callFrame->uncheckedR(realArgumentsRegister).jsValue();
}

// These two functions serve the purpose of isolating the common case from a
// PIC branch.

PropertySlot::GetValueFunc JSActivation::getArgumentsGetter()
{
    return argumentsGetter;
}

} // namespace TI
