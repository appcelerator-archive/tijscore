/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef qscriptengine_p_h
#define qscriptengine_p_h

#include "qscriptconverter_p.h"
#include "qscriptengine.h"
#include "qscriptoriginalglobalobject_p.h"
#include "qscriptstring_p.h"
#include "qscriptsyntaxcheckresult_p.h"
#include "qscriptvalue.h"
#include <TiCore/Ti.h>
#include <TiCore/TiRetainPtr.h>
#include <TiBasePrivate.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

class QScriptEngine;
class QScriptSyntaxCheckResultPrivate;

class QScriptEnginePrivate : public QSharedData {
public:
    static QScriptEnginePrivate* get(const QScriptEngine* q) { Q_ASSERT(q); return q->d_ptr.data(); }
    static QScriptEngine* get(const QScriptEnginePrivate* d) { Q_ASSERT(d); return d->q_ptr; }

    QScriptEnginePrivate(const QScriptEngine*);
    ~QScriptEnginePrivate();

    enum SetExceptionFlag {
        IgnoreNullException = 0x01,
        NotNullException = 0x02,
    };

    QScriptSyntaxCheckResultPrivate* checkSyntax(const QString& program);
    QScriptValuePrivate* evaluate(const QString& program, const QString& fileName, int lineNumber);
    QScriptValuePrivate* evaluate(const QScriptProgramPrivate* program);
    inline TiValueRef evaluate(TiStringRef program, TiStringRef fileName, int lineNumber);

    inline bool hasUncaughtException() const;
    QScriptValuePrivate* uncaughtException() const;
    inline void clearExceptions();
    inline void setException(TiValueRef exception, const /* SetExceptionFlags */ unsigned flags = IgnoreNullException);
    inline int uncaughtExceptionLineNumber() const;
    inline QStringList uncaughtExceptionBacktrace() const;

    inline void collectGarbage();
    inline void reportAdditionalMemoryCost(int cost);

    inline TiValueRef makeTiValue(double number) const;
    inline TiValueRef makeTiValue(int number) const;
    inline TiValueRef makeTiValue(uint number) const;
    inline TiValueRef makeTiValue(const QString& string) const;
    inline TiValueRef makeTiValue(bool number) const;
    inline TiValueRef makeTiValue(QScriptValue::SpecialValue value) const;

    QScriptValuePrivate* newFunction(QScriptEngine::FunctionSignature fun, QScriptValuePrivate* prototype, int length);
    QScriptValuePrivate* newFunction(QScriptEngine::FunctionWithArgSignature fun, void* arg);
    QScriptValuePrivate* newFunction(TiObjectRef funObject, QScriptValuePrivate* prototype);

    QScriptValuePrivate* newObject() const;
    QScriptValuePrivate* newArray(uint length);
    QScriptValuePrivate* newDate(qsreal value);
    QScriptValuePrivate* globalObject() const;

    inline QScriptStringPrivate* toStringHandle(const QString& str) const;

    inline operator TiGlobalContextRef() const;

    inline bool isDate(TiValueRef value) const;
    inline bool isArray(TiValueRef value) const;
    inline bool isError(TiValueRef value) const;
    inline bool objectHasOwnProperty(TiObjectRef object, TiStringRef property) const;
    inline QVector<TiStringRef> objectGetOwnPropertyNames(TiObjectRef object) const;

private:
    QScriptEngine* q_ptr;
    TiGlobalContextRef m_context;
    TiValueRef m_exception;

    QScriptOriginalGlobalObject m_originalGlobalObject;

    TiClassRef m_nativeFunctionClass;
    TiClassRef m_nativeFunctionWithArgClass;
};


/*!
  Evaluates given Ti program and returns result of the evaluation.
  \attention this function doesn't take ownership of the parameters.
  \internal
*/
TiValueRef QScriptEnginePrivate::evaluate(TiStringRef program, TiStringRef fileName, int lineNumber)
{
    TiValueRef exception;
    TiValueRef result = TiEvalScript(m_context, program, /* Global Object */ 0, fileName, lineNumber, &exception);
    if (!result) {
        setException(exception, NotNullException);
        return exception; // returns an exception
    }
    clearExceptions();
    return result;
}

bool QScriptEnginePrivate::hasUncaughtException() const
{
    return m_exception;
}

void QScriptEnginePrivate::clearExceptions()
{
    if (m_exception)
        TiValueUnprotect(m_context, m_exception);
    m_exception = 0;
}

void QScriptEnginePrivate::setException(TiValueRef exception, const /* SetExceptionFlags */ unsigned flags)
{
    if (!((flags & NotNullException) || exception))
        return;
    Q_ASSERT(exception);

    if (m_exception)
        TiValueUnprotect(m_context, m_exception);
    TiValueProtect(m_context, exception);
    m_exception = exception;
}

int QScriptEnginePrivate::uncaughtExceptionLineNumber() const
{
    if (!hasUncaughtException() || !TiValueIsObject(m_context, m_exception))
        return -1;

    TiValueRef exception = 0;
    TiRetainPtr<TiStringRef> lineNumberPropertyName(Adopt, QScriptConverter::toString("line"));
    TiValueRef lineNumber = TiObjectGetProperty(m_context, const_cast<TiObjectRef>(m_exception), lineNumberPropertyName.get(), &exception);
    int result = TiValueToNumber(m_context, lineNumber, &exception);
    return exception ? -1 : result;
}

QStringList QScriptEnginePrivate::uncaughtExceptionBacktrace() const
{
    if (!hasUncaughtException() || !TiValueIsObject(m_context, m_exception))
        return QStringList();

    TiValueRef exception = 0;
    TiRetainPtr<TiStringRef> fileNamePropertyName(Adopt, QScriptConverter::toString("sourceURL"));
    TiRetainPtr<TiStringRef> lineNumberPropertyName(Adopt, QScriptConverter::toString("line"));
    TiValueRef jsFileName = TiObjectGetProperty(m_context, const_cast<TiObjectRef>(m_exception), fileNamePropertyName.get(), &exception);
    TiValueRef jsLineNumber = TiObjectGetProperty(m_context, const_cast<TiObjectRef>(m_exception), lineNumberPropertyName.get(), &exception);
    TiRetainPtr<TiStringRef> fileName(Adopt, TiValueToStringCopy(m_context, jsFileName, &exception));
    int lineNumber = TiValueToNumber(m_context, jsLineNumber, &exception);
    return QStringList(QString::fromLatin1("<anonymous>()@%0:%1")
            .arg(QScriptConverter::toString(fileName.get()))
            .arg(QScriptConverter::toString(exception ? -1 : lineNumber)));
}

void QScriptEnginePrivate::collectGarbage()
{
    TiGarbageCollect(m_context);
}

void QScriptEnginePrivate::reportAdditionalMemoryCost(int cost)
{
    if (cost > 0)
        JSReportExtraMemoryCost(m_context, cost);
}

TiValueRef QScriptEnginePrivate::makeTiValue(double number) const
{
    return TiValueMakeNumber(m_context, number);
}

TiValueRef QScriptEnginePrivate::makeTiValue(int number) const
{
    return TiValueMakeNumber(m_context, number);
}

TiValueRef QScriptEnginePrivate::makeTiValue(uint number) const
{
    return TiValueMakeNumber(m_context, number);
}

TiValueRef QScriptEnginePrivate::makeTiValue(const QString& string) const
{
    TiStringRef tmp = QScriptConverter::toString(string);
    TiValueRef result = TiValueMakeString(m_context, tmp);
    TiStringRelease(tmp);
    return result;
}

TiValueRef QScriptEnginePrivate::makeTiValue(bool value) const
{
    return TiValueMakeBoolean(m_context, value);
}

TiValueRef QScriptEnginePrivate::makeTiValue(QScriptValue::SpecialValue value) const
{
    if (value == QScriptValue::NullValue)
        return TiValueMakeNull(m_context);
    return TiValueMakeUndefined(m_context);
}

QScriptStringPrivate* QScriptEnginePrivate::toStringHandle(const QString& str) const
{
    return new QScriptStringPrivate(str);
}

QScriptEnginePrivate::operator TiGlobalContextRef() const
{
    Q_ASSERT(this);
    return m_context;
}

bool QScriptEnginePrivate::isDate(TiValueRef value) const
{
    return m_originalGlobalObject.isDate(value);
}

bool QScriptEnginePrivate::isArray(TiValueRef value) const
{
    return m_originalGlobalObject.isArray(value);
}

bool QScriptEnginePrivate::isError(TiValueRef value) const
{
    return m_originalGlobalObject.isError(value);
}

inline bool QScriptEnginePrivate::objectHasOwnProperty(TiObjectRef object, TiStringRef property) const
{
    // FIXME We need a JSC C API function for this.
    return m_originalGlobalObject.objectHasOwnProperty(object, property);
}

inline QVector<TiStringRef> QScriptEnginePrivate::objectGetOwnPropertyNames(TiObjectRef object) const
{
    // FIXME We can't use C API function TiObjectGetPropertyNames as it returns only enumerable properties.
    return m_originalGlobalObject.objectGetOwnPropertyNames(object);
}

#endif
