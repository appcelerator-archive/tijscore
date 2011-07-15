/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef JSZombie_h
#define JSZombie_h

#include "TiCell.h"

#if ENABLE(JSC_ZOMBIES)
namespace TI {

class JSZombie : public TiCell {
public:
    JSZombie(const ClassInfo* oldInfo, Structure* structure)
        : TiCell(structure)
        , m_oldInfo(oldInfo)
    {
    }
    virtual bool isZombie() const { return true; }
    virtual const ClassInfo* classInfo() const { return &s_info; }
    static Structure* leakedZombieStructure();

    virtual bool isGetterSetter() const { ASSERT_NOT_REACHED(); return false; }
    virtual bool isAPIValueWrapper() const { ASSERT_NOT_REACHED(); return false; }
    virtual bool isPropertyNameIterator() const { ASSERT_NOT_REACHED(); return false; }
    virtual CallType getCallData(CallData&) { ASSERT_NOT_REACHED(); return CallTypeNone; }
    virtual ConstructType getConstructData(ConstructData&) { ASSERT_NOT_REACHED(); return ConstructTypeNone; }
    virtual bool getUInt32(uint32_t&) const { ASSERT_NOT_REACHED(); return false; }
    virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType) const { ASSERT_NOT_REACHED(); return jsNull(); }
    virtual bool getPrimitiveNumber(TiExcState*, double&, TiValue&) { ASSERT_NOT_REACHED(); return false; }
    virtual bool toBoolean(TiExcState*) const { ASSERT_NOT_REACHED(); return false; }
    virtual double toNumber(TiExcState*) const { ASSERT_NOT_REACHED(); return 0.0; }
    virtual UString toString(TiExcState*) const { ASSERT_NOT_REACHED(); return ""; }
    virtual TiObject* toObject(TiExcState*) const { ASSERT_NOT_REACHED(); return 0; }
    virtual void markChildren(MarkStack&) { ASSERT_NOT_REACHED(); }
    virtual void put(TiExcState*, const Identifier&, TiValue, PutPropertySlot&) { ASSERT_NOT_REACHED(); }
    virtual void put(TiExcState*, unsigned, TiValue) { ASSERT_NOT_REACHED(); }
    virtual bool deleteProperty(TiExcState*, const Identifier&) { ASSERT_NOT_REACHED(); return false; }
    virtual bool deleteProperty(TiExcState*, unsigned) { ASSERT_NOT_REACHED(); return false; }
    virtual TiObject* toThisObject(TiExcState*) const { ASSERT_NOT_REACHED(); return 0; }
    virtual TiValue getJSNumber() { ASSERT_NOT_REACHED(); return jsNull(); }
    virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&) { ASSERT_NOT_REACHED(); return false; }
    virtual bool getOwnPropertySlot(TiExcState*, unsigned, PropertySlot&) { ASSERT_NOT_REACHED(); return false; }
    
    static const ClassInfo s_info;
private:
    const ClassInfo* m_oldInfo;
};

}

#endif // ENABLE(JSC_ZOMBIES)

#endif // JSZombie_h
