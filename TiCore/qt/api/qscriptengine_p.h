/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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
#include "qscriptstring_p.h"
#include "qscriptsyntaxcheckresult_p.h"
#include "qscriptvalue.h"
#include <TiCore/Ti.h>
#include <TiBasePrivate.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

class QScriptEngine;
class QScriptSyntaxCheckResultPrivate;

class QScriptEnginePrivate : public QSharedData {
public:
    static QScriptEnginePrivate* get(const QScriptEngine* q) { Q_ASSERT(q); return q->d_ptr.data(); }
    static QScriptEngine* get(const QScriptEnginePrivate* d) { Q_ASSERT(d); return d->q_ptr; }

    QScriptEnginePrivate(const QScriptEngine*);
    ~QScriptEnginePrivate();

    QScriptSyntaxCheckResultPrivate* checkSyntax(const QString& program);
    QScriptValuePrivate* evaluate(const QString& program, const QString& fileName, int lineNumber);
    QScriptValuePrivate* evaluate(const QScriptProgramPrivate* program);
    inline TiValueRef evaluate(TiStringRef program, TiStringRef fileName, int lineNumber);

    inline void collectGarbage();
    inline void reportAdditionalMemoryCost(int cost);

    inline TiValueRef makeTiValue(double number) const;
    inline TiValueRef makeTiValue(int number) const;
    inline TiValueRef makeTiValue(uint number) const;
    inline TiValueRef makeTiValue(const QString& string) const;
    inline TiValueRef makeTiValue(bool number) const;
    inline TiValueRef makeTiValue(QScriptValue::SpecialValue value) const;

    QScriptValuePrivate* globalObject() const;

    inline QScriptStringPrivate* toStringHandle(const QString& str) const;

    inline TiGlobalContextRef context() const;
private:
    QScriptEngine* q_ptr;
    TiGlobalContextRef m_context;
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
    if (!result)
        return exception; // returns an exception
    return result;
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

TiGlobalContextRef QScriptEnginePrivate::context() const
{
    return m_context;
}

#endif
