/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
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

#ifndef Arguments_h
#define Arguments_h

#include "JSActivation.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "Interpreter.h"
#include "ObjectConstructor.h"
#include "PrototypeFunction.h"

namespace TI {

    struct ArgumentsData : Noncopyable {
        JSActivation* activation;

        unsigned numParameters;
        ptrdiff_t firstParameterIndex;
        unsigned numArguments;

        Register* registers;
        OwnArrayPtr<Register> registerArray;

        Register* extraArguments;
        OwnArrayPtr<bool> deletedArguments;
        Register extraArgumentsFixedBuffer[4];

        TiFunction* callee;
        bool overrodeLength : 1;
        bool overrodeCallee : 1;
    };


    class Arguments : public TiObject {
    public:
        enum NoParametersType { NoParameters };

        Arguments(CallFrame*);
        Arguments(CallFrame*, NoParametersType);
        virtual ~Arguments();

        static const ClassInfo info;

        virtual void markChildren(MarkStack&);

        void fillArgList(TiExcState*, MarkedArgumentBuffer&);

        uint32_t numProvidedArguments(TiExcState* exec) const 
        {
            if (UNLIKELY(d->overrodeLength))
                return get(exec, exec->propertyNames().length).toUInt32(exec);
            return d->numArguments; 
        }
        
        void copyToRegisters(TiExcState* exec, Register* buffer, uint32_t maxSize);
        void copyRegisters();
        bool isTornOff() const { return d->registerArray; }
        void setActivation(JSActivation* activation)
        {
            d->activation = activation;
            d->registers = &activation->registerAt(0);
        }

        static PassRefPtr<Structure> createStructure(TiValue prototype) 
        { 
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags)); 
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesMarkChildren | OverridesGetPropertyNames | TiObject::StructureFlags;

    private:
        void getArgumentsData(CallFrame*, TiFunction*&, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc);
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual void put(TiExcState*, unsigned propertyName, TiValue, PutPropertySlot&);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual bool deleteProperty(TiExcState*, unsigned propertyName);

        virtual const ClassInfo* classInfo() const { return &info; }

        void init(CallFrame*);

        OwnPtr<ArgumentsData> d;
    };

    Arguments* asArguments(TiValue);

    inline Arguments* asArguments(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&Arguments::info));
        return static_cast<Arguments*>(asObject(value));
    }

    ALWAYS_INLINE void Arguments::getArgumentsData(CallFrame* callFrame, TiFunction*& function, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc)
    {
        function = callFrame->callee();

        int numParameters = function->jsExecutable()->parameterCount();
        argc = callFrame->argumentCount();

        if (argc <= numParameters)
            argv = callFrame->registers() - RegisterFile::CallFrameHeaderSize - numParameters;
        else
            argv = callFrame->registers() - RegisterFile::CallFrameHeaderSize - numParameters - argc;

        argc -= 1; // - 1 to skip "this"
        firstParameterIndex = -RegisterFile::CallFrameHeaderSize - numParameters;
    }

    inline Arguments::Arguments(CallFrame* callFrame)
        : TiObject(callFrame->lexicalGlobalObject()->argumentsStructure())
        , d(new ArgumentsData)
    {
        TiFunction* callee;
        ptrdiff_t firstParameterIndex;
        Register* argv;
        int numArguments;
        getArgumentsData(callFrame, callee, firstParameterIndex, argv, numArguments);

        d->numParameters = callee->jsExecutable()->parameterCount();
        d->firstParameterIndex = firstParameterIndex;
        d->numArguments = numArguments;

        d->activation = 0;
        d->registers = callFrame->registers();

        Register* extraArguments;
        if (d->numArguments <= d->numParameters)
            extraArguments = 0;
        else {
            unsigned numExtraArguments = d->numArguments - d->numParameters;
            if (numExtraArguments > sizeof(d->extraArgumentsFixedBuffer) / sizeof(Register))
                extraArguments = new Register[numExtraArguments];
            else
                extraArguments = d->extraArgumentsFixedBuffer;
            for (unsigned i = 0; i < numExtraArguments; ++i)
                extraArguments[i] = argv[d->numParameters + i];
        }

        d->extraArguments = extraArguments;

        d->callee = callee;
        d->overrodeLength = false;
        d->overrodeCallee = false;
    }

    inline Arguments::Arguments(CallFrame* callFrame, NoParametersType)
        : TiObject(callFrame->lexicalGlobalObject()->argumentsStructure())
        , d(new ArgumentsData)
    {
        ASSERT(!callFrame->callee()->jsExecutable()->parameterCount());

        unsigned numArguments = callFrame->argumentCount() - 1;

        d->numParameters = 0;
        d->numArguments = numArguments;
        d->activation = 0;

        Register* extraArguments;
        if (numArguments > sizeof(d->extraArgumentsFixedBuffer) / sizeof(Register))
            extraArguments = new Register[numArguments];
        else
            extraArguments = d->extraArgumentsFixedBuffer;

        Register* argv = callFrame->registers() - RegisterFile::CallFrameHeaderSize - numArguments - 1;
        for (unsigned i = 0; i < numArguments; ++i)
            extraArguments[i] = argv[i];

        d->extraArguments = extraArguments;

        d->callee = callFrame->callee();
        d->overrodeLength = false;
        d->overrodeCallee = false;
    }

    inline void Arguments::copyRegisters()
    {
        ASSERT(!isTornOff());

        if (!d->numParameters)
            return;

        int registerOffset = d->numParameters + RegisterFile::CallFrameHeaderSize;
        size_t registerArraySize = d->numParameters;

        Register* registerArray = new Register[registerArraySize];
        memcpy(registerArray, d->registers - registerOffset, registerArraySize * sizeof(Register));
        d->registerArray.set(registerArray);
        d->registers = registerArray + registerOffset;
    }

    // This JSActivation function is defined here so it can get at Arguments::setRegisters.
    inline void JSActivation::copyRegisters(Arguments* arguments)
    {
        ASSERT(!d()->registerArray);

        size_t numParametersMinusThis = d()->functionExecutable->generatedBytecode().m_numParameters - 1;
        size_t numVars = d()->functionExecutable->generatedBytecode().m_numVars;
        size_t numLocals = numVars + numParametersMinusThis;

        if (!numLocals)
            return;

        int registerOffset = numParametersMinusThis + RegisterFile::CallFrameHeaderSize;
        size_t registerArraySize = numLocals + RegisterFile::CallFrameHeaderSize;

        Register* registerArray = copyRegisterArray(d()->registers - registerOffset, registerArraySize);
        setRegisters(registerArray + registerOffset, registerArray);
        if (arguments && !arguments->isTornOff())
            static_cast<Arguments*>(arguments)->setActivation(this);
    }

    ALWAYS_INLINE Arguments* Register::arguments() const
    {
        if (jsValue() == TiValue())
            return 0;
        return asArguments(jsValue());
    }
    

} // namespace TI

#endif // Arguments_h
