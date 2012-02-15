/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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

#include "TiObjectWithGlobalObject.h"

namespace TI {

    class ExecutableBase;
    class FunctionExecutable;
    class FunctionPrototype;
    class JSActivation;
    class TiGlobalObject;
    class NativeExecutable;
    class VPtrHackExecutable;

    EncodedTiValue JSC_HOST_CALL callHostFunctionAsConstructor(TiExcState*);

    class TiFunction : public TiObjectWithGlobalObject {
        friend class JIT;
        friend class TiGlobalData;

        typedef TiObjectWithGlobalObject Base;

    public:
        TiFunction(TiExcState*, TiGlobalObject*, Structure*, int length, const Identifier&, NativeFunction);
        TiFunction(TiExcState*, TiGlobalObject*, Structure*, int length, const Identifier&, NativeExecutable*);
        TiFunction(TiExcState*, FunctionExecutable*, ScopeChainNode*);
        virtual ~TiFunction();

        const UString& name(TiExcState*);
        const UString displayName(TiExcState*);
        const UString calculatedDisplayName(TiExcState*);

        ScopeChainNode* scope()
        {
            ASSERT(!isHostFunctionNonInline());
            return m_scopeChain.get();
        }
        void setScope(TiGlobalData& globalData, ScopeChainNode* scopeChain)
        {
            ASSERT(!isHostFunctionNonInline());
            m_scopeChain.set(globalData, this, scopeChain);
        }

        ExecutableBase* executable() const { return m_executable.get(); }

        // To call either of these methods include Executable.h
        inline bool isHostFunction() const;
        FunctionExecutable* jsExecutable() const;

        static JS_EXPORTDATA const ClassInfo s_info;

        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype) 
        { 
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info); 
        }

        NativeFunction nativeFunction();

        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);

    protected:
        const static unsigned StructureFlags = OverridesGetOwnPropertySlot | ImplementsHasInstance | OverridesVisitChildren | OverridesGetPropertyNames | TiObject::StructureFlags;

    private:
        explicit TiFunction(VPtrStealingHackType);

        bool isHostFunctionNonInline() const;

        virtual void preventExtensions(TiGlobalData&);
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);

        virtual void visitChildren(SlotVisitor&);

        static TiValue argumentsGetter(TiExcState*, TiValue, const Identifier&);
        static TiValue callerGetter(TiExcState*, TiValue, const Identifier&);
        static TiValue lengthGetter(TiExcState*, TiValue, const Identifier&);

        WriteBarrier<ExecutableBase> m_executable;
        WriteBarrier<ScopeChainNode> m_scopeChain;
    };

    TiFunction* asFunction(TiValue);

    inline TiFunction* asFunction(TiValue value)
    {
        ASSERT(asObject(value)->inherits(&TiFunction::s_info));
        return static_cast<TiFunction*>(asObject(value));
    }

} // namespace TI

#endif // TiFunction_h
