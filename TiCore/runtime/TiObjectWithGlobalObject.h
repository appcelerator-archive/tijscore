/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TiObjectWithGlobalObject_h
#define TiObjectWithGlobalObject_h

#include "TiGlobalObject.h"

namespace TI {

class TiGlobalObject;

class TiObjectWithGlobalObject : public JSNonFinalObject {
public:
    static Structure* createStructure(TiGlobalData& globalData, TiValue proto)
    {
        return Structure::create(globalData, proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }

    TiGlobalObject* globalObject() const
    {
        return asGlobalObject((getAnonymousValue(GlobalObjectSlot).asCell()));
    }

protected:
    TiObjectWithGlobalObject(TiGlobalObject*, Structure*);
    TiObjectWithGlobalObject(TiGlobalData&, TiGlobalObject*, Structure*);

    TiObjectWithGlobalObject(VPtrStealingHackType)
        : JSNonFinalObject(VPtrStealingHack)
    {
        // Should only be used by TiFunction when we aquire the TiFunction vptr.
    }
    static const unsigned AnonymousSlotCount = TiObject::AnonymousSlotCount + 1;
    static const unsigned GlobalObjectSlot = 0;
};

} // namespace TI

#endif // TiObjectWithGlobalObject_h
