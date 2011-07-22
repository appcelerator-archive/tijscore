/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#ifndef TiStaticScopeObject_h
#define TiStaticScopeObject_h

#include "JSVariableObject.h"

namespace TI{
    
    class TiStaticScopeObject : public JSVariableObject {
    protected:
        using JSVariableObject::JSVariableObjectData;
        struct TiStaticScopeObjectData : public JSVariableObjectData {
            TiStaticScopeObjectData()
                : JSVariableObjectData(&symbolTable, &registerStore + 1)
            {
            }
            SymbolTable symbolTable;
            Register registerStore;
        };
        
    public:
        TiStaticScopeObject(TiExcState* exec, const Identifier& ident, TiValue value, unsigned attributes)
            : JSVariableObject(exec->globalData().staticScopeStructure, new TiStaticScopeObjectData())
        {
            d()->registerStore = value;
            symbolTable().add(ident.ustring().rep(), SymbolTableEntry(-1, attributes));
        }
        virtual ~TiStaticScopeObject();
        virtual void markChildren(MarkStack&);
        bool isDynamicScope(bool& requiresDynamicChecks) const;
        virtual TiObject* toThisObject(TiExcState*) const;
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&);
        virtual void put(TiExcState*, const Identifier&, TiValue, PutPropertySlot&);
        void putWithAttributes(TiExcState*, const Identifier&, TiValue, unsigned attributes);

        static PassRefPtr<Structure> createStructure(TiValue proto) { return Structure::create(proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount); }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | NeedsThisConversion | OverridesMarkChildren | OverridesGetPropertyNames | JSVariableObject::StructureFlags;

    private:
        TiStaticScopeObjectData* d() { return static_cast<TiStaticScopeObjectData*>(JSVariableObject::d); }
    };

}

#endif // TiStaticScopeObject_h
