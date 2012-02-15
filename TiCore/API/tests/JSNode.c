/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
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
#include "TiStringRef.h"
#include "TiValueRef.h"
#include "Node.h"
#include "NodeList.h"
#include "UnusedParam.h"
#include <wtf/Assertions.h>

static TiValueRef JSNode_appendChild(TiContextRef context, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);

    /* Example of throwing a type error for invalid values */
    if (!TiValueIsObjectOfClass(context, thisObject, JSNode_class(context))) {
        TiStringRef message = TiStringCreateWithUTF8CString("TypeError: appendChild can only be called on nodes");
        *exception = TiValueMakeString(context, message);
        TiStringRelease(message);
    } else if (argumentCount < 1 || !TiValueIsObjectOfClass(context, arguments[0], JSNode_class(context))) {
        TiStringRef message = TiStringCreateWithUTF8CString("TypeError: first argument to appendChild must be a node");
        *exception = TiValueMakeString(context, message);
        TiStringRelease(message);
    } else {
        Node* node = TiObjectGetPrivate(thisObject);
        Node* child = TiObjectGetPrivate(TiValueToObject(context, arguments[0], NULL));

        Node_appendChild(node, child);
    }

    return TiValueMakeUndefined(context);
}

static TiValueRef JSNode_removeChild(TiContextRef context, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);

    /* Example of ignoring invalid values */
    if (argumentCount > 0) {
        if (TiValueIsObjectOfClass(context, thisObject, JSNode_class(context))) {
            if (TiValueIsObjectOfClass(context, arguments[0], JSNode_class(context))) {
                Node* node = TiObjectGetPrivate(thisObject);
                Node* child = TiObjectGetPrivate(TiValueToObject(context, arguments[0], exception));
                
                Node_removeChild(node, child);
            }
        }
    }
    
    return TiValueMakeUndefined(context);
}

static TiValueRef JSNode_replaceChild(TiContextRef context, TiObjectRef function, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(function);
    
    if (argumentCount > 1) {
        if (TiValueIsObjectOfClass(context, thisObject, JSNode_class(context))) {
            if (TiValueIsObjectOfClass(context, arguments[0], JSNode_class(context))) {
                if (TiValueIsObjectOfClass(context, arguments[1], JSNode_class(context))) {
                    Node* node = TiObjectGetPrivate(thisObject);
                    Node* newChild = TiObjectGetPrivate(TiValueToObject(context, arguments[0], exception));
                    Node* oldChild = TiObjectGetPrivate(TiValueToObject(context, arguments[1], exception));
                    
                    Node_replaceChild(node, newChild, oldChild);
                }
            }
        }
    }
    
    return TiValueMakeUndefined(context);
}

static TiStaticFunction JSNode_staticFunctions[] = {
    { "appendChild", JSNode_appendChild, kTiPropertyAttributeDontDelete },
    { "removeChild", JSNode_removeChild, kTiPropertyAttributeDontDelete },
    { "replaceChild", JSNode_replaceChild, kTiPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static TiValueRef JSNode_getNodeType(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    Node* node = TiObjectGetPrivate(object);
    if (node) {
        TiStringRef nodeType = TiStringCreateWithUTF8CString(node->nodeType);
        TiValueRef value = TiValueMakeString(context, nodeType);
        TiStringRelease(nodeType);
        return value;
    }
    
    return NULL;
}

static TiValueRef JSNode_getChildNodes(TiContextRef context, TiObjectRef thisObject, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);

    Node* node = TiObjectGetPrivate(thisObject);
    ASSERT(node);
    return JSNodeList_new(context, NodeList_new(node));
}

static TiValueRef JSNode_getFirstChild(TiContextRef context, TiObjectRef object, TiStringRef propertyName, TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(propertyName);
    UNUSED_PARAM(exception);
    
    return TiValueMakeUndefined(context);
}

static TiStaticValue JSNode_staticValues[] = {
    { "nodeType", JSNode_getNodeType, NULL, kTiPropertyAttributeDontDelete | kTiPropertyAttributeReadOnly },
    { "childNodes", JSNode_getChildNodes, NULL, kTiPropertyAttributeDontDelete | kTiPropertyAttributeReadOnly },
    { "firstChild", JSNode_getFirstChild, NULL, kTiPropertyAttributeDontDelete | kTiPropertyAttributeReadOnly },
    { 0, 0, 0, 0 }
};

static void JSNode_initialize(TiContextRef context, TiObjectRef object)
{
    UNUSED_PARAM(context);

    Node* node = TiObjectGetPrivate(object);
    ASSERT(node);

    Node_ref(node);
}

static void JSNode_finalize(TiObjectRef object)
{
    Node* node = TiObjectGetPrivate(object);
    ASSERT(node);

    Node_deref(node);
}

TiClassRef JSNode_class(TiContextRef context)
{
    UNUSED_PARAM(context);

    static TiClassRef jsClass;
    if (!jsClass) {
        TiClassDefinition definition = kTiClassDefinitionEmpty;
        definition.staticValues = JSNode_staticValues;
        definition.staticFunctions = JSNode_staticFunctions;
        definition.initialize = JSNode_initialize;
        definition.finalize = JSNode_finalize;

        jsClass = TiClassCreate(&definition);
    }
    return jsClass;
}

TiObjectRef JSNode_new(TiContextRef context, Node* node)
{
    return TiObjectMake(context, JSNode_class(context), node);
}

TiObjectRef JSNode_construct(TiContextRef context, TiObjectRef object, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(argumentCount);
    UNUSED_PARAM(arguments);
    UNUSED_PARAM(exception);

    return JSNode_new(context, Node_new());
}
