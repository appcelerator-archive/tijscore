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

#include "config.h"

#include "qscriptengine_p.h"

#include "qscriptfunction_p.h"
#include "qscriptprogram_p.h"
#include "qscriptvalue_p.h"

/*!
    Constructs a default QScriptEnginePrivate object, a new global context will be created.
    \internal
*/
QScriptEnginePrivate::QScriptEnginePrivate(const QScriptEngine* engine)
    : q_ptr(const_cast<QScriptEngine*>(engine))
    , m_context(TiGlobalContextCreate(0))
    , m_exception(0)
    , m_originalGlobalObject(m_context)
    , m_nativeFunctionClass(TiClassCreate(&qt_NativeFunctionClass))
    , m_nativeFunctionWithArgClass(TiClassCreate(&qt_NativeFunctionWithArgClass))
{
}

QScriptEnginePrivate::~QScriptEnginePrivate()
{
    TiClassRelease(m_nativeFunctionClass);
    TiClassRelease(m_nativeFunctionWithArgClass);
    if (m_exception)
        TiValueUnprotect(m_context, m_exception);
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
    return new QScriptValuePrivate(this, evaluate(*program, program->file(), program->line()));
}

QScriptValuePrivate* QScriptEnginePrivate::uncaughtException() const
{
    return m_exception ? new QScriptValuePrivate(this, m_exception) : new QScriptValuePrivate();
}

QScriptValuePrivate* QScriptEnginePrivate::newFunction(QScriptEngine::FunctionSignature fun, QScriptValuePrivate* prototype, int length)
{
    // Note that this private data will be deleted in the object finalize function.
    QNativeFunctionData* data = new QNativeFunctionData(this, fun);
    TiObjectRef funJS = TiObjectMake(m_context, m_nativeFunctionClass, reinterpret_cast<void*>(data));
    QScriptValuePrivate* proto = prototype ? prototype : newObject();
    return newFunction(funJS, proto);
}

QScriptValuePrivate* QScriptEnginePrivate::newFunction(QScriptEngine::FunctionWithArgSignature fun, void* arg)
{
    // Note that this private data will be deleted in the object finalize function.
    QNativeFunctionWithArgData* data = new QNativeFunctionWithArgData(this, fun, arg);
    TiObjectRef funJS = TiObjectMake(m_context, m_nativeFunctionWithArgClass, reinterpret_cast<void*>(data));
    QScriptValuePrivate* proto = newObject();
    return newFunction(funJS, proto);
}

QScriptValuePrivate* QScriptEnginePrivate::newFunction(TiObjectRef funJS, QScriptValuePrivate* prototype)
{
    TiObjectSetPrototype(m_context, funJS, m_originalGlobalObject.functionPrototype());

    QScriptValuePrivate* result = new QScriptValuePrivate(this, funJS);
    static TiStringRef protoName = QScriptConverter::toString("prototype");
    static TiStringRef constructorName = QScriptConverter::toString("constructor");
    result->setProperty(protoName, prototype, QScriptValue::Undeletable);
    prototype->setProperty(constructorName, result, QScriptValue::PropertyFlags(QScriptValue::Undeletable | QScriptValue::SkipInEnumeration));

    return result;
}

QScriptValuePrivate* QScriptEnginePrivate::newObject() const
{
    return new QScriptValuePrivate(this, TiObjectMake(m_context, /* jsClass */ 0, /* userData */ 0));
}

QScriptValuePrivate* QScriptEnginePrivate::newArray(uint length)
{
    TiValueRef exception = 0;
    TiObjectRef array = TiObjectMakeArray(m_context, /* argumentCount */ 0, /* arguments */ 0, &exception);

    if (!exception) {
        if (length > 0) {
            TiRetainPtr<TiStringRef> lengthRef(Adopt, TiStringCreateWithUTF8CString("length"));
            // array is an Array instance, so an exception should not occure here.
            TiObjectSetProperty(m_context, array, lengthRef.get(), TiValueMakeNumber(m_context, length), kTiPropertyAttributeNone, /* exception */ 0);
        }
    } else {
        setException(exception, NotNullException);
        return new QScriptValuePrivate();
    }

    return new QScriptValuePrivate(this, array);
}

QScriptValuePrivate* QScriptEnginePrivate::newDate(qsreal value)
{
    TiValueRef exception = 0;
    TiValueRef argument = TiValueMakeNumber(m_context, value);
    TiObjectRef result = TiObjectMakeDate(m_context, /* argumentCount */ 1, &argument, &exception);

    if (exception) {
        setException(exception, NotNullException);
        return new QScriptValuePrivate();
    }

    return new QScriptValuePrivate(this, result);
}

QScriptValuePrivate* QScriptEnginePrivate::globalObject() const
{
    TiObjectRef globalObject = TiContextGetGlobalObject(m_context);
    return new QScriptValuePrivate(this, globalObject);
}
