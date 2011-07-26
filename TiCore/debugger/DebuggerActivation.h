/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef DebuggerActivation_h
#define DebuggerActivation_h

#include "TiObject.h"

namespace TI {

    class JSActivation;

    class DebuggerActivation : public TiObject {
    public:
        DebuggerActivation(TiObject*);

        virtual void markChildren(MarkStack&);
        virtual UString className() const;
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual void putWithAttributes(TiExcState*, const Identifier& propertyName, TiValue, unsigned attributes);
        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
        virtual void defineGetter(TiExcState*, const Identifier& propertyName, TiObject* getterFunction, unsigned attributes);
        virtual void defineSetter(TiExcState*, const Identifier& propertyName, TiObject* setterFunction, unsigned attributes);
        virtual TiValue lookupGetter(TiExcState*, const Identifier& propertyName);
        virtual TiValue lookupSetter(TiExcState*, const Identifier& propertyName);

        static PassRefPtr<Structure> createStructure(TiValue prototype) 
        {
            return Structure::create(prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount); 
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesMarkChildren | TiObject::StructureFlags;

    private:
        JSActivation* m_activation;
    };

} // namespace TI

#endif // DebuggerActivation_h
