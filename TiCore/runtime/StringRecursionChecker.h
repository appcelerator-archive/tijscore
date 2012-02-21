/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef StringRecursionChecker_h
#define StringRecursionChecker_h

#include "Interpreter.h"

namespace TI {

class StringRecursionChecker {
    WTF_MAKE_NONCOPYABLE(StringRecursionChecker);

public:
    StringRecursionChecker(TiExcState*, TiObject* thisObject);
    ~StringRecursionChecker();

    EncodedTiValue earlyReturnValue() const; // 0 if everything is OK, value to return for failure cases

private:
    EncodedTiValue throwStackOverflowError();
    EncodedTiValue emptyString();
    EncodedTiValue performCheck();

    TiExcState* m_exec;
    TiObject* m_thisObject;
    EncodedTiValue m_earlyReturnValue;
};

inline EncodedTiValue StringRecursionChecker::performCheck()
{
    int size = m_exec->globalData().stringRecursionCheckVisitedObjects.size();
    if (size >= MaxSmallThreadReentryDepth && size >= m_exec->globalData().maxReentryDepth)
        return throwStackOverflowError();
    bool alreadyVisited = !m_exec->globalData().stringRecursionCheckVisitedObjects.add(m_thisObject).second;
    if (alreadyVisited)
        return emptyString(); // Return empty string to avoid infinite recursion.
    return 0; // Indicate success.
}

inline StringRecursionChecker::StringRecursionChecker(TiExcState* exec, TiObject* thisObject)
    : m_exec(exec)
    , m_thisObject(thisObject)
    , m_earlyReturnValue(performCheck())
{
}

inline EncodedTiValue StringRecursionChecker::earlyReturnValue() const
{
    return m_earlyReturnValue;
}

inline StringRecursionChecker::~StringRecursionChecker()
{
    if (m_earlyReturnValue)
        return;
    ASSERT(m_exec->globalData().stringRecursionCheckVisitedObjects.contains(m_thisObject));
    m_exec->globalData().stringRecursionCheckVisitedObjects.remove(m_thisObject);
}

}

#endif
