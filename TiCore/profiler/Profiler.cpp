/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "Profiler.h"

#include "CommonIdentifiers.h"
#include "CallFrame.h"
#include "CodeBlock.h"
#include "TiFunction.h"
#include "TiGlobalObject.h"
#include "Nodes.h"
#include "Profile.h"
#include "ProfileGenerator.h"
#include "ProfileNode.h"
#include <stdio.h>

namespace TI {

static const char* GlobalCodeExecution = "(program)";
static const char* AnonymousFunction = "(anonymous function)";
static unsigned ProfilesUID = 0;

static CallIdentifier createCallIdentifierFromFunctionImp(TiExcState*, TiFunction*);

Profiler* Profiler::s_sharedProfiler = 0;
Profiler* Profiler::s_sharedEnabledProfilerReference = 0;

Profiler* Profiler::profiler()
{
    if (!s_sharedProfiler)
        s_sharedProfiler = new Profiler();
    return s_sharedProfiler;
}   

void Profiler::startProfiling(TiExcState* exec, const UString& title)
{
    ASSERT_ARG(title, !title.isNull());

    // Check if we currently have a Profile for this global TiExcState and title.
    // If so return early and don't create a new Profile.
    TiExcState* globalExec = exec ? exec->lexicalGlobalObject()->globalExec() : 0;

    for (size_t i = 0; i < m_currentProfiles.size(); ++i) {
        ProfileGenerator* profileGenerator = m_currentProfiles[i].get();
        if (profileGenerator->originatingGlobalExec() == globalExec && profileGenerator->title() == title)
            return;
    }

    s_sharedEnabledProfilerReference = this;
    RefPtr<ProfileGenerator> profileGenerator = ProfileGenerator::create(title, exec, ++ProfilesUID);
    m_currentProfiles.append(profileGenerator);
}

PassRefPtr<Profile> Profiler::stopProfiling(TiExcState* exec, const UString& title)
{
    TiExcState* globalExec = exec ? exec->lexicalGlobalObject()->globalExec() : 0;
    for (ptrdiff_t i = m_currentProfiles.size() - 1; i >= 0; --i) {
        ProfileGenerator* profileGenerator = m_currentProfiles[i].get();
        if (profileGenerator->originatingGlobalExec() == globalExec && (title.isNull() || profileGenerator->title() == title)) {
            profileGenerator->stopProfiling();
            RefPtr<Profile> returnProfile = profileGenerator->profile();

            m_currentProfiles.remove(i);
            if (!m_currentProfiles.size())
                s_sharedEnabledProfilerReference = 0;
            
            return returnProfile;
        }
    }

    return 0;
}

static inline void dispatchFunctionToProfiles(const Vector<RefPtr<ProfileGenerator> >& profiles, ProfileGenerator::ProfileFunction function, const CallIdentifier& callIdentifier, unsigned currentProfileTargetGroup)
{
    for (size_t i = 0; i < profiles.size(); ++i) {
        if (profiles[i]->profileGroup() == currentProfileTargetGroup || !profiles[i]->originatingGlobalExec())
            (profiles[i].get()->*function)(callIdentifier);
    }
}

void Profiler::willExecute(TiExcState* exec, TiValue function)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(m_currentProfiles, &ProfileGenerator::willExecute, createCallIdentifier(exec, function, "", 0), exec->lexicalGlobalObject()->profileGroup());
}

void Profiler::willExecute(TiExcState* exec, const UString& sourceURL, int startingLineNumber)
{
    ASSERT(!m_currentProfiles.isEmpty());

    CallIdentifier callIdentifier = createCallIdentifier(exec, TiValue(), sourceURL, startingLineNumber);

    dispatchFunctionToProfiles(m_currentProfiles, &ProfileGenerator::willExecute, callIdentifier, exec->lexicalGlobalObject()->profileGroup());
}

void Profiler::didExecute(TiExcState* exec, TiValue function)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(m_currentProfiles, &ProfileGenerator::didExecute, createCallIdentifier(exec, function, "", 0), exec->lexicalGlobalObject()->profileGroup());
}

void Profiler::didExecute(TiExcState* exec, const UString& sourceURL, int startingLineNumber)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(m_currentProfiles, &ProfileGenerator::didExecute, createCallIdentifier(exec, TiValue(), sourceURL, startingLineNumber), exec->lexicalGlobalObject()->profileGroup());
}

CallIdentifier Profiler::createCallIdentifier(TiExcState* exec, TiValue functionValue, const UString& defaultSourceURL, int defaultLineNumber)
{
    if (!functionValue)
        return CallIdentifier(GlobalCodeExecution, defaultSourceURL, defaultLineNumber);
    if (!functionValue.isObject())
        return CallIdentifier("(unknown)", defaultSourceURL, defaultLineNumber);
    if (asObject(functionValue)->inherits(&TiFunction::info)) {
        TiFunction* function = asFunction(functionValue);
        if (!function->executable()->isHostFunction())
            return createCallIdentifierFromFunctionImp(exec, function);
    }
    if (asObject(functionValue)->inherits(&InternalFunction::info))
        return CallIdentifier(static_cast<InternalFunction*>(asObject(functionValue))->name(exec), defaultSourceURL, defaultLineNumber);
    return CallIdentifier(makeString("(", asObject(functionValue)->className(), " object)"), defaultSourceURL, defaultLineNumber);
}

CallIdentifier createCallIdentifierFromFunctionImp(TiExcState* exec, TiFunction* function)
{
    ASSERT(!function->isHostFunction());
    const UString& name = function->calculatedDisplayName(exec);
    return CallIdentifier(name.isEmpty() ? AnonymousFunction : name, function->jsExecutable()->sourceURL(), function->jsExecutable()->lineNo());
}

} // namespace TI
