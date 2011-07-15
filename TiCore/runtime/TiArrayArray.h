/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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

#ifndef TiArrayArray_h
#define TiArrayArray_h

#include "TiObject.h"

#include <wtf/ByteArray.h>

namespace TI {

    class TiArrayArray : public TiObject {
        friend class TiGlobalData;
    public:
        bool canAccessIndex(unsigned i) { return i < m_storage->length(); }
        TiValue getIndex(TiExcState* exec, unsigned i)
        {
            ASSERT(canAccessIndex(i));
            return jsNumber(exec, m_storage->data()[i]);
        }

        void setIndex(unsigned i, int value)
        {
            ASSERT(canAccessIndex(i));
            if (value & ~0xFF) {
                if (value < 0)
                    value = 0;
                else
                    value = 255;
            }
            m_storage->data()[i] = static_cast<unsigned char>(value);
        }
        
        void setIndex(unsigned i, double value)
        {
            ASSERT(canAccessIndex(i));
            if (!(value > 0)) // Clamp NaN to 0
                value = 0;
            else if (value > 255)
                value = 255;
            m_storage->data()[i] = static_cast<unsigned char>(value + 0.5);
        }
        
        void setIndex(TiExcState* exec, unsigned i, TiValue value)
        {
            double byteValue = value.toNumber(exec);
            if (exec->hadException())
                return;
            if (canAccessIndex(i))
                setIndex(i, byteValue);
        }

        TiArrayArray(TiExcState* exec, NonNullPassRefPtr<Structure>, WTI::ByteArray* storage, const TI::ClassInfo* = &s_defaultInfo);
        static PassRefPtr<Structure> createStructure(TiValue prototype);
        
        virtual bool getOwnPropertySlot(TI::TiExcState*, const TI::Identifier& propertyName, TI::PropertySlot&);
        virtual bool getOwnPropertySlot(TI::TiExcState*, unsigned propertyName, TI::PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void put(TI::TiExcState*, const TI::Identifier& propertyName, TI::TiValue, TI::PutPropertySlot&);
        virtual void put(TI::TiExcState*, unsigned propertyName, TI::TiValue);

        virtual void getOwnPropertyNames(TI::TiExcState*, TI::PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

        virtual const ClassInfo* classInfo() const { return m_classInfo; }
        static const ClassInfo s_defaultInfo;
        
        size_t length() const { return m_storage->length(); }

        WTI::ByteArray* storage() const { return m_storage.get(); }

#if !ASSERT_DISABLED
        virtual ~TiArrayArray();
#endif

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesGetPropertyNames | TiObject::StructureFlags;

    private:
        enum VPtrStealingHackType { VPtrStealingHack };
        TiArrayArray(VPtrStealingHackType) 
            : TiObject(createStructure(jsNull()))
            , m_classInfo(0)
        {
        }

        RefPtr<WTI::ByteArray> m_storage;
        const ClassInfo* m_classInfo;
    };
    
    TiArrayArray* asByteArray(TiValue value);
    inline TiArrayArray* asByteArray(TiValue value)
    {
        return static_cast<TiArrayArray*>(asCell(value));
    }

    inline bool isTiArrayArray(TiGlobalData* globalData, TiValue v) { return v.isCell() && v.asCell()->vptr() == globalData->jsByteArrayVPtr; }

} // namespace TI

#endif // TiArrayArray_h
