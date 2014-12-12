/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef WTF_FastMalloc_h
#define WTF_FastMalloc_h

#include <new>
#include <stdlib.h>
#include <wtf/Platform.h>
#include <wtf/PossiblyNull.h>
#include <wtf/StdLibExtras.h>

namespace WTI {

    // These functions call CRASH() if an allocation fails.
    WTF_EXPORT_PRIVATE void* fastMalloc(size_t);
    WTF_EXPORT_PRIVATE void* fastZeroedMalloc(size_t);
    WTF_EXPORT_PRIVATE void* fastCalloc(size_t numElements, size_t elementSize);
    WTF_EXPORT_PRIVATE void* fastRealloc(void*, size_t);
    WTF_EXPORT_PRIVATE char* fastStrDup(const char*);
    WTF_EXPORT_PRIVATE size_t fastMallocSize(const void*);
    WTF_EXPORT_PRIVATE size_t fastMallocGoodSize(size_t);

    struct TryMallocReturnValue {
        TryMallocReturnValue(void* data)
            : m_data(data)
        {
        }
        TryMallocReturnValue(const TryMallocReturnValue& source)
            : m_data(source.m_data)
        {
            source.m_data = 0;
        }
        ~TryMallocReturnValue() { ASSERT(!m_data); }
        template <typename T> bool getValue(T& data) WARN_UNUSED_RETURN;
        template <typename T> operator PossiblyNull<T>()
        { 
            T value; 
            getValue(value); 
            return PossiblyNull<T>(value);
        } 
    private:
        mutable void* m_data;
    };
    
    template <typename T> bool TryMallocReturnValue::getValue(T& data)
    {
        union u { void* data; T target; } res;
        res.data = m_data;
        data = res.target;
        bool returnValue = !!m_data;
        m_data = 0;
        return returnValue;
    }

    WTF_EXPORT_PRIVATE TryMallocReturnValue tryFastMalloc(size_t n);
    TryMallocReturnValue tryFastZeroedMalloc(size_t n);
    WTF_EXPORT_PRIVATE TryMallocReturnValue tryFastCalloc(size_t n_elements, size_t element_size);
    WTF_EXPORT_PRIVATE TryMallocReturnValue tryFastRealloc(void* p, size_t n);

    WTF_EXPORT_PRIVATE void fastFree(void*);

#ifndef NDEBUG    
    WTF_EXPORT_PRIVATE void fastMallocForbid();
    WTF_EXPORT_PRIVATE void fastMallocAllow();
#endif

    WTF_EXPORT_PRIVATE void releaseFastMallocFreeMemory();
    
    struct FastMallocStatistics {
        size_t reservedVMBytes;
        size_t committedVMBytes;
        size_t freeListBytes;
    };
    WTF_EXPORT_PRIVATE FastMallocStatistics fastMallocStatistics();

    // This defines a type which holds an unsigned integer and is the same
    // size as the minimally aligned memory allocation.
    typedef unsigned long long AllocAlignmentInteger;

    namespace Internal {
        enum AllocType {                    // Start with an unusual number instead of zero, because zero is common.
            AllocTypeMalloc = 0x375d6750,   // Encompasses fastMalloc, fastZeroedMalloc, fastCalloc, fastRealloc.
            AllocTypeClassNew,              // Encompasses class operator new from FastAllocBase.
            AllocTypeClassNewArray,         // Encompasses class operator new[] from FastAllocBase.
            AllocTypeNew,                   // Encompasses global operator new.
            AllocTypeNewArray               // Encompasses global operator new[].
        };

        enum {
            ValidationPrefix = 0xf00df00d,
            ValidationSuffix = 0x0badf00d
        };

        typedef unsigned ValidationTag;

        struct ValidationHeader {
            AllocType m_type;
            unsigned m_size;
            ValidationTag m_prefix;
            unsigned m_alignment;
        };

        static const int ValidationBufferSize = sizeof(ValidationHeader) + sizeof(ValidationTag);
    }

#if ENABLE(WTF_MALLOC_VALIDATION)

    // Malloc validation is a scheme whereby a tag is attached to an
    // allocation which identifies how it was originally allocated.
    // This allows us to verify that the freeing operation matches the
    // allocation operation. If memory is allocated with operator new[]
    // but freed with free or delete, this system would detect that.
    // In the implementation here, the tag is an integer prepended to
    // the allocation memory which is assigned one of the AllocType
    // enumeration values. An alternative implementation of this
    // scheme could store the tag somewhere else or ignore it.
    // Users of FastMalloc don't need to know or care how this tagging
    // is implemented.

    namespace Internal {
    
        // Handle a detected alloc/free mismatch. By default this calls CRASH().
        void fastMallocMatchFailed(void* p);

        inline ValidationHeader* fastMallocValidationHeader(void* p)
        {
            return reinterpret_cast<ValidationHeader*>(static_cast<char*>(p) - sizeof(ValidationHeader));
        }

        inline ValidationTag* fastMallocValidationSuffix(void* p)
        {
            ValidationHeader* header = fastMallocValidationHeader(p);
            if (header->m_prefix != static_cast<unsigned>(ValidationPrefix))
                fastMallocMatchFailed(p);
            
            return reinterpret_cast<ValidationTag*>(static_cast<char*>(p) + header->m_size);
        }

        // Return the AllocType tag associated with the allocated block p.
        inline AllocType fastMallocMatchValidationType(void* p)
        {
            return fastMallocValidationHeader(p)->m_type;
        }

        // Set the AllocType tag to be associaged with the allocated block p.
        inline void setFastMallocMatchValidationType(void* p, AllocType allocType)
        {
            fastMallocValidationHeader(p)->m_type = allocType;
        }

    } // namespace Internal

    // This is a higher level function which is used by FastMalloc-using code.
    inline void fastMallocMatchValidateMalloc(void* p, Internal::AllocType allocType)
    {
        if (!p)
            return;

        Internal::setFastMallocMatchValidationType(p, allocType);
    }

    // This is a higher level function which is used by FastMalloc-using code.
    inline void fastMallocMatchValidateFree(void* p, Internal::AllocType)
    {
        if (!p)
            return;
    
        Internal::ValidationHeader* header = Internal::fastMallocValidationHeader(p);
        if (header->m_prefix != static_cast<unsigned>(Internal::ValidationPrefix))
            Internal::fastMallocMatchFailed(p);

        if (*Internal::fastMallocValidationSuffix(p) != Internal::ValidationSuffix)
            Internal::fastMallocMatchFailed(p);

        Internal::setFastMallocMatchValidationType(p, Internal::AllocTypeMalloc);  // Set it to this so that fastFree thinks it's OK.
    }

    inline void fastMallocValidate(void* p)
    {
        if (!p)
            return;
        
        Internal::ValidationHeader* header = Internal::fastMallocValidationHeader(p);
        if (header->m_prefix != static_cast<unsigned>(Internal::ValidationPrefix))
            Internal::fastMallocMatchFailed(p);
        
        if (*Internal::fastMallocValidationSuffix(p) != Internal::ValidationSuffix)
            Internal::fastMallocMatchFailed(p);
    }

#else

    inline void fastMallocMatchValidateMalloc(void*, Internal::AllocType)
    {
    }

    inline void fastMallocMatchValidateFree(void*, Internal::AllocType)
    {
    }

#endif

} // namespace WTI

using WTI::fastCalloc;
using WTI::fastFree;
using WTI::fastMalloc;
using WTI::fastMallocGoodSize;
using WTI::fastMallocSize;
using WTI::fastRealloc;
using WTI::fastStrDup;
using WTI::fastZeroedMalloc;
using WTI::tryFastCalloc;
using WTI::tryFastMalloc;
using WTI::tryFastRealloc;
using WTI::tryFastZeroedMalloc;

#ifndef NDEBUG    
using WTI::fastMallocForbid;
using WTI::fastMallocAllow;
#endif

#if COMPILER(GCC) && OS(DARWIN)
#define WTF_PRIVATE_INLINE __private_extern__ inline __attribute__((always_inline))
#elif COMPILER(GCC)
#define WTF_PRIVATE_INLINE inline __attribute__((always_inline))
#elif COMPILER(MSVC)
#define WTF_PRIVATE_INLINE __forceinline
#else
#define WTF_PRIVATE_INLINE inline
#endif

#if !defined(_CRTDBG_MAP_ALLOC) && !(defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC)

// The nothrow functions here are actually not all that helpful, because fastMalloc will
// call CRASH() rather than returning 0, and returning 0 is what nothrow is all about.
// But since WebKit code never uses exceptions or nothrow at all, this is probably OK.
// Long term we will adopt FastAllocBase.h everywhere, and and replace this with
// debug-only code to make sure we don't use the system malloc via the default operator
// new by accident.

#if ENABLE(GLOBAL_FASTMALLOC_NEW)

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4290) // Disable the C++ exception specification ignored warning.
#elif COMPILER(CLANG) && defined(__has_warning)
#pragma clang diagnostic push
#if __has_warning("-Winline-new-delete")
// FIXME: The operator new, delete definitions cannot be inline per replacement.functions (17.6.4.6/3) of the C++
// standard. As a workaround, disable warnings for such usage. See <https://bugs.webkit.org/show_bug.cgi?id=124186>.
#pragma clang diagnostic ignored "-Winline-new-delete"
#endif
#endif
WTF_PRIVATE_INLINE void* operator new(size_t size) throw (std::bad_alloc) { return fastMalloc(size); }
WTF_PRIVATE_INLINE void* operator new(size_t size, const std::nothrow_t&) throw() { return fastMalloc(size); }
WTF_PRIVATE_INLINE void operator delete(void* p) throw() { fastFree(p); }
WTF_PRIVATE_INLINE void operator delete(void* p, const std::nothrow_t&) throw() { fastFree(p); }
WTF_PRIVATE_INLINE void* operator new[](size_t size) throw (std::bad_alloc) { return fastMalloc(size); }
WTF_PRIVATE_INLINE void* operator new[](size_t size, const std::nothrow_t&) throw() { return fastMalloc(size); }
WTF_PRIVATE_INLINE void operator delete[](void* p) throw() { fastFree(p); }
WTF_PRIVATE_INLINE void operator delete[](void* p, const std::nothrow_t&) throw() { fastFree(p); }
#if COMPILER(MSVC)
#pragma warning(pop)
#elif COMPILER(CLANG) && defined(__has_warning)
#pragma clang diagnostic pop
#endif

#endif // ENABLE(GLOBAL_FASTMALLOC_NEW)
#endif // !defined(_CRTDBG_MAP_ALLOC) && !(defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC)

#define WTF_FASTMALLOC_OPERATORS \
public: \
    void* operator new(size_t, void* p) { return p; } \
    void* operator new[](size_t, void* p) { return p; } \
    \
    void* operator new(size_t size) \
    { \
        void* p = ::WTI::fastMalloc(size); \
        ::WTI::fastMallocMatchValidateMalloc(p, ::WTI::Internal::AllocTypeClassNew); \
        return p; \
    } \
    \
    void operator delete(void* p) \
    { \
        ::WTI::fastMallocMatchValidateFree(p, ::WTI::Internal::AllocTypeClassNew); \
        ::WTI::fastFree(p); \
    } \
    \
    void* operator new[](size_t size) \
    { \
        void* p = ::WTI::fastMalloc(size); \
        ::WTI::fastMallocMatchValidateMalloc(p, ::WTI::Internal::AllocTypeClassNewArray); \
        return p; \
    } \
    \
    void operator delete[](void* p) \
    { \
        ::WTI::fastMallocMatchValidateFree(p, ::WTI::Internal::AllocTypeClassNewArray); \
        ::WTI::fastFree(p); \
    } \
    void* operator new(size_t, NotNullTag, void* location) \
    { \
        ASSERT(location); \
        return location; \
    } \
private: \
typedef int __thisIsHereToForceASemicolonAfterThisMacro

#if ENABLE(GLOBAL_FASTMALLOC_NEW)
#define WTF_MAKE_FAST_ALLOCATED
#else
#define WTF_MAKE_FAST_ALLOCATED WTF_FASTMALLOC_OPERATORS
#endif


#endif /* WTF_FastMalloc_h */
