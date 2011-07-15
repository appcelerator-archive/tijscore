/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "NativeFunctionWrapper.h"
#include "NumberPrototype.h"
#include "StringPrototype.h"
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
    class GlobalEvalFunction;
    class NativeErrorConstructor;
    class ProgramCodeBlock;
    class PrototypeFunction;
    class RegExpConstructor;
    class RegExpPrototype;
    class RegisterFile;

    struct ActivationStackNode;
    struct HashTable;

    typedef Vector<TiExcState*, 16> TiExcStateStack;
    
    class TiGlobalObject : public JSVariableObject {
    protected:
        using JSVariableObject::JSVariableObjectData;
        typedef HashSet<RefPtr<OpaqueJSWeakObjectMap> > WeakMapSet;

        struct TiGlobalObjectData : public JSVariableObjectData {
            // We use an explicit destructor function pointer instead of a
            // virtual destructor because we want to avoid adding a vtable
            // pointer to this struct. Adding a vtable pointer would force the
            // compiler to emit costly pointer fixup code when casting from
            // JSVariableObjectData* to TiGlobalObjectData*.
            typedef void (*Destructor)(void*);

            TiGlobalObjectData(Destructor destructor)
                : JSVariableObjectData(&symbolTable, 0)
                , destructor(destructor)
                , registerArraySize(0)
                , globalScopeChain(NoScopeChain())
                , regExpConstructor(0)
                , errorConstructor(0)
                , evalErrorConstructor(0)
                , rangeErrorConstructor(0)
                , referenceErrorConstructor(0)
                , syntaxErrorConstructor(0)
                , typeErrorConstructor(0)
                , URIErrorConstructor(0)
                , evalFunction(0)
                , callFunction(0)
                , applyFunction(0)
                , objectPrototype(0)
                , functionPrototype(0)
                , arrayPrototype(0)
                , booleanPrototype(0)
                , stringPrototype(0)
                , numberPrototype(0)
                , datePrototype(0)
                , regExpPrototype(0)
                , methodCallDummy(0)
                , weakRandom(static_cast<unsigned>(randomNumber() * (std::numeric_limits<unsigned>::max() + 1.0)))
            {
            }
            
            Destructor destructor;
            
            size_t registerArraySize;

            TiGlobalObject* next;
            TiGlobalObject* prev;

            Debugger* debugger;
            
            ScopeChain globalScopeChain;
            Register globalCallFrame[RegisterFile::CallFrameHeaderSize];

            int recursion;

            RegExpConstructor* regExpConstructor;
            ErrorConstructor* errorConstructor;
            NativeErrorConstructor* evalErrorConstructor;
            NativeErrorConstructor* rangeErrorConstructor;
            NativeErrorConstructor* referenceErrorConstructor;
            NativeErrorConstructor* syntaxErrorConstructor;
            NativeErrorConstructor* typeErrorConstructor;
            NativeErrorConstructor* URIErrorConstructor;

            GlobalEvalFunction* evalFunction;
            NativeFunctionWrapper* callFunction;
            NativeFunctionWrapper* applyFunction;

            ObjectPrototype* objectPrototype;
            FunctionPrototype* functionPrototype;
            ArrayPrototype* arrayPrototype;
            BooleanPrototype* booleanPrototype;
            StringPrototype* stringPrototype;
            NumberPrototype* numberPrototype;
            DatePrototype* datePrototype;
            RegExpPrototype* regExpPrototype;

            TiObject* methodCallDummy;

            RefPtr<Structure> argumentsStructure;
            RefPtr<Structure> arrayStructure;
            RefPtr<Structure> booleanObjectStructure;
            RefPtr<Structure> callbackConstructorStructure;
            RefPtr<Structure> callbackFunctionStructure;
            RefPtr<Structure> callbackObjectStructure;
            RefPtr<Structure> dateStructure;
            RefPtr<Structure> emptyObjectStructure;
            RefPtr<Structure> errorStructure;
            RefPtr<Structure> functionStructure;
            RefPtr<Structure> numberObjectStructure;
            RefPtr<Structure> prototypeFunctionStructure;
            RefPtr<Structure> regExpMatchesArrayStructure;
            RefPtr<Structure> regExpStructure;
            RefPtr<Structure> stringObjectStructure;

            SymbolTable symbolTable;
            unsigned profileGroup;

            RefPtr<TiGlobalData> globalData;

            HashSet<GlobalCodeBlock*> codeBlocks;
            WeakMapSet weakMaps;
            WeakRandom weakRandom;
        };

    public:
        void* operator new(size_t, TiGlobalData*);

        explicit TiGlobalObject()
            : JSVariableObject(TiGlobalObject::createStructure(jsNull()), new TiGlobalObjectData(destroyTiGlobalObjectData))
        {
            init(this);
        }

    protected:
        TiGlobalObject(NonNullPassRefPtr<Structure> structure, TiGlobalObjectData* data, TiObject* thisValue)
            : JSVariableObject(structure, data)
        {
            init(thisValue);
        }

    public:
        virtual ~TiGlobalObject();

        virtual void markChildren(MarkStack&);

        virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual bool hasOwnPropertyForWrite(TiExcState*, const Identifier&);
        virtual void put(TiExcState*, const Identifier&, TiValue, PutPropertySlot&);
        virtual void putWithAttributes(TiExcState*, const Identifier& propertyName, TiValue value, unsigned attributes);

        virtual void defineGetter(TiExcState*, const Identifier& propertyName, TiObject* getterFunc, unsigned attributes);
        virtual void defineSetter(TiExcState*, const Identifier& propertyName, TiObject* setterFunc, unsigned attributes);

        // Linked list of all global objects that use the same TiGlobalData.
        TiGlobalObject*& head() { return d()->globalData->head; }
        TiGlobalObject* next() { return d()->next; }

        // The following accessors return pristine values, even if a script 
        // replaces the global object's associated property.

        RegExpConstructor* regExpConstructor() const { return d()->regExpConstructor; }

        ErrorConstructor* errorConstructor() const { return d()->errorConstructor; }
        NativeErrorConstructor* evalErrorConstructor() const { return d()->evalErrorConstructor; }
        NativeErrorConstructor* rangeErrorConstructor() const { return d()->rangeErrorConstructor; }
        NativeErrorConstructor* referenceErrorConstructor() const { return d()->referenceErrorConstructor; }
        NativeErrorConstructor* syntaxErrorConstructor() const { return d()->syntaxErrorConstructor; }
        NativeErrorConstructor* typeErrorConstructor() const { return d()->typeErrorConstructor; }
        NativeErrorConstructor* URIErrorConstructor() const { return d()->URIErrorConstructor; }

        GlobalEvalFunction* evalFunction() const { return d()->evalFunction; }

        ObjectPrototype* objectPrototype() const { return d()->objectPrototype; }
        FunctionPrototype* functionPrototype() const { return d()->functionPrototype; }
        ArrayPrototype* arrayPrototype() const { return d()->arrayPrototype; }
        BooleanPrototype* booleanPrototype() const { return d()->booleanPrototype; }
        StringPrototype* stringPrototype() const { return d()->stringPrototype; }
        NumberPrototype* numberPrototype() const { return d()->numberPrototype; }
        DatePrototype* datePrototype() const { return d()->datePrototype; }
        RegExpPrototype* regExpPrototype() const { return d()->regExpPrototype; }

        TiObject* methodCallDummy() const { return d()->methodCallDummy; }

        Structure* argumentsStructure() const { return d()->argumentsStructure.get(); }
        Structure* arrayStructure() const { return d()->arrayStructure.get(); }
        Structure* booleanObjectStructure() const { return d()->booleanObjectStructure.get(); }
        Structure* callbackConstructorStructure() const { return d()->callbackConstructorStructure.get(); }
        Structure* callbackFunctionStructure() const { return d()->callbackFunctionStructure.get(); }
        Structure* callbackObjectStructure() const { return d()->callbackObjectStructure.get(); }
        Structure* dateStructure() const { return d()->dateStructure.get(); }
        Structure* emptyObjectStructure() const { return d()->emptyObjectStructure.get(); }
        Structure* errorStructure() const { return d()->errorStructure.get(); }
        Structure* functionStructure() const { return d()->functionStructure.get(); }
        Structure* numberObjectStructure() const { return d()->numberObjectStructure.get(); }
        Structure* prototypeFunctionStructure() const { return d()->prototypeFunctionStructure.get(); }
        Structure* regExpMatchesArrayStructure() const { return d()->regExpMatchesArrayStructure.get(); }
        Structure* regExpStructure() const { return d()->regExpStructure.get(); }
        Structure* stringObjectStructure() const { return d()->stringObjectStructure.get(); }

        void setProfileGroup(unsigned value) { d()->profileGroup = value; }
        unsigned profileGroup() const { return d()->profileGroup; }

        Debugger* debugger() const { return d()->debugger; }
        void setDebugger(Debugger* debugger) { d()->debugger = debugger; }
        
        virtual bool supportsProfiling() const { return false; }
        
        int recursion() { return d()->recursion; }
        void incRecursion() { ++d()->recursion; }
        void decRecursion() { --d()->recursion; }
        
        ScopeChain& globalScopeChain() { return d()->globalScopeChain; }

        virtual bool isGlobalObject() const { return true; }

        virtual TiExcState* globalExec();

        virtual bool shouldInterruptScriptBeforeTimeout() const { return false; }
        virtual bool shouldInterruptScript() const { return true; }

        virtual bool allowsAccessFrom(const TiGlobalObject*) const { return true; }

        virtual bool isDynamicScope(bool& requiresDynamicChecks) const;

        HashSet<GlobalCodeBlock*>& codeBlocks() { return d()->codeBlocks; }

        void copyGlobalsFrom(RegisterFile&);
        void copyGlobalsTo(RegisterFile&);
        
        void resetPrototype(TiValue prototype);

        TiGlobalData* globalData() { return d()->globalData.get(); }
        TiGlobalObjectData* d() const { return static_cast<TiGlobalObjectData*>(JSVariableObject::d); }

        static PassRefPtr<Structure> createStructure(TiValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount);
        }

        void registerWeakMap(OpaqueJSWeakObjectMap* map)
        {
            d()->weakMaps.add(map);
        }

        void deregisterWeakMap(OpaqueJSWeakObjectMap* map)
        {
            d()->weakMaps.remove(map);
        }

        double weakRandomNumber() { return d()->weakRandom.get(); }
    protected:

        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesMarkChildren | OverridesGetPropertyNames | JSVariableObject::StructureFlags;

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
        static void destroyTiGlobalObjectData(void*);

        // FIXME: Fold reset into init.
        void init(TiObject* thisValue);
        void reset(TiValue prototype);

        void setRegisters(Register* registers, Register* registerArray, size_t count);

        void* operator new(size_t); // can only be allocated with TiGlobalData
    };

    TiGlobalObject* asGlobalObject(TiValue);

    inline TiGlobalObject* asGlobalObject(TiValue value)
    {
        ASSERT(asObject(value)->isGlobalObject());
        return static_cast<TiGlobalObject*>(asObject(value));
    }

    inline void TiGlobalObject::setRegisters(Register* registers, Register* registerArray, size_t count)
    {
        JSVariableObject::setRegisters(registers, registerArray);
        d()->registerArraySize = count;
    }

    inline void TiGlobalObject::addStaticGlobals(GlobalPropertyInfo* globals, int count)
    {
        size_t oldSize = d()->registerArraySize;
        size_t newSize = oldSize + count;
        Register* registerArray = new Register[newSize];
        if (d()->registerArray)
            memcpy(registerArray + count, d()->registerArray.get(), oldSize * sizeof(Register));
        setRegisters(registerArray + newSize, registerArray, newSize);

        for (int i = 0, index = -static_cast<int>(oldSize) - 1; i < count; ++i, --index) {
            GlobalPropertyInfo& global = globals[i];
            ASSERT(global.attributes & DontDelete);
            SymbolTableEntry newEntry(index, global.attributes);
            symbolTable().add(global.identifier.ustring().rep(), newEntry);
            registerAt(index) = global.value;
        }
    }

    inline bool TiGlobalObject::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        if (JSVariableObject::getOwnPropertySlot(exec, propertyName, slot))
            return true;
        return symbolTableGet(propertyName, slot);
    }

    inline bool TiGlobalObject::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
    {
        if (symbolTableGet(propertyName, descriptor))
            return true;
        return JSVariableObject::getOwnPropertyDescriptor(exec, propertyName, descriptor);
    }

    inline bool TiGlobalObject::hasOwnPropertyForWrite(TiExcState* exec, const Identifier& propertyName)
    {
        PropertySlot slot;
        if (JSVariableObject::getOwnPropertySlot(exec, propertyName, slot))
            return true;
        bool slotIsWriteable;
        return symbolTableGet(propertyName, slot, slotIsWriteable);
    }

    inline TiValue Structure::prototypeForLookup(TiExcState* exec) const
    {
        if (typeInfo().type() == ObjectType)
            return m_prototype;

#if USE(JSVALUE32)
        if (typeInfo().type() == StringType)
            return exec->lexicalGlobalObject()->stringPrototype();

        ASSERT(typeInfo().type() == NumberType);
        return exec->lexicalGlobalObject()->numberPrototype();
#else
        ASSERT(typeInfo().type() == StringType);
        return exec->lexicalGlobalObject()->stringPrototype();
#endif
    }

    inline StructureChain* Structure::prototypeChain(TiExcState* exec) const
    {
        // We cache our prototype chain so our clients can share it.
        if (!isValid(exec, m_cachedPrototypeChain.get())) {
            TiValue prototype = prototypeForLookup(exec);
            m_cachedPrototypeChain = StructureChain::create(prototype.isNull() ? 0 : asObject(prototype)->structure());
        }
        return m_cachedPrototypeChain.get();
    }

    inline bool Structure::isValid(TiExcState* exec, StructureChain* cachedPrototypeChain) const
    {
        if (!cachedPrototypeChain)
            return false;

        TiValue prototype = prototypeForLookup(exec);
        RefPtr<Structure>* cachedStructure = cachedPrototypeChain->head();
        while(*cachedStructure && !prototype.isNull()) {
            if (asObject(prototype)->structure() != *cachedStructure)
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

    inline TiObject* constructEmptyObject(TiExcState* exec)
    {
        return new (exec) TiObject(exec->lexicalGlobalObject()->emptyObjectStructure());
    }
    
    inline TiObject* constructEmptyObject(TiExcState* exec, TiGlobalObject* globalObject)
    {
        return new (exec) TiObject(globalObject->emptyObjectStructure());
    }

    inline TiArray* constructEmptyArray(TiExcState* exec)
    {
        return new (exec) TiArray(exec->lexicalGlobalObject()->arrayStructure());
    }
    
    inline TiArray* constructEmptyArray(TiExcState* exec, TiGlobalObject* globalObject)
    {
        return new (exec) TiArray(globalObject->arrayStructure());
    }

    inline TiArray* constructEmptyArray(TiExcState* exec, unsigned initialLength)
    {
        return new (exec) TiArray(exec->lexicalGlobalObject()->arrayStructure(), initialLength);
    }

    inline TiArray* constructArray(TiExcState* exec, TiValue singleItemValue)
    {
        MarkedArgumentBuffer values;
        values.append(singleItemValue);
        return new (exec) TiArray(exec->lexicalGlobalObject()->arrayStructure(), values);
    }

    inline TiArray* constructArray(TiExcState* exec, const ArgList& values)
    {
        return new (exec) TiArray(exec->lexicalGlobalObject()->arrayStructure(), values);
    }

    class DynamicGlobalObjectScope : public Noncopyable {
    public:
        DynamicGlobalObjectScope(CallFrame* callFrame, TiGlobalObject* dynamicGlobalObject);

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
