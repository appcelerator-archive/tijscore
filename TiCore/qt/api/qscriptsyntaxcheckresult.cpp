/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
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

#include "config.h"

#include "qscriptsyntaxcheckresult.h"
#include "qscriptsyntaxcheckresult_p.h"

/*!
  \class QScriptSyntaxCheckResult

  \brief The QScriptSyntaxCheckResult class provides the result of a script syntax check.

  \ingroup script
  \mainclass

  QScriptSyntaxCheckResult is returned by QScriptEngine::checkSyntax() to
  provide information about the syntactical (in)correctness of a script.
*/

/*!
    \enum QScriptSyntaxCheckResult::State

    This enum specifies the state of a syntax check.

    \value Error The program contains a syntax error.
    \value Intermediate The program is incomplete.
    \value Valid The program is a syntactically correct Qt Script program.
*/

/*!
  Constructs a new QScriptSyntaxCheckResult from the \a other result.
*/
QScriptSyntaxCheckResult::QScriptSyntaxCheckResult(const QScriptSyntaxCheckResult& other)
    : d_ptr(other.d_ptr)
{}

/*!
  Constructs a new QScriptSyntaxCheckResult from an internal representation.
  \internal
*/
QScriptSyntaxCheckResult::QScriptSyntaxCheckResult(QScriptSyntaxCheckResultPrivate* d)
    : d_ptr(d)
{}

/*!
  Destroys this QScriptSyntaxCheckResult.
*/
QScriptSyntaxCheckResult::~QScriptSyntaxCheckResult()
{}

/*!
  Assigns the \a other result to this QScriptSyntaxCheckResult, and returns a
  reference to this QScriptSyntaxCheckResult.
*/
QScriptSyntaxCheckResult& QScriptSyntaxCheckResult::operator=(const QScriptSyntaxCheckResult& other)
{
    d_ptr = other.d_ptr;
    return *this;
}

/*!
  Returns the state of this QScriptSyntaxCheckResult.
*/
QScriptSyntaxCheckResult::State QScriptSyntaxCheckResult::state() const
{
    return d_ptr->state();
}

/*!
  Returns the error line number of this QScriptSyntaxCheckResult, or -1 if
  there is no error.

  \sa state(), errorMessage()
*/
int QScriptSyntaxCheckResult::errorLineNumber() const
{
    return d_ptr->errorLineNumber();
}

/*!
  Returns the error column number of this QScriptSyntaxCheckResult, or -1 if
  there is no error.

  \sa state(), errorLineNumber()
*/
int QScriptSyntaxCheckResult::errorColumnNumber() const
{
    return d_ptr->errorColumnNumber();
}

/*!
  Returns the error message of this QScriptSyntaxCheckResult, or an empty
  string if there is no error.

  \sa state(), errorLineNumber()
*/
QString QScriptSyntaxCheckResult::errorMessage() const
{
    return d_ptr->errorMessage();
}

QScriptSyntaxCheckResultPrivate::~QScriptSyntaxCheckResultPrivate()
{
    if (m_exception)
        TiValueUnprotect(m_engine->context(), m_exception);
}

QString QScriptSyntaxCheckResultPrivate::errorMessage() const
{
    if (!m_exception)
        return QString();

    TiStringRef tmp = TiValueToStringCopy(m_engine->context(), m_exception, /* exception */ 0);
    QString message = QScriptConverter::toString(tmp);
    TiStringRelease(tmp);
    return message;
}

int QScriptSyntaxCheckResultPrivate::errorLineNumber() const
{
    if (!m_exception)
        return -1;
    // m_exception is an instance of the Exception so it has "line" attribute.
    TiStringRef lineAttrName = QScriptConverter::toString("line");
    TiValueRef line = TiObjectGetProperty(m_engine->context(),
                                          m_exception,
                                          lineAttrName,
                                          /* exceptions */0);
    TiStringRelease(lineAttrName);
    return TiValueToNumber(m_engine->context(), line, /* exceptions */0);
}
