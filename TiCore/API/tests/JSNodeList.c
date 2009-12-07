/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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

#include "JSNode.h"
#include "JSNodeList.h"
#include "TiObjectRef.h"
#include "TiValueRef.h"
#include "UnusedParam.h"
#include <wtf/Assertions.h>

static TiValueRef JSNodeList_item(TiContextRef context, TiObjectRef object, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(object);

    if (argumentCount > 0) {
        NodeList* nodeList = TiObjectGetPrivate(thisObject);
        ASSERT(nodeList);
        Node* node = NodeList_item(nodeList, (unsigned)TiValueToNumber(context, arguments[0], exception));
        if (node)
            return JSNode_new(context, node);
    }
    
    return TiValueMakeUndefined(context);
}

static TiStaticFunction JSNodeList_staticFunctions[] = {
    { "item", JSNodeList_item, kTiPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static TiValueRef JSNodeList_length(TiContextRef context, TiObjectRef thisObject, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);
    
    NodeList* nodeList = TiObjectGetPrivate(thisObject);
    ASSERT(nodeList);
    return TiValueMakeNumber(context, NodeList_length(nodeList));
}

static TiStaticValue JSNodeList_staticValues[] = {
    { "length", JSNodeList_length, NULL, kTiPropertyAttributeReadOnly | kTiPropertyAttributeDontDelete },
    { 0, 0, 0, 0 }
};

static TiValueRef JSNodeList_getProperty(TiContextRef context, TiObjectRef thisObject, TiStringRef propertyName, TiValueRef* exception)
{
    NodeList* nodeList = TiObjectGetPrivate(thisObject);
    ASSERT(nodeList);
    double index = TiValueToNumber(context, TiValueMakeString(context, propertyName), exception);
    unsigned uindex = (unsigned)index;
    if (uindex == index) { /* false for NaN */
        Node* node = NodeList_item(nodeList, uindex);
        if (node)
            return JSNode_new(context, node);
    }
    
    return NULL;
}

static void JSNodeList_initialize(TiContextRef context, TiObjectRef thisObject)
{
    UNUSED_PARAM(context);

    NodeList* nodeList = TiObjectGetPrivate(thisObject);
    ASSERT(nodeList);
    
    NodeList_ref(nodeList);
}

static void JSNodeList_finalize(TiObjectRef thisObject)
{
    NodeList* nodeList = TiObjectGetPrivate(thisObject);
    ASSERT(nodeList);

    NodeList_deref(nodeList);
}

static TiClassRef JSNodeList_class(TiContextRef context)
{
    UNUSED_PARAM(context);

    static TiClassRef jsClass;
    if (!jsClass) {
        TiClassDefinition definition = kTiClassDefinitionEmpty;
        definition.staticValues = JSNodeList_staticValues;
        definition.staticFunctions = JSNodeList_staticFunctions;
        definition.getProperty = JSNodeList_getProperty;
        definition.initialize = JSNodeList_initialize;
        definition.finalize = JSNodeList_finalize;

        jsClass = TiClassCreate(&definition);
    }
    
    return jsClass;
}

TiObjectRef JSNodeList_new(TiContextRef context, NodeList* nodeList)
{
    return TiObjectMake(context, JSNodeList_class(context), nodeList);
}
