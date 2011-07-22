/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef TiString_h
#define TiString_h

#include "CallFrame.h"
#include "CommonIdentifiers.h"
#include "Identifier.h"
#include "JSNumberCell.h"
#include "PropertyDescriptor.h"
#include "PropertySlot.h"
#include "RopeImpl.h"

namespace TI {

    class TiString;

    TiString* jsEmptyString(TiGlobalData*);
    TiString* jsEmptyString(TiExcState*);
    TiString* jsString(TiGlobalData*, const UString&); // returns empty string if passed null string
    TiString* jsString(TiExcState*, const UString&); // returns empty string if passed null string

    TiString* jsSingleCharacterString(TiGlobalData*, UChar);
    TiString* jsSingleCharacterString(TiExcState*, UChar);
    TiString* jsSingleCharacterSubstring(TiExcState*, const UString&, unsigned offset);
    TiString* jsSubstring(TiGlobalData*, const UString&, unsigned offset, unsigned length);
    TiString* jsSubstring(TiExcState*, const UString&, unsigned offset, unsigned length);

    // Non-trivial strings are two or more characters long.
    // These functions are faster than just calling jsString.
    TiString* jsNontrivialString(TiGlobalData*, const UString&);
    TiString* jsNontrivialString(TiExcState*, const UString&);
    TiString* jsNontrivialString(TiGlobalData*, const char*);
    TiString* jsNontrivialString(TiExcState*, const char*);

    // Should be used for strings that are owned by an object that will
    // likely outlive the TiValue this makes, such as the parse tree or a
    // DOM object that contains a UString
    TiString* jsOwnedString(TiGlobalData*, const UString&); 
    TiString* jsOwnedString(TiExcState*, const UString&); 

    typedef void (*TiStringFinalizerCallback)(TiString*, void* context);
    TiString* jsStringWithFinalizer(TiExcState*, const UString&, TiStringFinalizerCallback callback, void* context);

    class JS_EXPORTCLASS TiString : public TiCell {
    public:
        friend class JIT;
        friend class TiGlobalData;
        friend class SpecializedThunkJIT;
        friend struct ThunkHelpers;

        class RopeBuilder {
        public:
            RopeBuilder(unsigned fiberCount)
                : m_index(0)
                , m_rope(RopeImpl::tryCreateUninitialized(fiberCount))
            {
            }

            bool isOutOfMemory() { return !m_rope; }

            void append(RopeImpl::Fiber& fiber)
            {
                ASSERT(m_rope);
                m_rope->initializeFiber(m_index, fiber);
            }
            void append(const UString& string)
            {
                ASSERT(m_rope);
                m_rope->initializeFiber(m_index, string.rep());
            }
            void append(TiString* jsString)
            {
                if (jsString->isRope()) {
                    for (unsigned i = 0; i < jsString->m_fiberCount; ++i)
                        append(jsString->m_other.m_fibers[i]);
                } else
                    append(jsString->string());
            }

            PassRefPtr<RopeImpl> release()
            {
                ASSERT(m_index == m_rope->fiberCount());
                return m_rope.release();
            }

            unsigned length() { return m_rope->length(); }

        private:
            unsigned m_index;
            RefPtr<RopeImpl> m_rope;
        };

        class RopeIterator {
            public:
                RopeIterator() { }

                RopeIterator(RopeImpl::Fiber* fibers, size_t fiberCount)
                {
                    ASSERT(fiberCount);
                    m_workQueue.append(WorkItem(fibers, fiberCount));
                    skipRopes();
                }

                RopeIterator& operator++()
                {
                    WorkItem& item = m_workQueue.last();
                    ASSERT(!RopeImpl::isRope(item.fibers[item.i]));
                    if (++item.i == item.fiberCount)
                        m_workQueue.removeLast();
                    skipRopes();
                    return *this;
                }

                UStringImpl* operator*()
                {
                    WorkItem& item = m_workQueue.last();
                    RopeImpl::Fiber fiber = item.fibers[item.i];
                    ASSERT(!RopeImpl::isRope(fiber));
                    return static_cast<UStringImpl*>(fiber);
                }

                bool operator!=(const RopeIterator& other) const
                {
                    return m_workQueue != other.m_workQueue;
                }

            private:
                struct WorkItem {
                    WorkItem(RopeImpl::Fiber* fibers, size_t fiberCount)
                        : fibers(fibers)
                        , fiberCount(fiberCount)
                        , i(0)
                    {
                    }

                    bool operator!=(const WorkItem& other) const
                    {
                        return fibers != other.fibers || fiberCount != other.fiberCount || i != other.i;
                    }

                    RopeImpl::Fiber* fibers;
                    size_t fiberCount;
                    size_t i;
                };

                void skipRopes()
                {
                    if (m_workQueue.isEmpty())
                        return;

                    while (1) {
                        WorkItem& item = m_workQueue.last();
                        RopeImpl::Fiber fiber = item.fibers[item.i];
                        if (!RopeImpl::isRope(fiber))
                            break;
                        RopeImpl* rope = static_cast<RopeImpl*>(fiber);
                        if (++item.i == item.fiberCount)
                            m_workQueue.removeLast();
                        m_workQueue.append(WorkItem(rope->fibers(), rope->fiberCount()));
                    }
                }

                Vector<WorkItem, 16> m_workQueue;
        };

        ALWAYS_INLINE TiString(TiGlobalData* globalData, const UString& value)
            : TiCell(globalData->stringStructure.get())
            , m_length(value.size())
            , m_value(value)
            , m_fiberCount(0)
        {
            ASSERT(!m_value.isNull());
            Heap::heap(this)->reportExtraMemoryCost(value.cost());
        }

        enum HasOtherOwnerType { HasOtherOwner };
        TiString(TiGlobalData* globalData, const UString& value, HasOtherOwnerType)
            : TiCell(globalData->stringStructure.get())
            , m_length(value.size())
            , m_value(value)
            , m_fiberCount(0)
        {
            ASSERT(!m_value.isNull());
        }
        TiString(TiGlobalData* globalData, PassRefPtr<UStringImpl> value, HasOtherOwnerType)
            : TiCell(globalData->stringStructure.get())
            , m_length(value->length())
            , m_value(value)
            , m_fiberCount(0)
        {
            ASSERT(!m_value.isNull());
        }
        TiString(TiGlobalData* globalData, PassRefPtr<RopeImpl> rope)
            : TiCell(globalData->stringStructure.get())
            , m_length(rope->length())
            , m_fiberCount(1)
        {
            m_other.m_fibers[0] = rope.releaseRef();
        }
        // This constructor constructs a new string by concatenating s1 & s2.
        // This should only be called with fiberCount <= 3.
        TiString(TiGlobalData* globalData, unsigned fiberCount, TiString* s1, TiString* s2)
            : TiCell(globalData->stringStructure.get())
            , m_length(s1->length() + s2->length())
            , m_fiberCount(fiberCount)
        {
            ASSERT(fiberCount <= s_maxInternalRopeLength);
            unsigned index = 0;
            appendStringInConstruct(index, s1);
            appendStringInConstruct(index, s2);
            ASSERT(fiberCount == index);
        }
        // This constructor constructs a new string by concatenating s1 & s2.
        // This should only be called with fiberCount <= 3.
        TiString(TiGlobalData* globalData, unsigned fiberCount, TiString* s1, const UString& u2)
            : TiCell(globalData->stringStructure.get())
            , m_length(s1->length() + u2.size())
            , m_fiberCount(fiberCount)
        {
            ASSERT(fiberCount <= s_maxInternalRopeLength);
            unsigned index = 0;
            appendStringInConstruct(index, s1);
            appendStringInConstruct(index, u2);
            ASSERT(fiberCount == index);
        }
        // This constructor constructs a new string by concatenating s1 & s2.
        // This should only be called with fiberCount <= 3.
        TiString(TiGlobalData* globalData, unsigned fiberCount, const UString& u1, TiString* s2)
            : TiCell(globalData->stringStructure.get())
            , m_length(u1.size() + s2->length())
            , m_fiberCount(fiberCount)
        {
            ASSERT(fiberCount <= s_maxInternalRopeLength);
            unsigned index = 0;
            appendStringInConstruct(index, u1);
            appendStringInConstruct(index, s2);
            ASSERT(fiberCount == index);
        }
        // This constructor constructs a new string by concatenating v1, v2 & v3.
        // This should only be called with fiberCount <= 3 ... which since every
        // value must require a fiberCount of at least one implies that the length
        // for each value must be exactly 1!
        TiString(TiExcState* exec, TiValue v1, TiValue v2, TiValue v3)
            : TiCell(exec->globalData().stringStructure.get())
            , m_length(0)
            , m_fiberCount(s_maxInternalRopeLength)
        {
            unsigned index = 0;
            appendValueInConstructAndIncrementLength(exec, index, v1);
            appendValueInConstructAndIncrementLength(exec, index, v2);
            appendValueInConstructAndIncrementLength(exec, index, v3);
            ASSERT(index == s_maxInternalRopeLength);
        }

        // This constructor constructs a new string by concatenating u1 & u2.
        TiString(TiGlobalData* globalData, const UString& u1, const UString& u2)
            : TiCell(globalData->stringStructure.get())
            , m_length(u1.size() + u2.size())
            , m_fiberCount(2)
        {
            unsigned index = 0;
            appendStringInConstruct(index, u1);
            appendStringInConstruct(index, u2);
            ASSERT(index <= s_maxInternalRopeLength);
        }

        // This constructor constructs a new string by concatenating u1, u2 & u3.
        TiString(TiGlobalData* globalData, const UString& u1, const UString& u2, const UString& u3)
            : TiCell(globalData->stringStructure.get())
            , m_length(u1.size() + u2.size() + u3.size())
            , m_fiberCount(s_maxInternalRopeLength)
        {
            unsigned index = 0;
            appendStringInConstruct(index, u1);
            appendStringInConstruct(index, u2);
            appendStringInConstruct(index, u3);
            ASSERT(index <= s_maxInternalRopeLength);
        }

        TiString(TiGlobalData* globalData, const UString& value, TiStringFinalizerCallback finalizer, void* context)
            : TiCell(globalData->stringStructure.get())
            , m_length(value.size())
            , m_value(value)
            , m_fiberCount(0)
        {
            ASSERT(!m_value.isNull());
            // nasty hack because we can't union non-POD types
            m_other.m_finalizerCallback = finalizer;
            m_other.m_finalizerContext = context;
            Heap::heap(this)->reportExtraMemoryCost(value.cost());
        }

        ~TiString()
        {
            ASSERT(vptr() == TiGlobalData::jsStringVPtr);
            for (unsigned i = 0; i < m_fiberCount; ++i)
                RopeImpl::deref(m_other.m_fibers[i]);

            if (!m_fiberCount && m_other.m_finalizerCallback)
                m_other.m_finalizerCallback(this, m_other.m_finalizerContext);
        }

        const UString& value(TiExcState* exec) const
        {
            if (isRope())
                resolveRope(exec);
            return m_value;
        }
        const UString& tryGetValue() const
        {
            if (isRope())
                resolveRope(0);
            return m_value;
        }
        unsigned length() { return m_length; }

        bool getStringPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        bool getStringPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        bool getStringPropertyDescriptor(TiExcState*, const Identifier& propertyName, PropertyDescriptor&);

        bool canGetIndex(unsigned i) { return i < m_length; }
        TiString* getIndex(TiExcState*, unsigned);
        TiString* getIndexSlowCase(TiExcState*, unsigned);

        TiValue replaceCharacter(TiExcState*, UChar, const UString& replacement);

        static PassRefPtr<Structure> createStructure(TiValue proto) { return Structure::create(proto, TypeInfo(StringType, OverridesGetOwnPropertySlot | NeedsThisConversion), AnonymousSlotCount); }

    private:
        enum VPtrStealingHackType { VPtrStealingHack };
        TiString(VPtrStealingHackType) 
            : TiCell(0)
            , m_fiberCount(0)
        {
        }

        void resolveRope(TiExcState*) const;
        TiString* substringFromRope(TiExcState*, unsigned offset, unsigned length);

        void appendStringInConstruct(unsigned& index, const UString& string)
        {
            UStringImpl* impl = string.rep();
            impl->ref();
            m_other.m_fibers[index++] = impl;
        }

        void appendStringInConstruct(unsigned& index, TiString* jsString)
        {
            if (jsString->isRope()) {
                for (unsigned i = 0; i < jsString->m_fiberCount; ++i) {
                    RopeImpl::Fiber fiber = jsString->m_other.m_fibers[i];
                    fiber->ref();
                    m_other.m_fibers[index++] = fiber;
                }
            } else
                appendStringInConstruct(index, jsString->string());
        }

        void appendValueInConstructAndIncrementLength(TiExcState* exec, unsigned& index, TiValue v)
        {
            if (v.isString()) {
                ASSERT(asCell(v)->isString());
                TiString* s = static_cast<TiString*>(asCell(v));
                ASSERT(s->size() == 1);
                appendStringInConstruct(index, s);
                m_length += s->length();
            } else {
                UString u(v.toString(exec));
                UStringImpl* impl = u.rep();
                impl->ref();
                m_other.m_fibers[index++] = impl;
                m_length += u.size();
            }
        }

        virtual TiValue toPrimitive(TiExcState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(TiExcState*, double& number, TiValue& value);
        virtual bool toBoolean(TiExcState*) const;
        virtual double toNumber(TiExcState*) const;
        virtual TiObject* toObject(TiExcState*) const;
        virtual UString toString(TiExcState*) const;

        virtual TiObject* toThisObject(TiExcState*) const;

        // Actually getPropertySlot, not getOwnPropertySlot (see TiCell).
        virtual bool getOwnPropertySlot(TiExcState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(TiExcState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(TiExcState*, const Identifier&, PropertyDescriptor&);

        static const unsigned s_maxInternalRopeLength = 3;

        // A string is represented either by a UString or a RopeImpl.
        unsigned m_length;
        mutable UString m_value;
        mutable unsigned m_fiberCount;
        // This structure exists to support a temporary workaround for a GC issue.
        struct TiStringFinalizerStruct {
            TiStringFinalizerStruct() : m_finalizerCallback(0) {}
            union {
                mutable RopeImpl::Fiber m_fibers[s_maxInternalRopeLength];
                struct {
                    TiStringFinalizerCallback m_finalizerCallback;
                    void* m_finalizerContext;
                };
            };
        } m_other;

        bool isRope() const { return m_fiberCount; }
        UString& string() { ASSERT(!isRope()); return m_value; }
        unsigned size() { return m_fiberCount ? m_fiberCount : 1; }

        friend TiValue jsString(TiExcState* exec, TiString* s1, TiString* s2);
        friend TiValue jsString(TiExcState* exec, const UString& u1, TiString* s2);
        friend TiValue jsString(TiExcState* exec, TiString* s1, const UString& u2);
        friend TiValue jsString(TiExcState* exec, Register* strings, unsigned count);
        friend TiValue jsString(TiExcState* exec, TiValue thisValue, const ArgList& args);
        friend TiString* jsStringWithFinalizer(TiExcState*, const UString&, TiStringFinalizerCallback callback, void* context);
        friend TiString* jsSubstring(TiExcState* exec, TiString* s, unsigned offset, unsigned length);
    };

    TiString* asString(TiValue);

    // When an object is created from a different DLL, MSVC changes vptr to a "local" one right after invoking a constructor,
    // see <http://groups.google.com/group/microsoft.public.vc.language/msg/55cdcefeaf770212>.
    // This breaks isTiString(), and we don't need that hack anyway, so we change vptr back to primary one.
    // The below function must be called by any inline function that invokes a TiString constructor.
#if COMPILER(MSVC) && !defined(BUILDING_TiCore)
    inline TiString* fixupVPtr(TiGlobalData* globalData, TiString* string) { string->setVPtr(globalData->jsStringVPtr); return string; }
#else
    inline TiString* fixupVPtr(TiGlobalData*, TiString* string) { return string; }
#endif

    inline TiString* asString(TiValue value)
    {
        ASSERT(asCell(value)->isString());
        return static_cast<TiString*>(asCell(value));
    }

    inline TiString* jsEmptyString(TiGlobalData* globalData)
    {
        return globalData->smallStrings.emptyString(globalData);
    }

    inline TiString* jsSingleCharacterString(TiGlobalData* globalData, UChar c)
    {
        if (c <= 0xFF)
            return globalData->smallStrings.singleCharacterString(globalData, c);
        return fixupVPtr(globalData, new (globalData) TiString(globalData, UString(&c, 1)));
    }

    inline TiString* jsSingleCharacterSubstring(TiExcState* exec, const UString& s, unsigned offset)
    {
        TiGlobalData* globalData = &exec->globalData();
        ASSERT(offset < static_cast<unsigned>(s.size()));
        UChar c = s.data()[offset];
        if (c <= 0xFF)
            return globalData->smallStrings.singleCharacterString(globalData, c);
        return fixupVPtr(globalData, new (globalData) TiString(globalData, UString(UStringImpl::create(s.rep(), offset, 1))));
    }

    inline TiString* jsNontrivialString(TiGlobalData* globalData, const char* s)
    {
        ASSERT(s);
        ASSERT(s[0]);
        ASSERT(s[1]);
        return fixupVPtr(globalData, new (globalData) TiString(globalData, s));
    }

    inline TiString* jsNontrivialString(TiGlobalData* globalData, const UString& s)
    {
        ASSERT(s.size() > 1);
        return fixupVPtr(globalData, new (globalData) TiString(globalData, s));
    }

    inline TiString* TiString::getIndex(TiExcState* exec, unsigned i)
    {
        ASSERT(canGetIndex(i));
        if (isRope())
            return getIndexSlowCase(exec, i);
        ASSERT(i < m_value.size());
        return jsSingleCharacterSubstring(exec, m_value, i);
    }

    inline TiString* jsString(TiGlobalData* globalData, const UString& s)
    {
        int size = s.size();
        if (!size)
            return globalData->smallStrings.emptyString(globalData);
        if (size == 1) {
            UChar c = s.data()[0];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return fixupVPtr(globalData, new (globalData) TiString(globalData, s));
    }

    inline TiString* jsStringWithFinalizer(TiExcState* exec, const UString& s, TiStringFinalizerCallback callback, void* context)
    {
        ASSERT(s.size() && (s.size() > 1 || s.data()[0] > 0xFF));
        TiGlobalData* globalData = &exec->globalData();
        return fixupVPtr(globalData, new (globalData) TiString(globalData, s, callback, context));
    }
    
    inline TiString* jsSubstring(TiExcState* exec, TiString* s, unsigned offset, unsigned length)
    {
        ASSERT(offset <= static_cast<unsigned>(s->length()));
        ASSERT(length <= static_cast<unsigned>(s->length()));
        ASSERT(offset + length <= static_cast<unsigned>(s->length()));
        TiGlobalData* globalData = &exec->globalData();
        if (!length)
            return globalData->smallStrings.emptyString(globalData);
        if (s->isRope())
            return s->substringFromRope(exec, offset, length);
        return jsSubstring(globalData, s->m_value, offset, length);
    }

    inline TiString* jsSubstring(TiGlobalData* globalData, const UString& s, unsigned offset, unsigned length)
    {
        ASSERT(offset <= static_cast<unsigned>(s.size()));
        ASSERT(length <= static_cast<unsigned>(s.size()));
        ASSERT(offset + length <= static_cast<unsigned>(s.size()));
        if (!length)
            return globalData->smallStrings.emptyString(globalData);
        if (length == 1) {
            UChar c = s.data()[offset];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return fixupVPtr(globalData, new (globalData) TiString(globalData, UString(UStringImpl::create(s.rep(), offset, length)), TiString::HasOtherOwner));
    }

    inline TiString* jsOwnedString(TiGlobalData* globalData, const UString& s)
    {
        int size = s.size();
        if (!size)
            return globalData->smallStrings.emptyString(globalData);
        if (size == 1) {
            UChar c = s.data()[0];
            if (c <= 0xFF)
                return globalData->smallStrings.singleCharacterString(globalData, c);
        }
        return fixupVPtr(globalData, new (globalData) TiString(globalData, s, TiString::HasOtherOwner));
    }

    inline TiString* jsEmptyString(TiExcState* exec) { return jsEmptyString(&exec->globalData()); }
    inline TiString* jsString(TiExcState* exec, const UString& s) { return jsString(&exec->globalData(), s); }
    inline TiString* jsSingleCharacterString(TiExcState* exec, UChar c) { return jsSingleCharacterString(&exec->globalData(), c); }
    inline TiString* jsSubstring(TiExcState* exec, const UString& s, unsigned offset, unsigned length) { return jsSubstring(&exec->globalData(), s, offset, length); }
    inline TiString* jsNontrivialString(TiExcState* exec, const UString& s) { return jsNontrivialString(&exec->globalData(), s); }
    inline TiString* jsNontrivialString(TiExcState* exec, const char* s) { return jsNontrivialString(&exec->globalData(), s); }
    inline TiString* jsOwnedString(TiExcState* exec, const UString& s) { return jsOwnedString(&exec->globalData(), s); } 

    ALWAYS_INLINE bool TiString::getStringPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        if (propertyName == exec->propertyNames().length) {
            slot.setValue(jsNumber(exec, m_length));
            return true;
        }

        bool isStrictUInt32;
        unsigned i = propertyName.toStrictUInt32(&isStrictUInt32);
        if (isStrictUInt32 && i < m_length) {
            slot.setValue(getIndex(exec, i));
            return true;
        }

        return false;
    }
        
    ALWAYS_INLINE bool TiString::getStringPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
    {
        if (propertyName < m_length) {
            slot.setValue(getIndex(exec, propertyName));
            return true;
        }

        return false;
    }

    inline bool isTiString(TiGlobalData* globalData, TiValue v) { return v.isCell() && v.asCell()->vptr() == globalData->jsStringVPtr; }

    // --- TiValue inlines ----------------------------

    inline UString TiValue::toString(TiExcState* exec) const
    {
        if (isString())
            return static_cast<TiString*>(asCell())->value(exec);
        if (isInt32())
            return exec->globalData().numericStrings.add(asInt32());
        if (isDouble())
            return exec->globalData().numericStrings.add(asDouble());
        if (isTrue())
            return "true";
        if (isFalse())
            return "false";
        if (isNull())
            return "null";
        if (isUndefined())
            return "undefined";
        ASSERT(isCell());
        return asCell()->toString(exec);
    }

    inline UString TiValue::toPrimitiveString(TiExcState* exec) const
    {
        if (isString())
            return static_cast<TiString*>(asCell())->value(exec);
        if (isInt32())
            return exec->globalData().numericStrings.add(asInt32());
        if (isDouble())
            return exec->globalData().numericStrings.add(asDouble());
        if (isTrue())
            return "true";
        if (isFalse())
            return "false";
        if (isNull())
            return "null";
        if (isUndefined())
            return "undefined";
        ASSERT(isCell());
        return asCell()->toPrimitive(exec, NoPreference).toString(exec);
    }

} // namespace TI

#endif // TiString_h
