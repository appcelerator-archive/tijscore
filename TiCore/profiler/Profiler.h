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

#ifndef Profiler_h
#define Profiler_h

#include "Profile.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace TI {

    class TiExcState;
    class TiGlobalData;
    class TiGlobalObject;
    class TiObject;
    class TiValue;
    class ProfileGenerator;
    class UString;
    struct CallIdentifier;    

    class Profiler {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static Profiler** enabledProfilerReference()
        {
            return &s_sharedEnabledProfilerReference;
        }

        static Profiler* profiler(); 
        static CallIdentifier createCallIdentifier(TiExcState* exec, TiValue, const UString& sourceURL, int lineNumber);

        void startProfiling(TiExcState*, const UString& title);
        PassRefPtr<Profile> stopProfiling(TiExcState*, const UString& title);
        void stopProfiling(TiGlobalObject*);

        void willExecute(TiExcState* callerCallFrame, TiValue function);
        void willExecute(TiExcState* callerCallFrame, const UString& sourceURL, int startingLineNumber);
        void willExecute(TiExcState* callerCallFrame, const UString& ident);
        void didExecute(TiExcState* callerCallFrame, TiValue function);
        void didExecute(TiExcState* callerCallFrame, const UString& sourceURL, int startingLineNumber);
        void didExecute(TiExcState* callerCallFrame, const UString& ident);

        void exceptionUnwind(TiExcState* handlerCallFrame);

        const Vector<RefPtr<ProfileGenerator> >& currentProfiles() { return m_currentProfiles; };

    private:
        Vector<RefPtr<ProfileGenerator> > m_currentProfiles;
        static Profiler* s_sharedProfiler;
        static Profiler* s_sharedEnabledProfilerReference;
    };

} // namespace TI

#endif // Profiler_h
