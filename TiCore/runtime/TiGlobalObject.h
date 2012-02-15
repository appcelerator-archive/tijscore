/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef TiGlobalObject_h
#define TiGlobalObject_h

#include "TiArray.h"
#include "TiGlobalData.h"
#include "JSVariableObject.h"
#include "JSWeakObjectMapRefInternal.h"
#include "NumberPrototype.h"
#include "StringPrototype.h"
#include "StructureChain.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/RandomNumber.h>

namespace TI {

    class ArrayPrototype;
    class BooleanPrototype;
    class DatePrototype;
    class Debugger;
    class ErrorConstructor;
    class FunctionPrototype;
    class GlobalCodeBlock;
    class NativeErrorConstructor;
    class ProgramCodeBlock;
    class RegExpConstructor;
    class RegExpPrototype;
    class RegisterFile;

    struct ActivationStackNode;
    struct HashTable;

    typedef Vector<TiExcState*, 16> TiExcStateStack;
    
    class TiGlobalObject : public JSVariableObject {
    protected:
        typedef HashSet<RefPtr<OpaqueJSWeakObjectMap> > WeakMapSet;

        RefPtr<TiGlobalData> m_globalData;

        size_t m_registerArraySize;
        Register m_globalCallFrame[RegisterFile::CallFrameHeaderSize];

        WriteBarrier<ScopeChainNode> m_globalScopeChain;
        WriteBarrier<TiObject> m_methodCallDummy;

        WriteBarrier<RegExpConstructor> m_regExpConstructor;
        WriteBarrier<ErrorConstructor> m_errorConstructor;
        WriteBarrier<NativeErrorConstructor> m_evalErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_rangeErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_referenceErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_syntaxErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_typeErrorConstructor;
        WriteBarrier<NativeErrorConstructor> m_URIErrorConstructor;

        WriteBarrier<TiFunction> m_evalFunction;
        WriteBarrier<TiFunction> m_callFunction;
        WriteBarrier<TiFunction> m_applyFunction;

        WriteBarrier<ObjectPrototype> m_objectPrototype;
        WriteBarrier<FunctionPrototype> m_functionPrototype;
        WriteBarrier<ArrayPrototype> m_arrayPrototype;
        WriteBarrier<BooleanPrototype> m_booleanPrototype;
        WriteBarrier<StringPrototype> m_stringPrototype;
        WriteBarrier<NumberPrototype> m_numberPrototype;
        WriteBarrier<DatePrototype> m_datePrototype;
        WriteBarrier<RegExpPrototype> m_regExpPrototype;

        WriteBarrier<Structure> m_argumentsStructure;
        WriteBarrier<Structure> m_arrayStructure;
        WriteBarrier<Structure> m_booleanObjectStructure;
        WriteBarrier<Structure> m_callbackConstructorStructure;
        WriteBarrier<Structure> m_callbackFunctionStructure;
        WriteBarrier<Structure> m_callbackObjectStructure;
        WriteBarrier<Structure> m_dateStructure;
        WriteBarrier<Structure> m_emptyObjectStructure;
        WriteBarrier<Structure> m_nullPrototypeObjectStructure;
        WriteBarrier<Structure> m_errorStructure;
        WriteBarrier<Structure> m_functionStructure;
        WriteBarrier<Structure> m_numberObjectStructure;
        WriteBarrier<Structure> m_regExpMatchesArrayStructure;
        WriteBarrier<Structure> m_regExpStructure;
        WriteBarrier<Structure> m_stringObjectStructure;
        WriteBarrier<Structure> m_internalFunctionStructure;

        unsigned m_profileGroup;
        Debugger* m_debugger;

        WeakMapSet m_weakMaps;
        Weak<TiGlobalObject> m_weakMapsFinalizer;
        class WeakMapsFinalizer : public WeakHandleOwner {
        public:
            virtual void finalize(Handle<Unknown>, void* context);
        };
        static WeakMapsFinalizer* weakMapsFinalizer();

        WeakRandom m_weakRandom;

        SymbolTable m_symbolTable;

        bool m_isEvalEnabled;

    public:
        void* operator new(size_t, TiGlobalData*);

        explicit TiGlobalObject(TiGlobalData& globalData, Structure* structure)
            : JSVariableObject(globalData, structure, &m_symbolTable, 0)
            , m_registerArraySize(0)
            , m_globalScopeChain()
            , m_weakRandom(static_cast<unsigned>(randomNumber() * (std::numeric_limits<unsigned>::max() + 1.0)))
            , m_isEvalEnabled(true)
        {
            COMPILE_ASSERT(TiGlobalObject::AnonymousSlotCount == 1, TiGlobalObject_has_only_a_single_slot);
            putThisToAnonymousValue(0);
            init(this);
        }

        static JS_EXPORTDATA const ClassInfo s_info;

    protected:
        TiGlobalObject(TiGlobalData& globalData, Structure* structure, TiObject* thisValue)
            : JSVariableObject(globalData, structure, &m_symbolTable, 0)
            , m_registerArraySize(0)
            , m_globalScopeChain()
            , m_weakRandom(static_cast<unsigned>(randomNumber() * (std::numeric_limits<unsigned>::max() + 1.0)))
            , m_isEvalEnabled(true)
        {
            COMPILE_ASSERT(TiGlobalObject::AnonymousSlotCount == 1, TiGlobalObject_has_only_a_single_slot);
            putThisToAnonymousValue(0);
            init(thisValue);
        }

    public:
        virtual ~TiGlobalObject();

        virtual void visitChildren(SlotVisitor&);

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual bool hasOwnPropertyForWrite(TiExcState*, const Identifier&);
        virtual void put(TiExcState*, const Identifier&, TiValue, PutPropertySlot&);
        virtual void putWithAttributes(TiExcState*, const Identifier& propertyName, TiValue value, unsigned attributes);

        virtual void defineGetter(TiExcState*, const Identifier& propertyName, TiObject* getterFunc, unsigned attributes);
        virtual void defineSetter(TiExcState*, const Identifier& propertyName, TiObject* setterFunc, unsigned attributes);

        // We use this in the code generator as we perform symbol table
        // lookups prior to initializing the properties
        bool symbolTableHasProperty(const Identifier& propertyName);

        // The following accessors return pristine values, even if a script 
        // replaces the global object's associated property.

        RegExpConstructor* regExpConstructor() const { return m_regExpConstructor.get(); }

        ErrorConstructor* errorConstructor() const { return m_errorConstructor.get(); }
        NativeErrorConstructor* evalErrorConstructor() const { return m_evalErrorConstructor.get(); }
        NativeErrorConstructor* rangeErrorConstructor() const { return m_rangeErrorConstructor.get(); }
        NativeErrorConstructor* referenceErrorConstructor() const { return m_referenceErrorConstructor.get(); }
        NativeErrorConstructor* syntaxErrorConstructor() const { return m_syntaxErrorConstructor.get(); }
        NativeErrorConstructor* typeErrorConstructor() const { return m_typeErrorConstructor.get(); }
        NativeErrorConstructor* URIErrorConstructor() const { return m_URIErrorConstructor.get(); }

        TiFunction* evalFunction() const { return m_evalFunction.get(); }
        TiFunction* callFunction() const { return m_callFunction.get(); }
        TiFunction* applyFunction() const { return m_applyFunction.get(); }

        ObjectPrototype* objectPrototype() const { return m_objectPrototype.get(); }
        FunctionPrototype* functionPrototype() const { return m_functionPrototype.get(); }
        ArrayPrototype* arrayPrototype() const { return m_arrayPrototype.get(); }
        BooleanPrototype* booleanPrototype() const { return m_booleanPrototype.get(); }
        StringPrototype* stringPrototype() const { return m_stringPrototype.get(); }
        NumberPrototype* numberPrototype() const { return m_numberPrototype.get(); }
        DatePrototype* datePrototype() const { return m_datePrototype.get(); }
        RegExpPrototype* regExpPrototype() const { return m_regExpPrototype.get(); }

        TiObject* methodCallDummy() const { return m_methodCallDummy.get(); }

        Structure* argumentsStructure() const { return m_argumentsStructure.get(); }
        Structure* arrayStructure() const { return m_arrayStructure.get(); }
        Structure* booleanObjectStructure() const { return m_booleanObjectStructure.get(); }
        Structure* callbackConstructorStructure() const { return m_callbackConstructorStructure.get(); }
        Structure* callbackFunctionStructure() const { return m_callbackFunctionStructure.get(); }
        Structure* callbackObjectStructure() const { return m_callbackObjectStructure.get(); }
        Structure* dateStructure() const { return m_dateStructure.get(); }
        Structure* emptyObjectStructure() const { return m_emptyObjectStructure.get(); }
        Structure* nullPrototypeObjectStructure() const { return m_nullPrototypeObjectStructure.get(); }
        Structure* errorStructure() const { return m_errorStructure.get(); }
        Structure* functionStructure() const { return m_functionStructure.get(); }
        Structure* numberObjectStructure() const { return m_numberObjectStructure.get(); }
        Structure* internalFunctionStructure() const { return m_internalFunctionStructure.get(); }
        Structure* regExpMatchesArrayStructure() const { return m_regExpMatchesArrayStructure.get(); }
        Structure* regExpStructure() const { return m_regExpStructure.get(); }
        Structure* stringObjectStructure() const { return m_stringObjectStructure.get(); }

        void setProfileGroup(unsigned value) { m_profileGroup = value; }
        unsigned profileGroup() const { return m_profileGroup; }

        Debugger* debugger() const { return m_debugger; }
        void setDebugger(Debugger* debugger) { m_debugger = debugger; }

        virtual bool supportsProfiling() const { return false; }
        virtual bool supportsRichSourceInfo() const { return true; }

        ScopeChainNode* globalScopeChain() { return m_globalScopeChain.get(); }

        virtual bool isGlobalObject() const { return true; }

        virtual TiExcState* globalExec();

        virtual bool shouldInterruptScriptBeforeTimeout() const { return false; }
        virtual bool shouldInterruptScript() const { return true; }

        virtual bool allowsAccessFrom(const TiGlobalObject*) const { return true; }

        virtual bool isDynamicScope(bool& requiresDynamicChecks) const;

        void disableEval();
        bool isEvalEnabled() { return m_isEvalEnabled; }

        void copyGlobalsFrom(RegisterFile&);
        void copyGlobalsTo(RegisterFile&);
        void resizeRegisters(int oldSize, int newSize);

        void resetPrototype(TiGlobalData&, TiValue prototype);

        TiGlobalData& globalData() const { return *m_globalData.get(); }

        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

        void registerWeakMap(OpaqueJSWeakObjectMap* map)
        {
            if (!m_weakMapsFinalizer)
                m_weakMapsFinalizer.set(globalData(), this, weakMapsFinalizer());
            m_weakMaps.add(map);
        }

        void deregisterWeakMap(OpaqueJSWeakObjectMap* map)
        {
            m_weakMaps.remove(map);
        }

        double weakRandomNumber() { return m_weakRandom.get(); }
    protected:

        static const unsigned AnonymousSlotCount = JSVariableObject::AnonymousSlotCount + 1;
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | JSVariableObject::StructureFlags;

        struct GlobalPropertyInfo {
            GlobalPropertyInfo(const Identifier& i, TiValue v, unsigned a)
                : identifier(i)
                , value(v)
                , attributes(a)
            {
            }

            const Identifier identifier;
            TiValue value;
            unsigned attributes;
        };
        void addStaticGlobals(GlobalPropertyInfo*, int count);

    private:
        // FIXME: Fold reset into init.
        void init(TiObject* thisValue);
        void reset(TiValue prototype);

        void setRegisters(WriteBarrier<Unknown>* registers, PassOwnArrayPtr<WriteBarrier<Unknown> > registerArray, size_t count);

        void* operator new(size_t); // can only be allocated with TiGlobalData
    };

    TiGlobalObject* asGlobalObject(TiValue);

    inline TiGlobalObject* asGlobalObject(TiValue value)
    {
        ASSERT(asObject(value)->isGlobalObject());
        return static_cast<TiGlobalObject*>(asObject(value));
    }

    inline void TiGlobalObject::setRegisters(WriteBarrier<Unknown>* registers, PassOwnArrayPtr<WriteBarrier<Unknown> > registerArray, size_t count)
    {
        JSVariableObject::setRegisters(registers, registerArray);
        m_registerArraySize = count;
    }

    inline bool TiGlobalObject::hasOwnPropertyForWrite(TiExcState* exec, const Identifier& propertyName)
    {
        PropertySlot slot;
        if (JSVariableObject::getOwnPropertySlot(exec, propertyName, slot))
            return true;
        bool slotIsWriteable;
        return symbolTableGet(propertyName, slot, slotIsWriteable);
    }

    inline bool TiGlobalObject::symbolTableHasProperty(const Identifier& propertyName)
    {
        SymbolTableEntry entry = symbolTable().inlineGet(propertyName.impl());
        return !entry.isNull();
    }

    inline TiValue Structure::prototypeForLookup(TiExcState* exec) const
    {
        if (typeInfo().type() == ObjectType)
            return m_prototype.get();

        ASSERT(typeInfo().type() == StringType);
        return exec->lexicalGlobalObject()->stringPrototype();
    }

    inline StructureChain* Structure::prototypeChain(TiExcState* exec) const
    {
        // We cache our prototype chain so our clients can share it.
        if (!isValid(exec, m_cachedPrototypeChain.get())) {
            TiValue prototype = prototypeForLookup(exec);
            m_cachedPrototypeChain.set(exec->globalData(), this, StructureChain::create(exec->globalData(), prototype.isNull() ? 0 : asObject(prototype)->structure()));
        }
        return m_cachedPrototypeChain.get();
    }

    inline bool Structure::isValid(TiExcState* exec, StructureChain* cachedPrototypeChain) const
    {
        if (!cachedPrototypeChain)
            return false;

        TiValue prototype = prototypeForLookup(exec);
        WriteBarrier<Structure>* cachedStructure = cachedPrototypeChain->head();
        while(*cachedStructure && !prototype.isNull()) {
            if (asObject(prototype)->structure() != cachedStructure->get())
                return false;
            ++cachedStructure;
            prototype = asObject(prototype)->prototype();
        }
        return prototype.isNull() && !*cachedStructure;
    }

    inline TiGlobalObject* TiExcState::dynamicGlobalObject()
    {
        if (this == lexicalGlobalObject()->globalExec())
            return lexicalGlobalObject();

        // For any TiExcState that's not a globalExec, the 
        // dynamic global object must be set since code is running
        ASSERT(globalData().dynamicGlobalObject);
        return globalData().dynamicGlobalObject;
    }

    inline TiObject* constructEmptyObject(TiExcState* exec, TiGlobalObject* globalObject)
    {
        return constructEmptyObject(exec, globalObject->emptyObjectStructure());
    }

    inline TiObject* constructEmptyObject(TiExcState* exec)
    {
        return constructEmptyObject(exec, exec->lexicalGlobalObject());
    }

    inline TiArray* constructEmptyArray(TiExcState* exec, TiGlobalObject* globalObject)
    {
        return new (exec) TiArray(exec->globalData(), globalObject->arrayStructure());
    }
    
    inline TiArray* constructEmptyArray(TiExcState* exec)
    {
        return constructEmptyArray(exec, exec->lexicalGlobalObject());
    }

    inline TiArray* constructEmptyArray(TiExcState* exec, TiGlobalObject* globalObject, unsigned initialLength)
    {
        return new (exec) TiArray(exec->globalData(), globalObject->arrayStructure(), initialLength, CreateInitialized);
    }

    inline TiArray* constructEmptyArray(TiExcState* exec, unsigned initialLength)
    {
        return constructEmptyArray(exec, exec->lexicalGlobalObject(), initialLength);
    }

    inline TiArray* constructArray(TiExcState* exec, TiGlobalObject* globalObject, TiValue singleItemValue)
    {
        MarkedArgumentBuffer values;
        values.append(singleItemValue);
        return new (exec) TiArray(exec->globalData(), globalObject->arrayStructure(), values);
    }

    inline TiArray* constructArray(TiExcState* exec, TiValue singleItemValue)
    {
        return constructArray(exec, exec->lexicalGlobalObject(), singleItemValue);
    }

    inline TiArray* constructArray(TiExcState* exec, TiGlobalObject* globalObject, const ArgList& values)
    {
        return new (exec) TiArray(exec->globalData(), globalObject->arrayStructure(), values);
    }

    inline TiArray* constructArray(TiExcState* exec, const ArgList& values)
    {
        return constructArray(exec, exec->lexicalGlobalObject(), values);
    }

    class DynamicGlobalObjectScope {
        WTF_MAKE_NONCOPYABLE(DynamicGlobalObjectScope);
    public:
        DynamicGlobalObjectScope(TiGlobalData&, TiGlobalObject*);

        ~DynamicGlobalObjectScope()
        {
            m_dynamicGlobalObjectSlot = m_savedDynamicGlobalObject;
        }

    private:
        TiGlobalObject*& m_dynamicGlobalObjectSlot;
        TiGlobalObject* m_savedDynamicGlobalObject;
    };

} // namespace TI

#endif // TiGlobalObject_h
