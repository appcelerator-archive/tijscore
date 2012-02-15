/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef TiCallbackObject_h
#define TiCallbackObject_h

#include "TiObjectRef.h"
#include "TiValueRef.h"
#include "TiObject.h"
#include <wtf/PassOwnPtr.h>

namespace TI {

struct TiCallbackObjectData : WeakHandleOwner {
    TiCallbackObjectData(void* privateData, TiClassRef jsClass)
        : privateData(privateData)
        , jsClass(jsClass)
    {
        TiClassRetain(jsClass);
    }
    
    ~TiCallbackObjectData()
    {
        TiClassRelease(jsClass);
    }
    
    TiValue getPrivateProperty(const Identifier& propertyName) const
    {
        if (!m_privateProperties)
            return TiValue();
        return m_privateProperties->getPrivateProperty(propertyName);
    }
    
    void setPrivateProperty(TiGlobalData& globalData, TiCell* owner, const Identifier& propertyName, TiValue value)
    {
        if (!m_privateProperties)
            m_privateProperties = adoptPtr(new JSPrivatePropertyMap);
        m_privateProperties->setPrivateProperty(globalData, owner, propertyName, value);
    }
    
    void deletePrivateProperty(const Identifier& propertyName)
    {
        if (!m_privateProperties)
            return;
        m_privateProperties->deletePrivateProperty(propertyName);
    }

    void visitChildren(SlotVisitor& visitor)
    {
        if (!m_privateProperties)
            return;
        m_privateProperties->visitChildren(visitor);
    }

    void* privateData;
    TiClassRef jsClass;
    struct JSPrivatePropertyMap {
        TiValue getPrivateProperty(const Identifier& propertyName) const
        {
            PrivatePropertyMap::const_iterator location = m_propertyMap.find(propertyName.impl());
            if (location == m_propertyMap.end())
                return TiValue();
            return location->second.get();
        }
        
        void setPrivateProperty(TiGlobalData& globalData, TiCell* owner, const Identifier& propertyName, TiValue value)
        {
            WriteBarrier<Unknown> empty;
            m_propertyMap.add(propertyName.impl(), empty).first->second.set(globalData, owner, value);
        }
        
        void deletePrivateProperty(const Identifier& propertyName)
        {
            m_propertyMap.remove(propertyName.impl());
        }

        void visitChildren(SlotVisitor& visitor)
        {
            for (PrivatePropertyMap::iterator ptr = m_propertyMap.begin(); ptr != m_propertyMap.end(); ++ptr) {
                if (ptr->second)
                    visitor.append(&ptr->second);
            }
        }

    private:
        typedef HashMap<RefPtr<StringImpl>, WriteBarrier<Unknown>, IdentifierRepHash> PrivatePropertyMap;
        PrivatePropertyMap m_propertyMap;
    };
    OwnPtr<JSPrivatePropertyMap> m_privateProperties;
    virtual void finalize(Handle<Unknown>, void*);
};

    
template <class Base>
class TiCallbackObject : public Base {
public:
    TiCallbackObject(TiExcState*, TiGlobalObject*, Structure*, TiClassRef, void* data);
    TiCallbackObject(TiGlobalData&, TiClassRef, Structure*);

    void setPrivate(void* data);
    void* getPrivate();

    static const ClassInfo s_info;

    TiClassRef classRef() const { return m_callbackObjectData->jsClass; }
    bool inherits(TiClassRef) const;

    static Structure* createStructure(TiGlobalData& globalData, TiValue proto) 
    { 
        return Structure::create(globalData, proto, TypeInfo(ObjectType, StructureFlags), Base::AnonymousSlotCount, &s_info); 
    }
    
    TiValue getPrivateProperty(const Identifier& propertyName) const
    {
        return m_callbackObjectData->getPrivateProperty(propertyName);
    }
    
    void setPrivateProperty(TiGlobalData& globalData, const Identifier& propertyName, TiValue value)
    {
        m_callbackObjectData->setPrivateProperty(globalData, this, propertyName, value);
    }
    
    void deletePrivateProperty(const Identifier& propertyName)
    {
        m_callbackObjectData->deletePrivateProperty(propertyName);
    }

protected:
    static const unsigned StructureFlags = ProhibitsPropertyCaching | OverridesGetOwnPropertySlot | ImplementsHasInstance | OverridesHasInstance | OverridesVisitChildren | OverridesGetPropertyNames | Base::StructureFlags;

private:
    virtual UString className() const;

    virtual bool getOwnPropertySlot(TiExcState*, const Identifier&, PropertySlot&);
    virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);
    
    virtual void put(TiExcState*, const Identifier&, TiValue, PutPropertySlot&);

    virtual bool deleteProperty(TiExcState*, const Identifier&);
    virtual bool deleteProperty(TiExcState*, unsigned);

    virtual bool hasInstance(TiExcState* exec, TiValue value, TiValue proto);

    virtual void getOwnPropertyNames(TiExcState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

    virtual double toNumber(TiExcState*) const;
    virtual UString toString(TiExcState*) const;

    virtual ConstructType getConstructData(ConstructData&);
    virtual CallType getCallData(CallData&);

    virtual void visitChildren(SlotVisitor& visitor)
    {
        Base::visitChildren(visitor);
        m_callbackObjectData->visitChildren(visitor);
    }

    void init(TiExcState*);
 
    static TiCallbackObject* asCallbackObject(TiValue);
 
    static EncodedTiValue JSC_HOST_CALL call(TiExcState*);
    static EncodedTiValue JSC_HOST_CALL construct(TiExcState*);
   
    TiValue getStaticValue(TiExcState*, const Identifier&);
    static TiValue staticFunctionGetter(TiExcState*, TiValue, const Identifier&);
    static TiValue callbackGetter(TiExcState*, TiValue, const Identifier&);

    OwnPtr<TiCallbackObjectData> m_callbackObjectData;
};

} // namespace TI

// include the actual template class implementation
#include "TiCallbackObjectFunctions.h"

#endif // TiCallbackObject_h
