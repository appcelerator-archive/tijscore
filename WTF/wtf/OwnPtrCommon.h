/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile, Inc.
 * Copyright (C) 2010 Company 100 Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WTF_OwnPtrCommon_h
#define WTF_OwnPtrCommon_h

#if PLATFORM(EFL)
typedef struct _Ecore_Evas Ecore_Evas;
typedef struct _Ecore_IMF_Context Ecore_IMF_Context;
typedef struct _Ecore_Pipe Ecore_Pipe;
typedef struct _Eina_Hash Eina_Hash;
typedef struct _Eina_Module Eina_Module;
#if USE(EO)
typedef struct _Eo_Opaque Evas_Object;
#else
typedef struct _Evas_Object Evas_Object;
#endif
#if USE(ACCELERATED_COMPOSITING)
typedef struct _Evas_GL Evas_GL;
#endif
#endif

namespace WTI {

    template <typename T> inline void deleteOwnedPtr(T* ptr)
    {
        typedef char known[sizeof(T) ? 1 : -1];
        if (sizeof(known))
            delete ptr;
    }

#if PLATFORM(EFL)
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Ecore_Evas*);
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Ecore_IMF_Context*);
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Ecore_Pipe*);
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Eina_Hash*);
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Eina_Module*);
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Evas_Object*);
#if USE(ACCELERATED_COMPOSITING)
    WTF_EXPORT_PRIVATE void deleteOwnedPtr(Evas_GL*);
#endif
#endif

} // namespace WTI

#endif // WTF_OwnPtrCommon_h
