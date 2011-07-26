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

#ifndef TiFunction_h
#define TiFunction_h

#include "InternalFunction.h"

namespace TI {

    class ExecutableBase;
    class FunctionExecutable;
    class FunctionPrototype;
    class JSActivation;
    class TiGlobalObject;
    class NativeExecutable;

    class TiFunction : public InternalFunction {
        friend class JIT;
        friend class TiGlobalData;

        typedef InternalFunction Base;

    public:
        TiFunction(TiExcState*, NonNullPassRefPtr<Structure>, int length, const Identifier&, NativeFunction);
        TiFunction(TiExcState*, NonNullPassRefPtr<Structure>, int length, const Identifier&, NativeExecutable*, NativeFunction);
        TiFunction(TiExcState*, NonNullPassRefPtr<FunctionExecutable>, ScopeChainNode*);
        virtual ~TiFunction();

        TiObject* construct(TiExcState*, const ArgList&);
        TiValue call(TiExcState*, TiValue thisValue, const ArgList&);

        void setScope(const ScopeChain& scopeChain) { setScopeChain(scopeChain); }
        ScopeChain& scope() { return scopeChain(); }

        ExecutableBase* executable() const { return m_executable.get(); }

        // To call either of these methods include Executable.h
        inline bool isHostFunction() const;
        FunctionExecutable* jsExecutable() const;

        static JS_EXPORTDATA const ClassInfo info;

        static PassRefPtr<Structure> createStructure(TiValue prototype) 
        { 
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount); 
        }

        NativeFunction nativeFunction()
        {
            return *WTI::bitwise_cast<NativeFunction*>(m_data);
        }

        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);

    protected:
        const static unsigned StructureFlags = OverridesGetOwnPropertySlot | ImplementsHasInstance | OverridesMarkChildren | OverridesGetPropertyNames | InternalFunction::StructureFlags;

    private:
        TiFunction(NonNullPassRefPtr<Structure>);

        bool isHostFunctionNonInline() const;

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);

        virtual void markChildren(MarkStack&);

        virtual const ClassInfo* classInfo() const { return &info; }

        static TiValue argumentsGetter(TiExcState*, TiValue, const Identifier&);
        static TiValue callerGetter(TiExcState*, TiValue, const Identifier&);
        static TiValue lengthGetter(TiExcState*, TiValue, const Identifier&);

        RefPtr<ExecutableBase> m_executable;
        ScopeChain& scopeChain()
        {
            ASSERT(!isHostFunctionNonInline());
            return *WTI::bitwise_cast<ScopeChain*>(m_data);
        }
        void clearScopeChain()
        {
            ASSERT(!isHostFunctionNonInline());
            new (m_data) ScopeChain(NoScopeChain());
        }
        void setScopeChain(ScopeChainNode* sc)
        {
            ASSERT(!isHostFunctionNonInline());
            new (m_data) ScopeChain(sc);
        }
        void setScopeChain(const ScopeChain& sc)
        {
            ASSERT(!isHostFunctionNonInline());
            *WTI::bitwise_cast<ScopeChain*>(m_data) = sc;
        }
        void setNativeFunction(NativeFunction func)
        {
            *WTI::bitwise_cast<NativeFunction*>(m_data) = func;
        }
        unsigned char m_data[sizeof(void*)];
    };

    TiFunction* asFunction(TiValue);

    inline TiFunction* asFunction(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&TiFunction::info));
        return static_cast<TiFunction*>(asObject(value));
    }

} // namespace TI

#endif // TiFunction_h
