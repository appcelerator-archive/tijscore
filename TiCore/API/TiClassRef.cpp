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
#include <wtf/text/StringHash.h>
#include <wtf/unicode/UTF8.h>

using namespace std;
using namespace TI;
using namespace WTI::Unicode;

const TiClassDefinition kTiClassDefinitionEmpty = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static inline UString tryCreateStringFromUTF8(const char* string)
{
    if (!string)
        return UString::null();

    size_t length = strlen(string);
    Vector<UChar, 1024> buffer(length);
    UChar* p = buffer.data();
    if (conversionOK != convertUTF8ToUTF16(&string, string + length, &p, p + length))
        return UString::null();

    return UString(buffer.data(), p - buffer.data());
}

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
    , m_className(tryCreateStringFromUTF8(definition->className))
    , m_staticValues(0)
    , m_staticFunctions(0)
{
    initializeThreading();

    if (const TiStaticValue* staticValue = definition->staticValues) {
        m_staticValues = new OpaqueTiClassStaticValuesTable();
        while (staticValue->name) {
            UString valueName = tryCreateStringFromUTF8(staticValue->name);
            if (!valueName.isNull()) {
                // Use a local variable here to sidestep an RVCT compiler bug.
                StaticValueEntry* entry = new StaticValueEntry(staticValue->getProperty, staticValue->setProperty, staticValue->attributes);
                UStringImpl* impl = valueName.rep();
                impl->ref();
                m_staticValues->add(impl, entry);
            }
            ++staticValue;
        }
    }

    if (const TiStaticFunction* staticFunction = definition->staticFunctions) {
        m_staticFunctions = new OpaqueTiClassStaticFunctionsTable();
        while (staticFunction->name) {
            UString functionName = tryCreateStringFromUTF8(staticFunction->name);
            if (!functionName.isNull()) {
                // Use a local variable here to sidestep an RVCT compiler bug.
                StaticFunctionEntry* entry = new StaticFunctionEntry(staticFunction->callAsFunction, staticFunction->attributes);
                UStringImpl* impl = functionName.rep();
                impl->ref();
                m_staticFunctions->add(impl, entry);
            }
            ++staticFunction;
        }
    }
        
    if (protoClass)
        prototypeClass = TiClassRetain(protoClass);
}

OpaqueTiClass::~OpaqueTiClass()
{
    // The empty string is shared across threads & is an identifier, in all other cases we should have done a deep copy in className(), below. 
    ASSERT(!m_className.size() || !m_className.rep()->isIdentifier());

    if (m_staticValues) {
        OpaqueTiClassStaticValuesTable::const_iterator end = m_staticValues->end();
        for (OpaqueTiClassStaticValuesTable::const_iterator it = m_staticValues->begin(); it != end; ++it) {
            ASSERT(!it->first->isIdentifier());
            delete it->second;
        }
        delete m_staticValues;
    }

    if (m_staticFunctions) {
        OpaqueTiClassStaticFunctionsTable::const_iterator end = m_staticFunctions->end();
        for (OpaqueTiClassStaticFunctionsTable::const_iterator it = m_staticFunctions->begin(); it != end; ++it) {
            ASSERT(!it->first->isIdentifier());
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
    jsClassData->cachedPrototype.clear(toJS(prototype));
}

PassRefPtr<OpaqueTiClass> OpaqueTiClass::create(const TiClassDefinition* clientDefinition)
{
    TiClassDefinition definition = *clientDefinition; // Avoid modifying client copy.

    TiClassDefinition protoDefinition = kTiClassDefinitionEmpty;
    protoDefinition.finalize = clearReferenceToPrototype;
    swap(definition.staticFunctions, protoDefinition.staticFunctions); // Move static functions to the prototype.
    
    // We are supposed to use TiClassRetain/Release but since we know that we currently have
    // the only reference to this class object we cheat and use a RefPtr instead.
    RefPtr<OpaqueTiClass> protoClass = adoptRef(new OpaqueTiClass(&protoDefinition, 0));
    return adoptRef(new OpaqueTiClass(&definition, protoClass.get()));
}

OpaqueTiClassContextData::OpaqueTiClassContextData(OpaqueTiClass* jsClass)
    : m_class(jsClass)
{
    if (jsClass->m_staticValues) {
        staticValues = new OpaqueTiClassStaticValuesTable;
        OpaqueTiClassStaticValuesTable::const_iterator end = jsClass->m_staticValues->end();
        for (OpaqueTiClassStaticValuesTable::const_iterator it = jsClass->m_staticValues->begin(); it != end; ++it) {
            ASSERT(!it->first->isIdentifier());
            // Use a local variable here to sidestep an RVCT compiler bug.
            StaticValueEntry* entry = new StaticValueEntry(it->second->getProperty, it->second->setProperty, it->second->attributes);
            staticValues->add(UString::Rep::create(it->first->characters(), it->first->length()), entry);
        }
    } else
        staticValues = 0;

    if (jsClass->m_staticFunctions) {
        staticFunctions = new OpaqueTiClassStaticFunctionsTable;
        OpaqueTiClassStaticFunctionsTable::const_iterator end = jsClass->m_staticFunctions->end();
        for (OpaqueTiClassStaticFunctionsTable::const_iterator it = jsClass->m_staticFunctions->begin(); it != end; ++it) {
            ASSERT(!it->first->isIdentifier());
            // Use a local variable here to sidestep an RVCT compiler bug.
            StaticFunctionEntry* entry = new StaticFunctionEntry(it->second->callAsFunction, it->second->attributes);
            staticFunctions->add(UString::Rep::create(it->first->characters(), it->first->length()), entry);
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
    return jsClassData.cachedPrototype.get();
}
