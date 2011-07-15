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

#include "config.h"

#include "qscriptengine_p.h"

#include "qscriptprogram_p.h"
#include "qscriptvalue_p.h"

/*!
    Constructs a default QScriptEnginePrivate object, a new global context will be created.
    \internal
*/
QScriptEnginePrivate::QScriptEnginePrivate(const QScriptEngine* engine)
    : q_ptr(const_cast<QScriptEngine*>(engine))
    , m_context(TiGlobalContextCreate(0))
{
}

QScriptEnginePrivate::~QScriptEnginePrivate()
{
    TiGlobalContextRelease(m_context);
}

QScriptSyntaxCheckResultPrivate* QScriptEnginePrivate::checkSyntax(const QString& program)
{
    TiValueRef exception;
    TiStringRef source = QScriptConverter::toString(program);
    bool syntaxIsCorrect = TiCheckScriptSyntax(m_context, source, /* url */ 0, /* starting line */ 1, &exception);
    TiStringRelease(source);
    if (syntaxIsCorrect) {
        return new QScriptSyntaxCheckResultPrivate(this);
    }
    TiValueProtect(m_context, exception);
    return new QScriptSyntaxCheckResultPrivate(this, const_cast<TiObjectRef>(exception));
}

/*!
    Evaluates program and returns the result of the evaluation.
    \internal
*/
QScriptValuePrivate* QScriptEnginePrivate::evaluate(const QString& program, const QString& fileName, int lineNumber)
{
    TiStringRef script = QScriptConverter::toString(program);
    TiStringRef file = QScriptConverter::toString(fileName);
    QScriptValuePrivate* result = new QScriptValuePrivate(this, evaluate(script, file, lineNumber));
    TiStringRelease(script);
    TiStringRelease(file);
    return result;
}

/*!
    Evaluates program and returns the result of the evaluation.
    \internal
*/
QScriptValuePrivate* QScriptEnginePrivate::evaluate(const QScriptProgramPrivate* program)
{
    if (program->isNull())
        return new QScriptValuePrivate;
    return new QScriptValuePrivate(this, evaluate(program->program(), program->file(), program->line()));
}

QScriptValuePrivate* QScriptEnginePrivate::globalObject() const
{
    TiObjectRef globalObject = TiContextGetGlobalObject(context());
    return new QScriptValuePrivate(this, globalObject, globalObject);
}
