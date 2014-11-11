/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "TiClassRef.h"

#include "APICast.h"
#include "Identifier.h"
#include "InitializeThreading.h"
#include "JSCallbackObject.h"
#include "JSGlobalObject.h"
#include "TiObjectRef.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include <wtf/text/StringHash.h>
#include <wtf/unicode/UTF8.h>

using namespace std;
using namespace TI;
using namespace WTI::Unicode;

const TiClassDefinition kTiClassDefinitionEmpty = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

OpaqueTiClass::OpaqueTiClass(const TiClassDefinition* definition, OpaqueTiClass* protoClass) 
    : parentClass(definition->parentClass)
    , prototypeClass(0)
    , initialize(definition->initialize)
    , finalize(definition->finalize)
    , hasProperty(definition->hasProperty)
    , getProperty(definition->getProperty)
    , setProperty(definition->setProperty)
    , deleteProperty(definition->deleteProperty)
    , getPropertyNames(definition->getPropertyNames)
    , callAsFunction(definition->callAsFunction)
    , callAsConstructor(definition->callAsConstructor)
    , hasInstance(definition->hasInstance)
    , convertToType(definition->convertToType)
    , m_className(String::fromUTF8(definition->className))
{
    initializeThreading();

    if (const TiStaticValue* staticValue = definition->staticValues) {
        m_staticValues = adoptPtr(new OpaqueTiClassStaticValuesTable);
        while (staticValue->name) {
            String valueName = String::fromUTF8(staticValue->name);
            if (!valueName.isNull())
                m_staticValues->set(valueName.impl(), std::make_unique<StaticValueEntry>(staticValue->getProperty, staticValue->setProperty, staticValue->attributes, valueName));
            ++staticValue;
        }
    }

    if (const TiStaticFunction* staticFunction = definition->staticFunctions) {
        m_staticFunctions = adoptPtr(new OpaqueTiClassStaticFunctionsTable);
        while (staticFunction->name) {
            String functionName = String::fromUTF8(staticFunction->name);
            if (!functionName.isNull())
                m_staticFunctions->set(functionName.impl(), std::make_unique<StaticFunctionEntry>(staticFunction->callAsFunction, staticFunction->attributes));
            ++staticFunction;
        }
    }
        
    if (protoClass)
        prototypeClass = TiClassRetain(protoClass);
}

OpaqueTiClass::~OpaqueTiClass()
{
    // The empty string is shared across threads & is an identifier, in all other cases we should have done a deep copy in className(), below. 
    ASSERT(!m_className.length() || !m_className.impl()->isIdentifier());

#ifndef NDEBUG
    if (m_staticValues) {
        OpaqueTiClassStaticValuesTable::const_iterator end = m_staticValues->end();
        for (OpaqueTiClassStaticValuesTable::const_iterator it = m_staticValues->begin(); it != end; ++it)
            ASSERT(!it->key->isIdentifier());
    }

    if (m_staticFunctions) {
        OpaqueTiClassStaticFunctionsTable::const_iterator end = m_staticFunctions->end();
        for (OpaqueTiClassStaticFunctionsTable::const_iterator it = m_staticFunctions->begin(); it != end; ++it)
            ASSERT(!it->key->isIdentifier());
    }
#endif
    
    if (prototypeClass)
        TiClassRelease(prototypeClass);
}

PassRefPtr<OpaqueTiClass> OpaqueTiClass::createNoAutomaticPrototype(const TiClassDefinition* definition)
{
    return adoptRef(new OpaqueTiClass(definition, 0));
}

PassRefPtr<OpaqueTiClass> OpaqueTiClass::create(const TiClassDefinition* clientDefinition)
{
    TiClassDefinition definition = *clientDefinition; // Avoid modifying client copy.

    TiClassDefinition protoDefinition = kTiClassDefinitionEmpty;
    protoDefinition.finalize = 0;
    swap(definition.staticFunctions, protoDefinition.staticFunctions); // Move static functions to the prototype.
    
    // We are supposed to use TiClassRetain/Release but since we know that we currently have
    // the only reference to this class object we cheat and use a RefPtr instead.
    RefPtr<OpaqueTiClass> protoClass = adoptRef(new OpaqueTiClass(&protoDefinition, 0));
    return adoptRef(new OpaqueTiClass(&definition, protoClass.get()));
}

OpaqueTiClassContextData::OpaqueTiClassContextData(TI::VM&, OpaqueTiClass* jsClass)
    : m_class(jsClass)
{
    if (jsClass->m_staticValues) {
        staticValues = std::make_unique<OpaqueTiClassStaticValuesTable>();
        OpaqueTiClassStaticValuesTable::const_iterator end = jsClass->m_staticValues->end();
        for (OpaqueTiClassStaticValuesTable::const_iterator it = jsClass->m_staticValues->begin(); it != end; ++it) {
            ASSERT(!it->key->isIdentifier());
            String valueName = it->key->isolatedCopy();
            staticValues->add(valueName.impl(), std::make_unique<StaticValueEntry>(it->value->getProperty, it->value->setProperty, it->value->attributes, valueName));
        }
    }

    if (jsClass->m_staticFunctions) {
        staticFunctions = std::make_unique<OpaqueTiClassStaticFunctionsTable>();
        OpaqueTiClassStaticFunctionsTable::const_iterator end = jsClass->m_staticFunctions->end();
        for (OpaqueTiClassStaticFunctionsTable::const_iterator it = jsClass->m_staticFunctions->begin(); it != end; ++it) {
            ASSERT(!it->key->isIdentifier());
            staticFunctions->add(it->key->isolatedCopy(), std::make_unique<StaticFunctionEntry>(it->value->callAsFunction, it->value->attributes));
        }
    }
}

OpaqueTiClassContextData& OpaqueTiClass::contextData(ExecState* exec)
{
    std::unique_ptr<OpaqueTiClassContextData>& contextData = exec->lexicalGlobalObject()->opaqueTiClassData().add(this, nullptr).iterator->value;
    if (!contextData)
        contextData = std::make_unique<OpaqueTiClassContextData>(exec->vm(), this);
    return *contextData;
}

String OpaqueTiClass::className()
{
    // Make a deep copy, so that the caller has no chance to put the original into IdentifierTable.
    return m_className.isolatedCopy();
}

OpaqueTiClassStaticValuesTable* OpaqueTiClass::staticValues(TI::ExecState* exec)
{
    return contextData(exec).staticValues.get();
}

OpaqueTiClassStaticFunctionsTable* OpaqueTiClass::staticFunctions(TI::ExecState* exec)
{
    return contextData(exec).staticFunctions.get();
}

/*!
// Doc here in case we make this public. (Hopefully we won't.)
@function
 @abstract Returns the prototype that will be used when constructing an object with a given class.
 @param ctx The execution context to use.
 @param jsClass A TiClass whose prototype you want to get.
 @result The JSObject prototype that was automatically generated for jsClass, or NULL if no prototype was automatically generated. This is the prototype that will be used when constructing an object using jsClass.
*/
JSObject* OpaqueTiClass::prototype(ExecState* exec)
{
    /* Class (C++) and prototype (JS) inheritance are parallel, so:
     *     (C++)      |        (JS)
     *   ParentClass  |   ParentClassPrototype
     *       ^        |          ^
     *       |        |          |
     *  DerivedClass  |  DerivedClassPrototype
     */

    if (!prototypeClass)
        return 0;

    OpaqueTiClassContextData& jsClassData = contextData(exec);

    if (JSObject* prototype = jsClassData.cachedPrototype.get())
        return prototype;

    // Recursive, but should be good enough for our purposes
    JSObject* prototype = JSCallbackObject<JSDestructibleObject>::create(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->callbackObjectStructure(), prototypeClass, &jsClassData); // set jsClassData as the object's private data, so it can clear our reference on destruction
    if (parentClass) {
        if (JSObject* parentPrototype = parentClass->prototype(exec))
            prototype->setPrototype(exec->vm(), parentPrototype);
    }

    jsClassData.cachedPrototype = Weak<JSObject>(prototype);
    return prototype;
}
