/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef DFGJITFinalizer_h
#define DFGJITFinalizer_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGFinalizer.h"
#include "DFGJITCode.h"
#include "LinkBuffer.h"
#include "MacroAssembler.h"

namespace TI { namespace DFG {

class JITFinalizer : public Finalizer {
public:
    JITFinalizer(Plan&, PassRefPtr<JITCode>, PassOwnPtr<LinkBuffer>, MacroAssemblerCodePtr withArityCheck = MacroAssemblerCodePtr(MacroAssemblerCodePtr::EmptyValue));
    virtual ~JITFinalizer();
    
    virtual bool finalize() OVERRIDE;
    virtual bool finalizeFunction() OVERRIDE;

private:
    void finalizeCommon();
    
    RefPtr<JITCode> m_jitCode;
    OwnPtr<LinkBuffer> m_linkBuffer;
    MacroAssemblerCodePtr m_withArityCheck;
};

} } // namespace TI::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGJITFinalizer_h

