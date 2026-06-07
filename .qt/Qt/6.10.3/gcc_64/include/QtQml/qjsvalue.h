// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSVALUE_H
#define QJSVALUE_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

class QJSValue;
class QJSEngine;
class QVariant;
class QObject;
struct QMetaObject;
class QDateTime;
class QJSPrimitiveValue;

typedef QList<QJSValue> QJSValueList;
namespace QV4 {
    struct ExecutionEngine;
}

class QJSPrimitiveValue;
class QJSManagedValue;

class Q_QML_EXPORT QJSValue
{
public:
    enum SpecialValue {
        NullValue,
        UndefinedValue
    };

    enum ErrorType {
        NoError,
        GenericError,
        EvalError,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError,
        URIError
    };

    enum ObjectConversionBehavior {
        ConvertJSObjects,
        RetainJSObjects
    };

public:
    QJSValue(SpecialValue value = UndefinedValue);
    ~QJSValue();
    QJSValue(const QJSValue &other);

    inline QJSValue(QJSValue && other) : d(other.d) { other.d = 0; }
    inline QJSValue &operator=(QJSValue &&other)
    { std::swap(d, other.d); return *this; }

    QJSValue(bool value);
    QJSValue(int value);
    QJSValue(uint value);
    QJSValue(double value);
    QJSValue(const QString &value);
    QJSValue(const QLatin1String &value);
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN QJSValue(const char *str);
#endif

    QJSValue &operator=(const QJSValue &other);

    explicit QJSValue(QJSPrimitiveValue &&value);
    explicit QJSValue(QJSManagedValue &&value);

    bool isBool() const;
    bool isNumber() const;
    bool isNull() const;
    bool isString() const;
    bool isUndefined() const;
#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_VERSION_X_6_9("This might return unexpected results; consult documentation for more information")
    bool isVariant() const;
#endif
    bool isQObject() const;
    bool isQMetaObject() const;
    bool isObject() const;
    bool isDate() const;
    bool isRegExp() const;
    bool isArray() const;
    bool isError() const;
    bool isUrl() const;

    QString toString() const;
    double toNumber() const;
    qint32 toInt() const;
    quint32 toUInt() const;
    bool toBool() const;

    QVariant toVariant() const;
    QVariant toVariant(ObjectConversionBehavior behavior) const;
    QJSPrimitiveValue toPrimitive() const;

    QObject *toQObject() const;
    const QMetaObject *toQMetaObject() const;
    QDateTime toDateTime() const;

    bool equals(const QJSValue &other) const;
    bool strictlyEquals(const QJSValue &other) const;

    QJSValue prototype() const;
    void setPrototype(const QJSValue &prototype);

    QJSValue property(const QString &name) const;
    void setProperty(const QString &name, const QJSValue &value);

    bool hasProperty(const QString &name) const;
    bool hasOwnProperty(const QString &name) const;

    QJSValue property(quint32 arrayIndex) const;
    void setProperty(quint32 arrayIndex, const QJSValue &value);

    bool deleteProperty(const QString &name);

    bool isCallable() const;
    QJSValue call(const QJSValueList &args = QJSValueList()) const;
    QJSValue callWithInstance(const QJSValue &instance, const QJSValueList &args = QJSValueList()) const;
    QJSValue callAsConstructor(const QJSValueList &args = QJSValueList()) const;

    ErrorType errorType() const;

private:
    friend class QJSValuePrivate;
    // force compile error, prevent QJSValue(bool) to be called
    QJSValue(void *) = delete;

    quint64 d;
};

#ifndef QT_NO_DATASTREAM
Q_QML_EXPORT QDataStream &operator<<(QDataStream &, const QJSValue &);
Q_QML_EXPORT QDataStream &operator>>(QDataStream &, QJSValue &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QJSValue)

#endif
