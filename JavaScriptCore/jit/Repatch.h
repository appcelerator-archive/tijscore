/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef Repatch_h
#define Repatch_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "CCallHelpers.h"
#include "JITOperations.h"

namespace TI {

void repatchGetByID(ExecState*, TiValue, const Identifier&, const PropertySlot&, StructureStubInfo&);
void buildGetByIDList(ExecState*, TiValue, const Identifier&, const PropertySlot&, StructureStubInfo&);
void buildGetByIDProtoList(ExecState*, TiValue, const Identifier&, const PropertySlot&, StructureStubInfo&);
void repatchPutByID(ExecState*, TiValue, const Identifier&, const PutPropertySlot&, StructureStubInfo&, PutKind);
void buildPutByIdList(ExecState*, TiValue, const Identifier&, const PutPropertySlot&, StructureStubInfo&, PutKind);
void repatchIn(ExecState*, JSCell*, const Identifier&, bool wasFound, const PropertySlot&, StructureStubInfo&);
void linkFor(ExecState*, CallLinkInfo&, CodeBlock*, JSFunction* callee, MacroAssemblerCodePtr, CodeSpecializationKind);
void linkSlowFor(ExecState*, CallLinkInfo&, CodeSpecializationKind);
void linkClosureCall(ExecState*, CallLinkInfo&, CodeBlock*, Structure*, ExecutableBase*, MacroAssemblerCodePtr);
void resetGetByID(RepatchBuffer&, StructureStubInfo&);
void resetPutByID(RepatchBuffer&, StructureStubInfo&);
void resetIn(RepatchBuffer&, StructureStubInfo&);

} // namespace TI

#else // ENABLE(JIT)

#include <wtf/Assertions.h>

namespace TI {

class RepatchBuffer;
struct StructureStubInfo;

inline NO_RETURN_DUE_TO_CRASH void resetGetByID(RepatchBuffer&, StructureStubInfo&) { RELEASE_ASSERT_NOT_REACHED(); }
inline NO_RETURN_DUE_TO_CRASH void resetPutByID(RepatchBuffer&, StructureStubInfo&) { RELEASE_ASSERT_NOT_REACHED(); }
inline NO_RETURN void resetIn(RepatchBuffer&, StructureStubInfo&) { RELEASE_ASSERT_NOT_REACHED(); }

} // namespace TI

#endif // ENABLE(JIT)
#endif // Repatch_h
