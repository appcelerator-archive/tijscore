/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
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

#ifndef PropertyDescriptor_h
#define PropertyDescriptor_h

#include "JSCTiValue.h"

namespace TI {
    class GetterSetter;

    // See ES5.1 9.12
    bool sameValue(ExecState*, TiValue, TiValue);

    class PropertyDescriptor {
    public:
        PropertyDescriptor()
            : m_attributes(defaultAttributes)
            , m_seenAttributes(0)
        {
        }
        PropertyDescriptor(TiValue value, unsigned attributes)
            : m_value(value)
            , m_attributes(attributes)
            , m_seenAttributes(EnumerablePresent | ConfigurablePresent | WritablePresent)
        {
            ASSERT(m_value);
            ASSERT(!m_value.isGetterSetter());
        }
        JS_EXPORT_PRIVATE bool writable() const;
        JS_EXPORT_PRIVATE bool enumerable() const;
        JS_EXPORT_PRIVATE bool configurable() const;
        JS_EXPORT_PRIVATE bool isDataDescriptor() const;
        bool isGenericDescriptor() const;
        JS_EXPORT_PRIVATE bool isAccessorDescriptor() const;
        unsigned attributes() const { return m_attributes; }
        TiValue value() const { return m_value; }
        JS_EXPORT_PRIVATE TiValue getter() const;
        JS_EXPORT_PRIVATE TiValue setter() const;
        JSObject* getterObject() const;
        JSObject* setterObject() const;
        JS_EXPORT_PRIVATE void setUndefined();
        JS_EXPORT_PRIVATE void setDescriptor(TiValue value, unsigned attributes);
        JS_EXPORT_PRIVATE void setAccessorDescriptor(GetterSetter* accessor, unsigned attributes);
        JS_EXPORT_PRIVATE void setWritable(bool);
        JS_EXPORT_PRIVATE void setEnumerable(bool);
        JS_EXPORT_PRIVATE void setConfigurable(bool);
        void setValue(TiValue value) { m_value = value; }
        JS_EXPORT_PRIVATE void setSetter(TiValue);
        JS_EXPORT_PRIVATE void setGetter(TiValue);
        bool isEmpty() const { return !(m_value || m_getter || m_setter || m_seenAttributes); }
        bool writablePresent() const { return m_seenAttributes & WritablePresent; }
        bool enumerablePresent() const { return m_seenAttributes & EnumerablePresent; }
        bool configurablePresent() const { return m_seenAttributes & ConfigurablePresent; }
        bool setterPresent() const { return m_setter; }
        bool getterPresent() const { return m_getter; }
        bool equalTo(ExecState* exec, const PropertyDescriptor& other) const;
        bool attributesEqual(const PropertyDescriptor& other) const;
        unsigned attributesOverridingCurrent(const PropertyDescriptor& current) const;

    private:
        JS_EXPORTDATA static unsigned defaultAttributes;
        bool operator==(const PropertyDescriptor&){ return false; }
        enum { WritablePresent = 1, EnumerablePresent = 2, ConfigurablePresent = 4};
        // May be a getter/setter
        TiValue m_value;
        TiValue m_getter;
        TiValue m_setter;
        unsigned m_attributes;
        unsigned m_seenAttributes;
    };
}

#endif
