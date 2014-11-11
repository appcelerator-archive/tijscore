/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

#ifndef JSDestructibleObject_h
#define JSDestructibleObject_h

#include "JSObject.h"

namespace TI {

struct ClassInfo;

class JSDestructibleObject : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    static const bool needsDestruction = true;

    const ClassInfo* classInfo() const { return m_classInfo; }
    
    static ptrdiff_t classInfoOffset() { return OBJECT_OFFSETOF(JSDestructibleObject, m_classInfo); }

protected:
    JSDestructibleObject(VM& vm, Structure* structure, Butterfly* butterfly = 0)
        : JSNonFinalObject(vm, structure, butterfly)
        , m_classInfo(structure->classInfo())
    {
        ASSERT(m_classInfo);
    }

private:
    const ClassInfo* m_classInfo;
};

inline const ClassInfo* JSCell::classInfo() const
{
    if (MarkedBlock::blockFor(this)->destructorType() == MarkedBlock::Normal)
        return static_cast<const JSDestructibleObject*>(this)->classInfo();
#if ENABLE(GC_VALIDATION)
    return m_structure.unvalidatedGet()->classInfo();
#else
    return m_structure->classInfo();
#endif
}

} // namespace TI

#endif
