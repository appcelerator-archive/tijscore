/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
#include "TiStringRef.h"
#include "TiStringRefPrivate.h"

#include "InitializeThreading.h"
#include "OpaqueTiString.h"
#include <wtf/unicode/UTF8.h>

using namespace TI;
using namespace WTI::Unicode;

TiStringRef TiStringCreateWithCharacters(const JSChar* chars, size_t numChars)
{
    initializeThreading();
    return OpaqueTiString::create(chars, numChars).leakRef();
}

TiStringRef TiStringCreateWithUTF8CString(const char* string)
{
    initializeThreading();
    if (string) {
        size_t length = strlen(string);
        Vector<UChar, 1024> buffer(length);
        UChar* p = buffer.data();
        bool sourceIsAllASCII;
        const LChar* stringStart = reinterpret_cast<const LChar*>(string);
        if (conversionOK == convertUTF8ToUTF16(&string, string + length, &p, p + length, &sourceIsAllASCII)) {
            if (sourceIsAllASCII)
                return OpaqueTiString::create(stringStart, length).leakRef();
            return OpaqueTiString::create(buffer.data(), p - buffer.data()).leakRef();
        }
    }

    return OpaqueTiString::create().leakRef();
}

TiStringRef TiStringCreateWithCharactersNoCopy(const JSChar* chars, size_t numChars)
{
    initializeThreading();
    return OpaqueTiString::create(StringImpl::createWithoutCopying(chars, numChars)).leakRef();
}

TiStringRef TiStringRetain(TiStringRef string)
{
    string->ref();
    return string;
}

void TiStringRelease(TiStringRef string)
{
    string->deref();
}

size_t TiStringGetLength(TiStringRef string)
{
    return string->length();
}

const JSChar* TiStringGetCharactersPtr(TiStringRef string)
{
    return string->characters();
}

size_t TiStringGetMaximumUTF8CStringSize(TiStringRef string)
{
    // Any UTF8 character > 3 bytes encodes as a UTF16 surrogate pair.
    return string->length() * 3 + 1; // + 1 for terminating '\0'
}

size_t TiStringGetUTF8CString(TiStringRef string, char* buffer, size_t bufferSize)
{
    if (!bufferSize)
        return 0;

    char* p = buffer;
    const UChar* d = string->characters();
    ConversionResult result = convertUTF16ToUTF8(&d, d + string->length(), &p, p + bufferSize - 1, true);
    *p++ = '\0';
    if (result != conversionOK && result != targetExhausted)
        return 0;

    return p - buffer;
}

bool TiStringIsEqual(TiStringRef a, TiStringRef b)
{
    unsigned len = a->length();
    return len == b->length() && 0 == memcmp(a->characters(), b->characters(), len * sizeof(UChar));
}

bool TiStringIsEqualToUTF8CString(TiStringRef a, const char* b)
{
    TiStringRef bBuf = TiStringCreateWithUTF8CString(b);
    bool result = TiStringIsEqual(a, bBuf);
    TiStringRelease(bBuf);
    
    return result;
}
