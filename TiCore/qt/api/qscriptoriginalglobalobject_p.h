/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef qscriptoriginalglobalobject_p_h
#define qscriptoriginalglobalobject_p_h

#include <TiCore/Ti.h>
#include <TiCore/TiRetainPtr.h>
#include <QtCore/qvector.h>

/*!
    \internal
    This class is a workaround for missing JSC C API functionality. This class keeps all important
    properties of an original (default) global object, so we can use it even if the global object was
    changed.

    FIXME this class is a container for workarounds :-) it should be replaced by proper JSC C API calls.

    The class have to be created on the QScriptEnginePrivate creation time (before any change got applied to
    global object).
*/
class QScriptOriginalGlobalObject {
public:
    inline QScriptOriginalGlobalObject(TiGlobalContextRef context);
    inline ~QScriptOriginalGlobalObject();

    inline bool objectHasOwnProperty(TiObjectRef object, TiStringRef property) const;
    inline QVector<TiStringRef> objectGetOwnPropertyNames(TiObjectRef object) const;

    inline bool isDate(TiValueRef value) const;
    inline bool isArray(TiValueRef value) const;
    inline bool isError(TiValueRef value) const;

    inline TiValueRef functionPrototype() const;
private:
    inline bool isType(TiValueRef value, TiObjectRef constructor, TiValueRef prototype) const;
    inline void initializeMember(TiObjectRef globalObject, TiStringRef prototypeName, const char* type, TiObjectRef& constructor, TiValueRef& prototype);

    // Copy of the global context reference (the same as in QScriptEnginePrivate).
    TiGlobalContextRef m_context;

    // Copy of constructors and prototypes used in isType functions.
    TiObjectRef m_arrayConstructor;
    TiValueRef m_arrayPrototype;
    TiObjectRef m_errorConstructor;
    TiValueRef m_errorPrototype;
    TiObjectRef m_functionConstructor;
    TiValueRef m_functionPrototype;
    TiObjectRef m_dateConstructor;
    TiValueRef m_datePrototype;

    // Reference to standard JS functions that are not exposed by JSC C API.
    TiObjectRef m_hasOwnPropertyFunction;
    TiObjectRef m_getOwnPropertyNamesFunction;
};

QScriptOriginalGlobalObject::QScriptOriginalGlobalObject(TiGlobalContextRef context)
    : m_context(TiGlobalContextRetain(context))
{
    TiObjectRef globalObject = TiContextGetGlobalObject(m_context);
    TiValueRef exception = 0;
    TiRetainPtr<TiStringRef> propertyName;

    propertyName.adopt(TiStringCreateWithUTF8CString("prototype"));
    initializeMember(globalObject, propertyName.get(), "Array", m_arrayConstructor, m_arrayPrototype);
    initializeMember(globalObject, propertyName.get(), "Error", m_errorConstructor, m_errorPrototype);
    initializeMember(globalObject, propertyName.get(), "Function", m_functionConstructor, m_functionPrototype);
    initializeMember(globalObject, propertyName.get(), "Date", m_dateConstructor, m_datePrototype);

    propertyName.adopt(TiStringCreateWithUTF8CString("hasOwnProperty"));
    m_hasOwnPropertyFunction = const_cast<TiObjectRef>(TiObjectGetProperty(m_context, globalObject, propertyName.get(), &exception));
    TiValueProtect(m_context, m_hasOwnPropertyFunction);
    Q_ASSERT(TiValueIsObject(m_context, m_hasOwnPropertyFunction));
    Q_ASSERT(TiObjectIsFunction(m_context, m_hasOwnPropertyFunction));
    Q_ASSERT(!exception);

    propertyName.adopt(TiStringCreateWithUTF8CString("Object"));
    TiObjectRef objectConstructor
            = const_cast<TiObjectRef>(TiObjectGetProperty(m_context, globalObject, propertyName.get(), &exception));
    propertyName.adopt(TiStringCreateWithUTF8CString("getOwnPropertyNames"));
    m_getOwnPropertyNamesFunction
            = const_cast<TiObjectRef>(TiObjectGetProperty(m_context, objectConstructor, propertyName.get(), &exception));
    TiValueProtect(m_context, m_getOwnPropertyNamesFunction);
    Q_ASSERT(TiValueIsObject(m_context, m_getOwnPropertyNamesFunction));
    Q_ASSERT(TiObjectIsFunction(m_context, m_getOwnPropertyNamesFunction));
    Q_ASSERT(!exception);
}

inline void QScriptOriginalGlobalObject::initializeMember(TiObjectRef globalObject, TiStringRef prototypeName, const char* type, TiObjectRef& constructor, TiValueRef& prototype)
{
    TiRetainPtr<TiStringRef> typeName(Adopt, TiStringCreateWithUTF8CString(type));
    TiValueRef exception = 0;

    // Save references to the Type constructor and prototype.
    TiValueRef typeConstructor = TiObjectGetProperty(m_context, globalObject, typeName.get(), &exception);
    Q_ASSERT(TiValueIsObject(m_context, typeConstructor));
    constructor = TiValueToObject(m_context, typeConstructor, &exception);
    TiValueProtect(m_context, constructor);

    // Note that this is not the [[Prototype]] internal property (which we could
    // get via TiObjectGetPrototype), but the Type.prototype, that will be set
    // as [[Prototype]] of Type instances.
    prototype = TiObjectGetProperty(m_context, constructor, prototypeName, &exception);
    Q_ASSERT(TiValueIsObject(m_context, prototype));
    TiValueProtect(m_context, prototype);
    Q_ASSERT(!exception);
}

QScriptOriginalGlobalObject::~QScriptOriginalGlobalObject()
{
    TiValueUnprotect(m_context, m_arrayConstructor);
    TiValueUnprotect(m_context, m_arrayPrototype);
    TiValueUnprotect(m_context, m_errorConstructor);
    TiValueUnprotect(m_context, m_errorPrototype);
    TiValueUnprotect(m_context, m_functionConstructor);
    TiValueUnprotect(m_context, m_functionPrototype);
    TiValueUnprotect(m_context, m_dateConstructor);
    TiValueUnprotect(m_context, m_datePrototype);
    TiValueUnprotect(m_context, m_hasOwnPropertyFunction);
    TiValueUnprotect(m_context, m_getOwnPropertyNamesFunction);
    TiGlobalContextRelease(m_context);
}

inline bool QScriptOriginalGlobalObject::objectHasOwnProperty(TiObjectRef object, TiStringRef property) const
{
    // FIXME This function should be replaced by JSC C API.
    TiValueRef exception = 0;
    TiValueRef propertyName[] = { TiValueMakeString(m_context, property) };
    TiValueRef result = TiObjectCallAsFunction(m_context, m_hasOwnPropertyFunction, object, 1, propertyName, &exception);
    return exception ? false : TiValueToBoolean(m_context, result);
}

/*!
    \internal
    This method gives ownership of all TiStringRefs.
*/
inline QVector<TiStringRef> QScriptOriginalGlobalObject::objectGetOwnPropertyNames(TiObjectRef object) const
{
    TiValueRef exception = 0;
    TiObjectRef propertyNames
            = const_cast<TiObjectRef>(TiObjectCallAsFunction(m_context,
                                                            m_getOwnPropertyNamesFunction,
                                                            /* thisObject */ 0,
                                                            /* argumentCount */ 1,
                                                            &object,
                                                            &exception));
    Q_ASSERT(TiValueIsObject(m_context, propertyNames));
    Q_ASSERT(!exception);
    TiStringRef lengthName = QScriptConverter::toString("length");
    int count = TiValueToNumber(m_context, TiObjectGetProperty(m_context, propertyNames, lengthName, &exception), &exception);

    Q_ASSERT(!exception);
    QVector<TiStringRef> names;
    names.reserve(count);
    for (int i = 0; i < count; ++i) {
        TiValueRef tmp = TiObjectGetPropertyAtIndex(m_context, propertyNames, i, &exception);
        names.append(TiValueToStringCopy(m_context, tmp, &exception));
        Q_ASSERT(!exception);
    }
    return names;
}

inline bool QScriptOriginalGlobalObject::isDate(TiValueRef value) const
{
    return isType(value, m_dateConstructor, m_datePrototype);
}

inline bool QScriptOriginalGlobalObject::isArray(TiValueRef value) const
{
    return isType(value, m_arrayConstructor, m_arrayPrototype);
}

inline bool QScriptOriginalGlobalObject::isError(TiValueRef value) const
{
    return isType(value, m_errorConstructor, m_errorPrototype);
}

inline TiValueRef QScriptOriginalGlobalObject::functionPrototype() const
{
    return m_functionPrototype;
}

inline bool QScriptOriginalGlobalObject::isType(TiValueRef value, TiObjectRef constructor, TiValueRef prototype) const
{
    // JSC API doesn't export the [[Class]] information for the builtins. But we know that a value
    // is an object of the Type if it was created with the Type constructor or if it is the Type.prototype.
    TiValueRef exception = 0;
    bool result = TiValueIsInstanceOfConstructor(m_context, value, constructor, &exception) || TiValueIsStrictEqual(m_context, value, prototype);
    Q_ASSERT(!exception);
    return result;
}

#endif // qscriptoriginalglobalobject_p_h
