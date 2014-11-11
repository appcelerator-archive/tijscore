/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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

#include "TiContextRef.h"
#include "JSNode.h"
#include "TiObjectRef.h"
#include "TiStringRef.h"
#include <stdio.h>
#include <stdlib.h>
#include <wtf/Assertions.h>

static char* createStringWithContentsOfFile(const char* fileName);
static TiValueRef print(TiContextRef context, TiObjectRef object, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception);

int main(int argc, char* argv[])
{
    const char *scriptPath = "minidom.js";
    if (argc > 1) {
        scriptPath = argv[1];
    }
    
    TiGlobalContextRef context = TiGlobalContextCreateInGroup(NULL, NULL);
    TiObjectRef globalObject = TiContextGetGlobalObject(context);
    
    TiStringRef printIString = TiStringCreateWithUTF8CString("print");
    TiObjectSetProperty(context, globalObject, printIString, TiObjectMakeFunctionWithCallback(context, printIString, print), kTiPropertyAttributeNone, NULL);
    TiStringRelease(printIString);
    
    TiStringRef node = TiStringCreateWithUTF8CString("Node");
    TiObjectSetProperty(context, globalObject, node, TiObjectMakeConstructor(context, JSNode_class(context), JSNode_construct), kTiPropertyAttributeNone, NULL);
    TiStringRelease(node);
    
    char* scriptUTF8 = createStringWithContentsOfFile(scriptPath);
    TiStringRef script = TiStringCreateWithUTF8CString(scriptUTF8);
    TiValueRef exception;
    TiValueRef result = TiEvalScript(context, script, NULL, NULL, 1, &exception);
    if (result)
        printf("PASS: Test script executed successfully.\n");
    else {
        printf("FAIL: Test script threw exception:\n");
        TiStringRef exceptionIString = TiValueToStringCopy(context, exception, NULL);
        size_t exceptionUTF8Size = TiStringGetMaximumUTF8CStringSize(exceptionIString);
        char* exceptionUTF8 = (char*)malloc(exceptionUTF8Size);
        TiStringGetUTF8CString(exceptionIString, exceptionUTF8, exceptionUTF8Size);
        printf("%s\n", exceptionUTF8);
        free(exceptionUTF8);
        TiStringRelease(exceptionIString);
    }
    TiStringRelease(script);
    free(scriptUTF8);

    globalObject = 0;
    TiGlobalContextRelease(context);
    printf("PASS: Program exited normally.\n");
    return 0;
}

static TiValueRef print(TiContextRef context, TiObjectRef object, TiObjectRef thisObject, size_t argumentCount, const TiValueRef arguments[], TiValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

    if (argumentCount > 0) {
        TiStringRef string = TiValueToStringCopy(context, arguments[0], exception);
        size_t numChars = TiStringGetMaximumUTF8CStringSize(string);
        char stringUTF8[numChars];
        TiStringGetUTF8CString(string, stringUTF8, numChars);
        printf("%s\n", stringUTF8);
    }
    
    return TiValueMakeUndefined(context);
}

static char* createStringWithContentsOfFile(const char* fileName)
{
    char* buffer;
    
    size_t buffer_size = 0;
    size_t buffer_capacity = 1024;
    buffer = (char*)malloc(buffer_capacity);
    
    FILE* f = fopen(fileName, "r");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", fileName);
        free(buffer);
        return 0;
    }
    
    while (!feof(f) && !ferror(f)) {
        buffer_size += fread(buffer + buffer_size, 1, buffer_capacity - buffer_size, f);
        if (buffer_size == buffer_capacity) { /* guarantees space for trailing '\0' */
            buffer_capacity *= 2;
            buffer = (char*)realloc(buffer, buffer_capacity);
            ASSERT(buffer);
        }
        
        ASSERT(buffer_size < buffer_capacity);
    }
    fclose(f);
    buffer[buffer_size] = '\0';
    
    return buffer;
}
