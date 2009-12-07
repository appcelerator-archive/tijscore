/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2004, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */


#ifndef Protect_h
#define Protect_h

#include "Collector.h"
#include "TiValue.h"

namespace TI {

    inline void gcProtect(TiCell* val) 
    {
        Heap::heap(val)->protect(val);
    }

    inline void gcUnprotect(TiCell* val)
    {
        Heap::heap(val)->unprotect(val);
    }

    inline void gcProtectNullTolerant(TiCell* val) 
    {
        if (val) 
            gcProtect(val);
    }

    inline void gcUnprotectNullTolerant(TiCell* val) 
    {
        if (val) 
            gcUnprotect(val);
    }
    
    inline void gcProtect(TiValue value)
    {
        if (value && value.isCell())
            gcProtect(asCell(value));
    }

    inline void gcUnprotect(TiValue value)
    {
        if (value && value.isCell())
            gcUnprotect(asCell(value));
    }

    // FIXME: Share more code with RefPtr template? The only differences are the ref/deref operation
    // and the implicit conversion to raw pointer
    template <class T> class ProtectedPtr {
    public:
        ProtectedPtr() : m_ptr(0) {}
        ProtectedPtr(T* ptr);
        ProtectedPtr(const ProtectedPtr&);
        ~ProtectedPtr();

        template <class U> ProtectedPtr(const ProtectedPtr<U>&);
        
        T* get() const { return m_ptr; }
        operator T*() const { return m_ptr; }
        operator TiValue() const { return TiValue(m_ptr); }
        T* operator->() const { return m_ptr; }
        
        operator bool() const { return m_ptr; }
        bool operator!() const { return !m_ptr; }

        ProtectedPtr& operator=(const ProtectedPtr&);
        ProtectedPtr& operator=(T*);
        
    private:
        T* m_ptr;
    };

    class ProtectedTiValue {
    public:
        ProtectedTiValue() {}
        ProtectedTiValue(TiValue value);
        ProtectedTiValue(const ProtectedTiValue&);
        ~ProtectedTiValue();

        template <class U> ProtectedTiValue(const ProtectedPtr<U>&);
        
        TiValue get() const { return m_value; }
        operator TiValue() const { return m_value; }
        TiValue operator->() const { return m_value; }
        
        operator bool() const { return m_value; }
        bool operator!() const { return !m_value; }

        ProtectedTiValue& operator=(const ProtectedTiValue&);
        ProtectedTiValue& operator=(TiValue);
        
    private:
        TiValue m_value;
    };

    template <class T> inline ProtectedPtr<T>::ProtectedPtr(T* ptr)
        : m_ptr(ptr)
    {
        gcProtectNullTolerant(m_ptr);
    }

    template <class T> inline ProtectedPtr<T>::ProtectedPtr(const ProtectedPtr& o)
        : m_ptr(o.get())
    {
        gcProtectNullTolerant(m_ptr);
    }

    template <class T> inline ProtectedPtr<T>::~ProtectedPtr()
    {
        gcUnprotectNullTolerant(m_ptr);
    }

    template <class T> template <class U> inline ProtectedPtr<T>::ProtectedPtr(const ProtectedPtr<U>& o)
        : m_ptr(o.get())
    {
        gcProtectNullTolerant(m_ptr);
    }

    template <class T> inline ProtectedPtr<T>& ProtectedPtr<T>::operator=(const ProtectedPtr<T>& o) 
    {
        T* optr = o.m_ptr;
        gcProtectNullTolerant(optr);
        gcUnprotectNullTolerant(m_ptr);
        m_ptr = optr;
        return *this;
    }

    template <class T> inline ProtectedPtr<T>& ProtectedPtr<T>::operator=(T* optr)
    {
        gcProtectNullTolerant(optr);
        gcUnprotectNullTolerant(m_ptr);
        m_ptr = optr;
        return *this;
    }

    inline ProtectedTiValue::ProtectedTiValue(TiValue value)
        : m_value(value)
    {
        gcProtect(m_value);
    }

    inline ProtectedTiValue::ProtectedTiValue(const ProtectedTiValue& o)
        : m_value(o.get())
    {
        gcProtect(m_value);
    }

    inline ProtectedTiValue::~ProtectedTiValue()
    {
        gcUnprotect(m_value);
    }

    template <class U> ProtectedTiValue::ProtectedTiValue(const ProtectedPtr<U>& o)
        : m_value(o.get())
    {
        gcProtect(m_value);
    }

    inline ProtectedTiValue& ProtectedTiValue::operator=(const ProtectedTiValue& o) 
    {
        TiValue ovalue = o.m_value;
        gcProtect(ovalue);
        gcUnprotect(m_value);
        m_value = ovalue;
        return *this;
    }

    inline ProtectedTiValue& ProtectedTiValue::operator=(TiValue ovalue)
    {
        gcProtect(ovalue);
        gcUnprotect(m_value);
        m_value = ovalue;
        return *this;
    }

    template <class T> inline bool operator==(const ProtectedPtr<T>& a, const ProtectedPtr<T>& b) { return a.get() == b.get(); }
    template <class T> inline bool operator==(const ProtectedPtr<T>& a, const T* b) { return a.get() == b; }
    template <class T> inline bool operator==(const T* a, const ProtectedPtr<T>& b) { return a == b.get(); }

    template <class T> inline bool operator!=(const ProtectedPtr<T>& a, const ProtectedPtr<T>& b) { return a.get() != b.get(); }
    template <class T> inline bool operator!=(const ProtectedPtr<T>& a, const T* b) { return a.get() != b; }
    template <class T> inline bool operator!=(const T* a, const ProtectedPtr<T>& b) { return a != b.get(); }

    inline bool operator==(const ProtectedTiValue& a, const ProtectedTiValue& b) { return a.get() == b.get(); }
    inline bool operator==(const ProtectedTiValue& a, const TiValue b) { return a.get() == b; }
    template <class T> inline bool operator==(const ProtectedTiValue& a, const ProtectedPtr<T>& b) { return a.get() == TiValue(b.get()); }
    inline bool operator==(const TiValue a, const ProtectedTiValue& b) { return a == b.get(); }
    template <class T> inline bool operator==(const ProtectedPtr<T>& a, const ProtectedTiValue& b) { return TiValue(a.get()) == b.get(); }

    inline bool operator!=(const ProtectedTiValue& a, const ProtectedTiValue& b) { return a.get() != b.get(); }
    inline bool operator!=(const ProtectedTiValue& a, const TiValue b) { return a.get() != b; }
    template <class T> inline bool operator!=(const ProtectedTiValue& a, const ProtectedPtr<T>& b) { return a.get() != TiValue(b.get()); }
    inline bool operator!=(const TiValue a, const ProtectedTiValue& b) { return a != b.get(); }
    template <class T> inline bool operator!=(const ProtectedPtr<T>& a, const ProtectedTiValue& b) { return TiValue(a.get()) != b.get(); }
 
} // namespace TI

#endif // Protect_h
