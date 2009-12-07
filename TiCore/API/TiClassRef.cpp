/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "TiCallbackObject.h"
#include "TiObjectRef.h"
#include <runtime/InitializeThreading.h>
#include <runtime/TiGlobalObject.h>
#include <runtime/ObjectPrototype.h>
#include <runtime/Identifier.h>

using namespace TI;

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
    , m_className(UString::Rep::createFromUTF8(definition->className))
    , m_staticValues(0)
    , m_staticFunctions(0)
{
    initializeThreading();

    if (const TiStaticValue* staticValue = definition->staticValues) {
        m_staticValues = new OpaqueTiClassStaticValuesTable();
        while (staticValue->name) {
            m_staticValues->add(UString::Rep::createFromUTF8(staticValue->name),
                              new StaticValueEntry(staticValue->getProperty, staticValue->setProperty, staticValue->attributes));
            ++staticValue;
        }
    }

    if (const TiStaticFunction* staticFunction = definition->staticFunctions) {
        m_staticFunctions = new OpaqueTiClassStaticFunctionsTable();
        while (staticFunction->name) {
            m_staticFunctions->add(UString::Rep::createFromUTF8(staticFunction->name),
                                 new StaticFunctionEntry(staticFunction->callAsFunction, staticFunction->attributes));
            ++staticFunction;
        }
    }
        
    if (protoClass)
        prototypeClass = TiClassRetain(protoClass);
}

OpaqueTiClass::~OpaqueTiClass()
{
    ASSERT(!m_className.rep()->identifierTable());

    if (m_staticValues) {
        OpaqueTiClassStaticValuesTable::const_iterator end = m_staticValues->end();
        for (OpaqueTiClassStaticValuesTable::const_iterator it = m_staticValues->begin(); it != end; ++it) {
            ASSERT(!it->first->identifierTable());
            delete it->second;
        }
        delete m_staticValues;
    }

    if (m_staticFunctions) {
        OpaqueTiClassStaticFunctionsTable::const_iterator end = m_staticFunctions->end();
        for (OpaqueTiClassStaticFunctionsTable::const_iterator it = m_staticFunctions->begin(); it != end; ++it) {
            ASSERT(!it->first->identifierTable());
            delete it->second;
        }
        delete m_staticFunctions;
    }
    
    if (prototypeClass)
        TiClassRelease(prototypeClass);
}

PassRefPtr<OpaqueTiClass> OpaqueTiClass::createNoAutomaticPrototype(const TiClassDefinition* definition)
{
    return adoptRef(new OpaqueTiClass(definition, 0));
}

static void clearReferenceToPrototype(TiObjectRef prototype)
{
    OpaqueTiClassContextData* jsClassData = static_cast<OpaqueTiClassContextData*>(TiObjectGetPrivate(prototype));
    ASSERT(jsClassData);
    jsClassData->cachedPrototype = 0;
}

PassRefPtr<OpaqueTiClass> OpaqueTiClass::create(const TiClassDefinition* definition)
{
    if (const TiStaticFunction* staticFunctions = definition->staticFunctions) {
        // copy functions into a prototype class
        TiClassDefinition protoDefinition = kTiClassDefinitionEmpty;
        protoDefinition.staticFunctions = staticFunctions;
        protoDefinition.finalize = clearReferenceToPrototype;
        
        // We are supposed to use TiClassRetain/Release but since we know that we currently have
        // the only reference to this class object we cheat and use a RefPtr instead.
        RefPtr<OpaqueTiClass> protoClass = adoptRef(new OpaqueTiClass(&protoDefinition, 0));

        // remove functions from the original class
        TiClassDefinition objectDefinition = *definition;
        objectDefinition.staticFunctions = 0;

        return adoptRef(new OpaqueTiClass(&objectDefinition, protoClass.get()));
    }

    return adoptRef(new OpaqueTiClass(definition, 0));
}

OpaqueTiClassContextData::OpaqueTiClassContextData(OpaqueTiClass* jsClass)
    : m_class(jsClass)
    , cachedPrototype(0)
{
    if (jsClass->m_staticValues) {
        staticValues = new OpaqueTiClassStaticValuesTable;
        OpaqueTiClassStaticValuesTable::const_iterator end = jsClass->m_staticValues->end();
        for (OpaqueTiClassStaticValuesTable::const_iterator it = jsClass->m_staticValues->begin(); it != end; ++it) {
            ASSERT(!it->first->identifierTable());
            staticValues->add(UString::Rep::createCopying(it->first->data(), it->first->size()),
                              new StaticValueEntry(it->second->getProperty, it->second->setProperty, it->second->attributes));
        }
            
    } else
        staticValues = 0;
        

    if (jsClass->m_staticFunctions) {
        staticFunctions = new OpaqueTiClassStaticFunctionsTable;
        OpaqueTiClassStaticFunctionsTable::const_iterator end = jsClass->m_staticFunctions->end();
        for (OpaqueTiClassStaticFunctionsTable::const_iterator it = jsClass->m_staticFunctions->begin(); it != end; ++it) {
            ASSERT(!it->first->identifierTable());
            staticFunctions->add(UString::Rep::createCopying(it->first->data(), it->first->size()),
                              new StaticFunctionEntry(it->second->callAsFunction, it->second->attributes));
        }
            
    } else
        staticFunctions = 0;
}

OpaqueTiClassContextData::~OpaqueTiClassContextData()
{
    if (staticValues) {
        deleteAllValues(*staticValues);
        delete staticValues;
    }

    if (staticFunctions) {
        deleteAllValues(*staticFunctions);
        delete staticFunctions;
    }
}

OpaqueTiClassContextData& OpaqueTiClass::contextData(TiExcState* exec)
{
    OpaqueTiClassContextData*& contextData = exec->globalData().opaqueTiClassData.add(this, 0).first->second;
    if (!contextData)
        contextData = new OpaqueTiClassContextData(this);
    return *contextData;
}

UString OpaqueTiClass::className()
{
    // Make a deep copy, so that the caller has no chance to put the original into IdentifierTable.
    return UString(m_className.data(), m_className.size());
}

OpaqueTiClassStaticValuesTable* OpaqueTiClass::staticValues(TI::TiExcState* exec)
{
    OpaqueTiClassContextData& jsClassData = contextData(exec);
    return jsClassData.staticValues;
}

OpaqueTiClassStaticFunctionsTable* OpaqueTiClass::staticFunctions(TI::TiExcState* exec)
{
    OpaqueTiClassContextData& jsClassData = contextData(exec);
    return jsClassData.staticFunctions;
}

/*!
// Doc here in case we make this public. (Hopefully we won't.)
@function
 @abstract Returns the prototype that will be used when constructing an object with a given class.
 @param ctx The execution context to use.
 @param jsClass A TiClass whose prototype you want to get.
 @result The TiObject prototype that was automatically generated for jsClass, or NULL if no prototype was automatically generated. This is the prototype that will be used when constructing an object using jsClass.
*/
TiObject* OpaqueTiClass::prototype(TiExcState* exec)
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

    if (!jsClassData.cachedPrototype) {
        // Recursive, but should be good enough for our purposes
        jsClassData.cachedPrototype = new (exec) TiCallbackObject<TiObject>(exec, exec->lexicalGlobalObject()->callbackObjectStructure(), prototypeClass, &jsClassData); // set jsClassData as the object's private data, so it can clear our reference on destruction
        if (parentClass) {
            if (TiObject* prototype = parentClass->prototype(exec))
                jsClassData.cachedPrototype->setPrototype(prototype);
        }
    }
    return jsClassData.cachedPrototype;
}
