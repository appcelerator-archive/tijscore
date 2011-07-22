/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef qscriptvalue_p_h
#define qscriptvalue_p_h

#include "qscriptconverter_p.h"
#include "qscriptengine_p.h"
#include "qscriptvalue.h"
#include <TiCore/Ti.h>
#include <TiCore/TiRetainPtr.h>
#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qvarlengtharray.h>

class QScriptEngine;
class QScriptValue;

/*
  \internal
  \class QScriptValuePrivate

  Implementation of QScriptValue.
  The implementation is based on a state machine. The states names are included in
  QScriptValuePrivate::State. Each method should check for the current state and then perform a
  correct action.

  State:
    Invalid -> QSVP is invalid, no assumptions should be made about class members (apart from m_value).
    CString -> QSVP is created from QString or const char* and no JSC engine has been associated yet.
        Current value is kept in m_string,
    CNumber -> QSVP is created from int, uint, double... and no JSC engine has been bind yet. Current
        value is kept in m_number
    CBool -> QSVP is created from bool and no JSC engine has been associated yet. Current value is kept
        in m_number
    CSpecial -> QSVP is Undefined or Null, but a JSC engine hasn't been associated yet, current value
        is kept in m_number (cast of QScriptValue::SpecialValue)
    TiValue -> QSVP is associated with engine, but there is no information about real type, the state
        have really short live cycle. Normally it is created as a function call result.
    JSPrimitive -> QSVP is associated with engine, and it is sure that it isn't a Ti object.
    TiObject -> QSVP is associated with engine, and it is sure that it is a Ti object.

  Each state keep all necessary information to invoke all methods, if not it should be changed to
  a proper state. Changed state shouldn't be reverted.
*/

class QScriptValuePrivate : public QSharedData {
public:
    inline static QScriptValuePrivate* get(const QScriptValue& q);
    inline static QScriptValue get(const QScriptValuePrivate* d);
    inline static QScriptValue get(QScriptValuePrivate* d);

    inline ~QScriptValuePrivate();

    inline QScriptValuePrivate();
    inline QScriptValuePrivate(const QString& string);
    inline QScriptValuePrivate(bool value);
    inline QScriptValuePrivate(int number);
    inline QScriptValuePrivate(uint number);
    inline QScriptValuePrivate(qsreal number);
    inline QScriptValuePrivate(QScriptValue::SpecialValue value);

    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, bool value);
    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, int value);
    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, uint value);
    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, qsreal value);
    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, const QString& value);
    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, QScriptValue::SpecialValue value);

    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, TiValueRef value);
    inline QScriptValuePrivate(const QScriptEnginePrivate* engine, TiValueRef value, TiObjectRef object);

    inline bool isValid() const;
    inline bool isBool();
    inline bool isNumber();
    inline bool isNull();
    inline bool isString();
    inline bool isUndefined();
    inline bool isError();
    inline bool isObject();
    inline bool isFunction();

    inline QString toString() const;
    inline qsreal toNumber() const;
    inline bool toBool() const;
    inline qsreal toInteger() const;
    inline qint32 toInt32() const;
    inline quint32 toUInt32() const;
    inline quint16 toUInt16() const;

    inline bool equals(QScriptValuePrivate* other);
    inline bool strictlyEquals(const QScriptValuePrivate* other) const;
    inline bool assignEngine(QScriptEnginePrivate* engine);

    inline QScriptValuePrivate* call(const QScriptValuePrivate* , const QScriptValueList& args);

    inline TiGlobalContextRef context() const;
    inline TiValueRef value() const;
    inline TiObjectRef object() const;
    inline QScriptEnginePrivate* engine() const;

private:
    // Please, update class documentation when you change the enum.
    enum State {
        Invalid = 0,
        CString = 0x1000,
        CNumber,
        CBool,
        CSpecial,
        TiValue = 0x2000, // JS values are equal or higher then this value.
        JSPrimitive,
        TiObject
    } m_state;
    QScriptEnginePtr m_engine;
    QString m_string;
    qsreal m_number;
    TiValueRef m_value;
    TiObjectRef m_object;

    inline void setValue(TiValueRef);

    inline bool inherits(const char*);
    inline State refinedTiValue();

    inline bool isTiBased() const;
    inline bool isNumberBased() const;
    inline bool isStringBased() const;
};

QScriptValuePrivate* QScriptValuePrivate::get(const QScriptValue& q) { return q.d_ptr.data(); }

QScriptValue QScriptValuePrivate::get(const QScriptValuePrivate* d)
{
    return QScriptValue(const_cast<QScriptValuePrivate*>(d));
}

QScriptValue QScriptValuePrivate::get(QScriptValuePrivate* d)
{
    return QScriptValue(d);
}

QScriptValuePrivate::~QScriptValuePrivate()
{
    if (m_value)
        TiValueUnprotect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate()
    : m_state(Invalid)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(const QString& string)
    : m_state(CString)
    , m_string(string)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(bool value)
    : m_state(CBool)
    , m_number(value)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(int number)
    : m_state(CNumber)
    , m_number(number)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(uint number)
    : m_state(CNumber)
    , m_number(number)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(qsreal number)
    : m_state(CNumber)
    , m_number(number)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(QScriptValue::SpecialValue value)
    : m_state(CSpecial)
    , m_number(value)
    , m_value(0)
{
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, bool value)
    : m_state(JSPrimitive)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(engine->makeTiValue(value))
{
    Q_ASSERT(engine);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, int value)
    : m_state(JSPrimitive)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(m_engine->makeTiValue(value))
{
    Q_ASSERT(engine);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, uint value)
    : m_state(JSPrimitive)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(m_engine->makeTiValue(value))
{
    Q_ASSERT(engine);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, qsreal value)
    : m_state(JSPrimitive)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(m_engine->makeTiValue(value))
{
    Q_ASSERT(engine);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, const QString& value)
    : m_state(JSPrimitive)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(m_engine->makeTiValue(value))
{
    Q_ASSERT(engine);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, QScriptValue::SpecialValue value)
    : m_state(JSPrimitive)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(m_engine->makeTiValue(value))
{
    Q_ASSERT(engine);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, TiValueRef value)
    : m_state(TiValue)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(value)
{
    Q_ASSERT(engine);
    Q_ASSERT(value);
    TiValueProtect(context(), m_value);
}

QScriptValuePrivate::QScriptValuePrivate(const QScriptEnginePrivate* engine, TiValueRef value, TiObjectRef object)
    : m_state(TiObject)
    , m_engine(const_cast<QScriptEnginePrivate*>(engine))
    , m_value(value)
    , m_object(object)
{
    Q_ASSERT(engine);
    Q_ASSERT(value);
    Q_ASSERT(object);
    TiValueProtect(context(), m_value);
}

bool QScriptValuePrivate::isValid() const { return m_state != Invalid; }

bool QScriptValuePrivate::isBool()
{
    switch (m_state) {
    case CBool:
        return true;
    case TiValue:
        if (refinedTiValue() != JSPrimitive)
            return false;
        // Fall-through.
    case JSPrimitive:
        return TiValueIsBoolean(context(), value());
    default:
        return false;
    }
}

bool QScriptValuePrivate::isNumber()
{
    switch (m_state) {
    case CNumber:
        return true;
    case TiValue:
        if (refinedTiValue() != JSPrimitive)
            return false;
        // Fall-through.
    case JSPrimitive:
        return TiValueIsNumber(context(), value());
    default:
        return false;
    }
}

bool QScriptValuePrivate::isNull()
{
    switch (m_state) {
    case CSpecial:
        return m_number == static_cast<int>(QScriptValue::NullValue);
    case TiValue:
        if (refinedTiValue() != JSPrimitive)
            return false;
        // Fall-through.
    case JSPrimitive:
        return TiValueIsNull(context(), value());
    default:
        return false;
    }
}

bool QScriptValuePrivate::isString()
{
    switch (m_state) {
    case CString:
        return true;
    case TiValue:
        if (refinedTiValue() != JSPrimitive)
            return false;
        // Fall-through.
    case JSPrimitive:
        return TiValueIsString(context(), value());
    default:
        return false;
    }
}

bool QScriptValuePrivate::isUndefined()
{
    switch (m_state) {
    case CSpecial:
        return m_number == static_cast<int>(QScriptValue::UndefinedValue);
    case TiValue:
        if (refinedTiValue() != JSPrimitive)
            return false;
        // Fall-through.
    case JSPrimitive:
        return TiValueIsUndefined(context(), value());
    default:
        return false;
    }
}

bool QScriptValuePrivate::isError()
{
    switch (m_state) {
    case TiValue:
        if (refinedTiValue() != TiObject)
            return false;
        // Fall-through.
    case TiObject:
        return inherits("Error");
    default:
        return false;
    }
}

bool QScriptValuePrivate::isObject()
{
    switch (m_state) {
    case TiValue:
        return refinedTiValue() == TiObject;
    case TiObject:
        return true;

    default:
        return false;
    }
}

bool QScriptValuePrivate::isFunction()
{
    switch (m_state) {
    case TiValue:
        if (refinedTiValue() != TiObject)
            return false;
        // Fall-through.
    case TiObject:
        return TiObjectIsFunction(context(), object());
    default:
        return false;
    }
}

QString QScriptValuePrivate::toString() const
{
    switch (m_state) {
    case Invalid:
        return QString();
    case CBool:
        return m_number ? QString::fromLatin1("true") : QString::fromLatin1("false");
    case CString:
        return m_string;
    case CNumber:
        return QScriptConverter::toString(m_number);
    case CSpecial:
        return m_number == QScriptValue::NullValue ? QString::fromLatin1("null") : QString::fromLatin1("undefined");
    case TiValue:
    case JSPrimitive:
    case TiObject:
        TiRetainPtr<TiStringRef> ptr(Adopt, TiValueToStringCopy(context(), value(), /* exception */ 0));
        return QScriptConverter::toString(ptr.get());
    }

    Q_ASSERT_X(false, "toString()", "Not all states are included in the previous switch statement.");
    return QString(); // Avoid compiler warning.
}

qsreal QScriptValuePrivate::toNumber() const
{
    switch (m_state) {
    case TiValue:
    case JSPrimitive:
    case TiObject:
        return TiValueToNumber(context(), value(), /* exception */ 0);
    case CNumber:
        return m_number;
    case CBool:
        return m_number ? 1 : 0;
    case Invalid:
        return 0;
    case CSpecial:
        return m_number == QScriptValue::NullValue ? 0 : qQNaN();
    case CString:
        bool ok;
        qsreal result = m_string.toDouble(&ok);
        if (ok)
            return result;
        result = m_string.toInt(&ok, 0); // Try other bases.
        if (ok)
            return result;
        if (m_string == "Infinity" || m_string == "-Infinity")
            return qInf();
        return m_string.length() ? qQNaN() : 0;
    }

    Q_ASSERT_X(false, "toNumber()", "Not all states are included in the previous switch statement.");
    return 0; // Avoid compiler warning.
}

bool QScriptValuePrivate::toBool() const
{
    switch (m_state) {
    case TiValue:
    case JSPrimitive:
        return TiValueToBoolean(context(), value());
    case TiObject:
        return true;
    case CNumber:
        return !(qIsNaN(m_number) || !m_number);
    case CBool:
        return m_number;
    case Invalid:
    case CSpecial:
        return false;
    case CString:
        return m_string.length();
    }

    Q_ASSERT_X(false, "toBool()", "Not all states are included in the previous switch statement.");
    return false; // Avoid compiler warning.
}

qsreal QScriptValuePrivate::toInteger() const
{
    qsreal result = toNumber();
    if (qIsNaN(result))
        return 0;
    if (qIsInf(result))
        return result;
    return (result > 0) ? qFloor(result) : -1 * qFloor(-result);
}

qint32 QScriptValuePrivate::toInt32() const
{
    qsreal result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint32 QScriptValuePrivate::toUInt32() const
{
    qsreal result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint16 QScriptValuePrivate::toUInt16() const
{
    return toInt32();
}


bool QScriptValuePrivate::equals(QScriptValuePrivate* other)
{
    if (!isValid() || !other->isValid())
        return false;

    if ((m_state == other->m_state) && !isTiBased()) {
        if (isNumberBased())
            return m_number == other->m_number;
        return m_string == other->m_string;
    }

    if (isTiBased() && !other->isTiBased()) {
        if (!other->assignEngine(engine())) {
            qWarning("equals(): Cannot compare to a value created in a different engine");
            return false;
        }
    } else if (!isTiBased() && other->isTiBased()) {
        if (!other->assignEngine(other->engine())) {
            qWarning("equals(): Cannot compare to a value created in a different engine");
            return false;
        }
    }

    return TiValueIsEqual(context(), value(), other->value(), /* exception */ 0);
}

bool QScriptValuePrivate::strictlyEquals(const QScriptValuePrivate* other) const
{
    if (m_state != other->m_state)
        return false;
    if (isTiBased()) {
        if (other->engine() != engine()) {
            qWarning("strictlyEquals(): Cannot compare to a value created in a different engine");
            return false;
        }
        return TiValueIsStrictEqual(context(), value(), other->value());
    }
    if (isStringBased())
        return m_string == other->m_string;
    if (isNumberBased())
        return m_number == other->m_number;

    return false; // Invalid state.
}

/*!
  Tries to assign \a engine to this value. Returns true on success; otherwise returns false.
*/
bool QScriptValuePrivate::assignEngine(QScriptEnginePrivate* engine)
{
    TiValueRef value;
    switch (m_state) {
    case CBool:
        value = engine->makeTiValue(static_cast<bool>(m_number));
        break;
    case CString:
        value = engine->makeTiValue(m_string);
        break;
    case CNumber:
        value = engine->makeTiValue(m_number);
        break;
    case CSpecial:
        value = engine->makeTiValue(static_cast<QScriptValue::SpecialValue>(m_number));
        break;
    default:
        if (!isTiBased())
            Q_ASSERT_X(!isTiBased(), "assignEngine()", "Not all states are included in the previous switch statement.");
        else
            qWarning("TiValue can't be rassigned to an another engine.");
        return false;
    }
    m_engine = engine;
    m_state = JSPrimitive;
    setValue(value);
    return true;
}

QScriptValuePrivate* QScriptValuePrivate::call(const QScriptValuePrivate*, const QScriptValueList& args)
{
    switch (m_state) {
    case TiValue:
        if (refinedTiValue() != TiObject)
            return new QScriptValuePrivate;
        // Fall-through.
    case TiObject:
        {
            // Convert all arguments and bind to the engine.
            int argc = args.size();
            QVarLengthArray<TiValueRef, 8> argv(argc);
            QScriptValueList::const_iterator i = args.constBegin();
            for (int j = 0; i != args.constEnd(); j++, i++) {
                QScriptValuePrivate* value = QScriptValuePrivate::get(*i);
                if (!value->assignEngine(engine())) {
                    qWarning("QScriptValue::call() failed: cannot call function with values created in a different engine");
                    return new QScriptValuePrivate;
                }
                argv[j] = value->value();
            }

            // Make the call
            TiValueRef exception = 0;
            TiValueRef result = TiObjectCallAsFunction(context(), object(), /* thisObject */ 0, argc, argv.constData(), &exception);
            if (!result && exception)
                return new QScriptValuePrivate(engine(), exception);
            if (result && !exception)
                return new QScriptValuePrivate(engine(), result);
        }
        // this QSV is not a function <-- !result && !exception. Fall-through.
    default:
        return new QScriptValuePrivate;
    }
}

QScriptEnginePrivate* QScriptValuePrivate::engine() const
{
    // As long as m_engine is an autoinitializated pointer we can safely return it without
    // checking current state.
    return m_engine.data();
}

TiGlobalContextRef QScriptValuePrivate::context() const
{
    Q_ASSERT(isTiBased());
    return m_engine->context();
}

TiValueRef QScriptValuePrivate::value() const
{
    Q_ASSERT(isTiBased());
    return m_value;
}

TiObjectRef QScriptValuePrivate::object() const
{
    Q_ASSERT(m_state == TiObject);
    return m_object;
}

void QScriptValuePrivate::setValue(TiValueRef value)
{
    if (m_value)
        TiValueUnprotect(context(), m_value);
    if (value)
        TiValueProtect(context(), value);
    m_value = value;
}

/*!
  \internal
  Returns true if QSV is created from constructor with the given \a name, it has to be a
  built-in type.
*/
bool QScriptValuePrivate::inherits(const char* name)
{
    Q_ASSERT(isTiBased());
    TiObjectRef globalObject = TiContextGetGlobalObject(context());
    TiStringRef errorAttrName = QScriptConverter::toString(name);
    TiValueRef error = TiObjectGetProperty(context(), globalObject, errorAttrName, /* exception */ 0);
    TiStringRelease(errorAttrName);
    return TiValueIsInstanceOfConstructor(context(), value(), TiValueToObject(context(), error, /* exception */ 0), /* exception */ 0);
}

/*!
  \internal
  Refines the state of this QScriptValuePrivate. Returns the new state.
*/
QScriptValuePrivate::State QScriptValuePrivate::refinedTiValue()
{
    Q_ASSERT(m_state == TiValue);
    if (!TiValueIsObject(context(), value())) {
        m_state = JSPrimitive;
    } else {
        m_state = TiObject;
        // We are sure that value is an TiObject, so we can const_cast safely without
        // calling JSC C API (TiValueToObject(context(), value(), /* exceptions */ 0)).
        m_object = const_cast<TiObjectRef>(m_value);
    }
    return m_state;
}

/*!
  \internal
  Returns true if QSV have an engine associated.
*/
bool QScriptValuePrivate::isTiBased() const { return m_state >= TiValue; }

/*!
  \internal
  Returns true if current value of QSV is placed in m_number.
*/
bool QScriptValuePrivate::isNumberBased() const { return !isTiBased() && !isStringBased() && m_state != Invalid; }

/*!
  \internal
  Returns true if current value of QSV is placed in m_string.
*/
bool QScriptValuePrivate::isStringBased() const { return m_state == CString; }

#endif // qscriptvalue_p_h
