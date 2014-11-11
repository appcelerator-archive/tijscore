/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (c) 2008, 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScriptValue_h
#define ScriptValue_h

#include "JSCTiValue.h"
#include "Operations.h"
#include "Strong.h"
#include "StrongInlines.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace Inspector {
class InspectorValue;
}

namespace Deprecated {

class JS_EXPORT_PRIVATE ScriptValue {
public:
    ScriptValue() { }
    ScriptValue(TI::VM& vm, TI::TiValue value) : m_value(vm, value) { }
    virtual ~ScriptValue();

    TI::TiValue jsValue() const { return m_value.get(); }
    bool getString(TI::ExecState*, String& result) const;
    String toString(TI::ExecState*) const;
    bool isEqual(TI::ExecState*, const ScriptValue&) const;
    bool isNull() const;
    bool isUndefined() const;
    bool isObject() const;
    bool isFunction() const;
    bool hasNoValue() const { return !m_value; }

    void clear() { m_value.clear(); }

    bool operator==(const ScriptValue& other) const { return m_value == other.m_value; }

#if ENABLE(INSPECTOR)
    PassRefPtr<Inspector::InspectorValue> toInspectorValue(TI::ExecState*) const;
#endif

private:
    TI::Strong<TI::Unknown> m_value;
};

} // namespace Deprecated

#endif // ScriptValue_h
