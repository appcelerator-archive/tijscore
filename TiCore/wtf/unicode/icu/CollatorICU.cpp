/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Collator.h"

// Because Apple "helpfully" disables access to ICU collation, we have to
// implement our own collation mechanisms using CFString and CFLocale.
// Fortunately, these are ICU-compatible, including ICU's UChar being
// the same sizeof() as UniChar (16 bit)!
//
// If they ever change that, we just need to patch this out to #undef...
// in case they change their minds (again).
#define USE_TI_UCOL_REPLACEMENTS

#if USE(ICU_UNICODE) && !UCONFIG_NO_COLLATION

#include "Assertions.h"
#include "Threading.h"
#include <unicode/ucol.h>
#include <string.h>

#if OS(DARWIN)
#include "RetainPtr.h"
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace WTI {

#ifndef USE_TI_UCOL_REPLACEMENTS
static UCollator* cachedCollator;
static Mutex& cachedCollatorMutex()
{
    AtomicallyInitializedStatic(Mutex&, mutex = *new Mutex);
    return mutex;
}
#endif

Collator::Collator(const char* locale)
    : m_collator(0)
    , m_locale(locale ? strdup(locale) : 0)
    , m_lowerFirst(false)
{
}

PassOwnPtr<Collator> Collator::userDefault()
{
#if OS(DARWIN) && USE(CF)
    // Mac OS X doesn't set UNIX locale to match user-selected one, so ICU default doesn't work.
#if !defined(BUILDING_ON_LEOPARD) && !OS(IOS)
    RetainPtr<CFLocaleRef> currentLocale(AdoptCF, CFLocaleCopyCurrent());
    CFStringRef collationOrder = (CFStringRef)CFLocaleGetValue(currentLocale.get(), kCFLocaleCollatorIdentifier);
#else
    RetainPtr<CFStringRef> collationOrderRetainer(AdoptCF, (CFStringRef)CFPreferencesCopyValue(CFSTR("AppleCollationOrder"), kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesAnyHost));
    CFStringRef collationOrder = collationOrderRetainer.get();
#endif
    char buf[256];
    if (!collationOrder)
        return adoptPtr(new Collator(""));
    CFStringGetCString(collationOrder, buf, sizeof(buf), kCFStringEncodingASCII);
    return adoptPtr(new Collator(buf));
#else
    return adoptPtr(new Collator(0));
#endif
}

Collator::~Collator()
{
    releaseCollator();
    free(m_locale);
}

void Collator::setOrderLowerFirst(bool lowerFirst)
{
    m_lowerFirst = lowerFirst;
}

Collator::Result Collator::collate(const UChar* lhs, size_t lhsLength, const UChar* rhs, size_t rhsLength) const
{
#ifndef USE_TI_UCOL_REPLACEMENTS
    if (!m_collator)
        createCollator();

    return static_cast<Result>(ucol_strcoll(m_collator, lhs, lhsLength, rhs, rhsLength));
#else
    RetainPtr<CFStringRef> localeStr = CFStringCreateWithCString(NULL, m_locale, kCFStringEncodingASCII);
    RetainPtr<CFLocaleRef> locale = CFLocaleCreate(NULL, localeStr.get());
    
    if (!locale) {
        locale = CFLocaleCopyCurrent();
    }
    
    // CFStringCreateWithCharacters does not accept -1; we have to determine the terminator position ourselves.
    if (lhsLength == (size_t)(-1)) {
        lhsLength = 0;
        while (*(lhs++) != 0x0000) {
            lhsLength++;
        }
    }
    if (rhsLength == (size_t)(-1)) {
        rhsLength = 0;
        while (*(rhs++) != 0x0000) {
            rhsLength++;
        }
    }
    
    
    RetainPtr<CFStringRef> lhsRef = CFStringCreateWithCharacters(NULL, (const UniChar*)lhs, lhsLength);
    RetainPtr<CFStringRef> rhsRef = CFStringCreateWithCharacters(NULL, (const UniChar*)rhs, rhsLength);
    
    return static_cast<Result>(CFStringCompareWithOptionsAndLocale(lhsRef.get(), 
                                                                   rhsRef.get(), 
                                                                   CFRangeMake(0, CFStringGetLength(lhsRef.get())), 
                                                                   kCFCompareLocalized, 
                                                                   locale.get()));
#endif
}

void Collator::createCollator() const
{
#ifndef USE_TI_UCOL_REPLACEMENTS
    ASSERT(!m_collator);
    UErrorCode status = U_ZERO_ERROR;

    {
        Locker<Mutex> lock(cachedCollatorMutex());
        if (cachedCollator) {
            const char* cachedCollatorLocale = ucol_getLocaleByType(cachedCollator, ULOC_REQUESTED_LOCALE, &status);
            ASSERT(U_SUCCESS(status));
            ASSERT(cachedCollatorLocale);

            UColAttributeValue cachedCollatorLowerFirst = ucol_getAttribute(cachedCollator, UCOL_CASE_FIRST, &status);
            ASSERT(U_SUCCESS(status));

            // FIXME: default locale is never matched, because ucol_getLocaleByType returns the actual one used, not 0.
            if (m_locale && 0 == strcmp(cachedCollatorLocale, m_locale)
                && ((UCOL_LOWER_FIRST == cachedCollatorLowerFirst && m_lowerFirst) || (UCOL_UPPER_FIRST == cachedCollatorLowerFirst && !m_lowerFirst))) {
                m_collator = cachedCollator;
                cachedCollator = 0;
                return;
            }
        }
    }

    m_collator = ucol_open(m_locale, &status);
    if (U_FAILURE(status)) {
        status = U_ZERO_ERROR;
        m_collator = ucol_open("", &status); // Fallback to Unicode Collation Algorithm.
    }
    ASSERT(U_SUCCESS(status));

    ucol_setAttribute(m_collator, UCOL_CASE_FIRST, m_lowerFirst ? UCOL_LOWER_FIRST : UCOL_UPPER_FIRST, &status);
    ASSERT(U_SUCCESS(status));
#endif
}

void Collator::releaseCollator()
{
#ifndef USE_TI_UCOL_REPLACEMENTS
    {
        Locker<Mutex> lock(cachedCollatorMutex());
        if (cachedCollator)
            ucol_close(cachedCollator);
        cachedCollator = m_collator;
        m_collator  = 0;
    }
#endif
}

}

#endif
