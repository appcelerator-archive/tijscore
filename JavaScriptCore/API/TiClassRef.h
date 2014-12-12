/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef TiClassRef_h
#define TiClassRef_h

#include "OpaqueTiString.h"
#include "Protect.h"
#include "Weak.h"
#include <JavaScriptCore/TiObjectRef.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

struct StaticValueEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StaticValueEntry(TiObjectGetPropertyCallback _getProperty, TiObjectSetPropertyCallback _setProperty, TiPropertyAttributes _attributes, String& propertyName)
    : getProperty(_getProperty), setProperty(_setProperty), attributes(_attributes), propertyNameRef(OpaqueTiString::create(propertyName))
    {
    }
    
    TiObjectGetPropertyCallback getProperty;
    TiObjectSetPropertyCallback setProperty;
    TiPropertyAttributes attributes;
    RefPtr<OpaqueTiString> propertyNameRef;
};

struct StaticFunctionEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StaticFunctionEntry(TiObjectCallAsFunctionCallback _callAsFunction, TiPropertyAttributes _attributes)
        : callAsFunction(_callAsFunction), attributes(_attributes)
    {
    }

    TiObjectCallAsFunctionCallback callAsFunction;
    TiPropertyAttributes attributes;
};

typedef HashMap<RefPtr<StringImpl>, std::unique_ptr<StaticValueEntry>> OpaqueTiClassStaticValuesTable;
typedef HashMap<RefPtr<StringImpl>, std::unique_ptr<StaticFunctionEntry>> OpaqueTiClassStaticFunctionsTable;

struct OpaqueTiClass;

// An OpaqueTiClass (TiClass) is created without a context, so it can be used with any context, even across context groups.
// This structure holds data members that vary across context groups.
struct OpaqueTiClassContextData {
    WTF_MAKE_NONCOPYABLE(OpaqueTiClassContextData); WTF_MAKE_FAST_ALLOCATED;
public:
    OpaqueTiClassContextData(TI::VM&, OpaqueTiClass*);

    // It is necessary to keep OpaqueTiClass alive because of the following rare scenario:
    // 1. A class is created and used, so its context data is stored in VM hash map.
    // 2. The class is released, and when all JS objects that use it are collected, OpaqueTiClass
    // is deleted (that's the part prevented by this RefPtr).
    // 3. Another class is created at the same address.
    // 4. When it is used, the old context data is found in VM and used.
    RefPtr<OpaqueTiClass> m_class;

    std::unique_ptr<OpaqueTiClassStaticValuesTable> staticValues;
    std::unique_ptr<OpaqueTiClassStaticFunctionsTable> staticFunctions;
    TI::Weak<TI::JSObject> cachedPrototype;
};

struct OpaqueTiClass : public ThreadSafeRefCounted<OpaqueTiClass> {
    static PassRefPtr<OpaqueTiClass> create(const TiClassDefinition*);
    static PassRefPtr<OpaqueTiClass> createNoAutomaticPrototype(const TiClassDefinition*);
    JS_EXPORT_PRIVATE ~OpaqueTiClass();
    
    String className();
    OpaqueTiClassStaticValuesTable* staticValues(TI::ExecState*);
    OpaqueTiClassStaticFunctionsTable* staticFunctions(TI::ExecState*);
    TI::JSObject* prototype(TI::ExecState*);

    OpaqueTiClass* parentClass;
    OpaqueTiClass* prototypeClass;
    
    TiObjectInitializeCallback initialize;
    TiObjectFinalizeCallback finalize;
    TiObjectHasPropertyCallback hasProperty;
    TiObjectGetPropertyCallback getProperty;
    TiObjectSetPropertyCallback setProperty;
    TiObjectDeletePropertyCallback deleteProperty;
    TiObjectGetPropertyNamesCallback getPropertyNames;
    TiObjectCallAsFunctionCallback callAsFunction;
    TiObjectCallAsConstructorCallback callAsConstructor;
    TiObjectHasInstanceCallback hasInstance;
    TiObjectConvertToTypeCallback convertToType;

private:
    friend struct OpaqueTiClassContextData;

    OpaqueTiClass();
    OpaqueTiClass(const OpaqueTiClass&);
    OpaqueTiClass(const TiClassDefinition*, OpaqueTiClass* protoClass);

    OpaqueTiClassContextData& contextData(TI::ExecState*);

    // Strings in these data members should not be put into any IdentifierTable.
    String m_className;
    OwnPtr<OpaqueTiClassStaticValuesTable> m_staticValues;
    OwnPtr<OpaqueTiClassStaticFunctionsTable> m_staticFunctions;
};

#endif // TiClassRef_h
