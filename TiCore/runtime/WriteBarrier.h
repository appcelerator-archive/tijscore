/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WriteBarrier_h
#define WriteBarrier_h

#include "HandleTypes.h"
#include "Heap.h"
#include "TypeTraits.h"

namespace TI {

class TiCell;
class TiGlobalData;
class TiGlobalObject;

template<class T> class WriteBarrierBase;
template<> class WriteBarrierBase<TiValue>;

void slowValidateCell(TiCell*);
void slowValidateCell(TiGlobalObject*);
    
#if ENABLE(GC_VALIDATION)
template<class T> inline void validateCell(T cell)
{
    ASSERT_GC_OBJECT_INHERITS(cell, &WTI::RemovePointer<T>::Type::s_info);
}

template<> inline void validateCell<TiCell*>(TiCell* cell)
{
    slowValidateCell(cell);
}

template<> inline void validateCell<TiGlobalObject*>(TiGlobalObject* globalObject)
{
    slowValidateCell(globalObject);
}
#else
template<class T> inline void validateCell(T)
{
}
#endif

// We have a separate base class with no constructors for use in Unions.
template <typename T> class WriteBarrierBase {
public:
    void set(TiGlobalData& globalData, const TiCell* owner, T* value)
    {
        ASSERT(value);
        validateCell(value);
        setEarlyValue(globalData, owner, value);
    }

    void setMayBeNull(TiGlobalData& globalData, const TiCell* owner, T* value)
    {
        if (value)
            validateCell(value);
        setEarlyValue(globalData, owner, value);
    }

    // Should only be used by TiCell during early initialisation
    // when some basic types aren't yet completely instantiated
    void setEarlyValue(TiGlobalData&, const TiCell* owner, T* value)
    {
        this->m_cell = reinterpret_cast<TiCell*>(value);
        Heap::writeBarrier(owner, this->m_cell);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie(owner));
        ASSERT(!isZombie(m_cell));
#endif
    }
    
    T* get() const
    {
        if (m_cell)
            validateCell(m_cell);
        return reinterpret_cast<T*>(m_cell);
    }

    T* operator*() const
    {
        ASSERT(m_cell);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie(m_cell));
#endif
        validateCell<T>(static_cast<T*>(m_cell));
        return static_cast<T*>(m_cell);
    }

    T* operator->() const
    {
        ASSERT(m_cell);
        validateCell(static_cast<T*>(m_cell));
        return static_cast<T*>(m_cell);
    }

    void clear() { m_cell = 0; }
    
    TiCell** slot() { return &m_cell; }
    
    typedef T* (WriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return m_cell ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    
    bool operator!() const { return !m_cell; }

    void setWithoutWriteBarrier(T* value)
    {
        this->m_cell = reinterpret_cast<TiCell*>(value);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!m_cell || !isZombie(m_cell));
#endif
    }

#if ENABLE(GC_VALIDATION)
    T* unvalidatedGet() const { return reinterpret_cast<T*>(m_cell); }
#endif

private:
    TiCell* m_cell;
};

template <> class WriteBarrierBase<Unknown> {
public:
    void set(TiGlobalData&, const TiCell* owner, TiValue value)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie(owner));
        ASSERT(!value.isZombie());
#endif
        m_value = TiValue::encode(value);
        Heap::writeBarrier(owner, value);
    }

    void setWithoutWriteBarrier(TiValue value)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!value.isZombie());
#endif
        m_value = TiValue::encode(value);
    }

    TiValue get() const
    {
        return TiValue::decode(m_value);
    }
    void clear() { m_value = TiValue::encode(TiValue()); }
    void setUndefined() { m_value = TiValue::encode(jsUndefined()); }
    bool isNumber() const { return get().isNumber(); }
    bool isObject() const { return get().isObject(); }
    bool isNull() const { return get().isNull(); }
    bool isGetterSetter() const { return get().isGetterSetter(); }
    
    TiValue* slot()
    { 
        union {
            EncodedTiValue* v;
            TiValue* slot;
        } u;
        u.v = &m_value;
        return u.slot;
    }
    
    typedef TiValue (WriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return get() ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    bool operator!() const { return !get(); } 
    
private:
    EncodedTiValue m_value;
};

template <typename T> class WriteBarrier : public WriteBarrierBase<T> {
public:
    WriteBarrier()
    {
        this->setWithoutWriteBarrier(0);
    }

    WriteBarrier(TiGlobalData& globalData, const TiCell* owner, T* value)
    {
        this->set(globalData, owner, value);
    }

    enum MayBeNullTag { MayBeNull };
    WriteBarrier(TiGlobalData& globalData, const TiCell* owner, T* value, MayBeNullTag)
    {
        this->setMayBeNull(globalData, owner, value);
    }
};

template <> class WriteBarrier<Unknown> : public WriteBarrierBase<Unknown> {
public:
    WriteBarrier()
    {
        this->setWithoutWriteBarrier(TiValue());
    }

    WriteBarrier(TiGlobalData& globalData, const TiCell* owner, TiValue value)
    {
        this->set(globalData, owner, value);
    }
};

template <typename U, typename V> inline bool operator==(const WriteBarrierBase<U>& lhs, const WriteBarrierBase<V>& rhs)
{
    return lhs.get() == rhs.get();
}

// MarkStack functions

template<typename T> inline void MarkStack::append(WriteBarrierBase<T>* slot)
{
    internalAppend(*slot->slot());
}

inline void MarkStack::appendValues(WriteBarrierBase<Unknown>* barriers, size_t count, MarkSetProperties properties)
{
    TiValue* values = barriers->slot();
#if ENABLE(GC_VALIDATION)
    validateSet(values, count);
#endif
    if (count)
        m_markSets.append(MarkSet(values, values + count, properties));
}

} // namespace TI

#endif // WriteBarrier_h
