/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSNotAnObject_h
#define JSNotAnObject_h

#include "TiObject.h"

namespace TI {

    // This unholy class is used to allow us to avoid multiple exception checks
    // in certain SquirrelFish bytecodes -- effectively it just silently consumes
    // any operations performed on the result of a failed toObject call.
    class JSNotAnObject : public JSNonFinalObject {
    public:
        JSNotAnObject(TiExcState* exec)
            : JSNonFinalObject(exec->globalData(), exec->globalData().notAnObjectStructure.get())
        {
        }

        static Structure* createStructure(TiGlobalData& globalData, TiValue prototype)
        {
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
        }

     private:
        
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesGetPropertyNames | TiObject::StructureFlags;

        // TiValue methods
        virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(TiExcState*, double& number, TiValue&);
        virtual bool toBoolean(TiExcState*) const;
        virtual double toNumber(TiExcState*) const;
        virtual UString toString(TiExcState*) const;
        virtual TiObject* toObject(TiExcState*, TiGlobalObject*) const;

        // TiObject methods
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);

        virtual void put(TiExcState*, const Identifier& propertyName, TiValue, PutPropertySlot&);
        virtual void put(TiExcState*, unsigned propertyName, TiValue);

        virtual bool deleteProperty(TiExcState*, const Identifier& propertyName);
        virtual bool deleteProperty(TiExcState*, unsigned propertyName);

        virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
    };

} // namespace TI

#endif // JSNotAnObject_h
