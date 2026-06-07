// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSMANAGEDVALUE_H
#define QJSMANAGEDVALUE_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qjsprimitivevalue.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
struct Value;
struct ExecutionEngine;
}

class QJSEngine;
class Q_QML_EXPORT QJSManagedValue
{
    Q_DISABLE_COPY(QJSManagedValue)
public:
    enum Type {
        Undefined,
        Boolean,
        Number,
        String,
        Object,
        Symbol,
        Function
    };

    QJSManagedValue() = default;
    QJSManagedValue(QJSValue value, QJSEngine *engine);
    QJSManagedValue(const QJSPrimitiveValue &value, QJSEngine *engine);
    QJSManagedValue(const QVariant &variant, QJSEngine *engine);
    QJSManagedValue(const QString &string, QJSEngine *engine);

    ~QJSManagedValue();
    QJSManagedValue(QJSManagedValue &&other);
    QJSManagedValue &operator=(QJSManagedValue &&other);

    bool equals(const QJSManagedValue &other) const;
    bool strictlyEquals(const QJSManagedValue &other) const;

    QJSEngine *engine() const;

    QJSManagedValue prototype() const;
    void setPrototype(const QJSManagedValue &prototype);

    Type type() const;

    // Compatibility with QJSValue
    bool isUndefined() const { return type() == Undefined; }
    bool isBoolean() const { return type() == Boolean; }
    bool isNumber() const { return type() == Number; }
    bool isString() const { return type() == String; }
    bool isObject() const { return type() == Object; }
    bool isSymbol() const { return type() == Symbol; }
    bool isFunction() const { return type() == Function; }

    // Special case of Number
    bool isInteger() const;

    // Selected special cases of Object
    bool isNull() const;
    bool isRegularExpression() const;
    bool isArray() const;
    bool isUrl() const;
    bool isVariant() const;
    bool isQObject() const;
    bool isQMetaObject() const;
    bool isDate() const;
    bool isError() const;
    bool isJsMetaType() const;

    // Native type transformations
    QString toString() const;
    double toNumber() const;
    bool toBoolean() const;

    // Variant-like type transformations
    QJSPrimitiveValue toPrimitive() const;
    QJSValue toJSValue() const;
    QVariant toVariant() const;

    // Special cases
    int toInteger() const;
    QRegularExpression toRegularExpression() const;
    QUrl toUrl() const;
    QObject *toQObject() const;
    const QMetaObject *toQMetaObject() const;
    QDateTime toDateTime() const;

    // Properties of objects
    bool hasProperty(const QString &name) const;
    bool hasOwnProperty(const QString &name) const;
    QJSValue property(const QString &name) const;
    void setProperty(const QString &name, const QJSValue &value);
    bool deleteProperty(const QString &name);

    //  ### Qt 7 use qsizetype instead.
    // Array indexing
    bool hasProperty(quint32 arrayIndex) const;
    bool hasOwnProperty(quint32 arrayIndex) const;
    QJSValue property(quint32 arrayIndex) const;
    void setProperty(quint32 arrayIndex, const QJSValue &value);
    bool deleteProperty(quint32 arrayIndex);

    // Calling functions
    QJSValue call(const QJSValueList &arguments = {}) const;
    QJSValue callWithInstance(const QJSValue &instance, const QJSValueList &arguments = {}) const;
    QJSValue callAsConstructor(const QJSValueList &arguments = {}) const;

    // JavaScript metatypes
    QJSManagedValue jsMetaType() const;
    QStringList jsMetaMembers() const;
    QJSManagedValue jsMetaInstantiate(const QJSValueList &values = {}) const;

private:
    friend class QJSValue;
    friend class QJSEngine;
    friend class QJSManagedValuePrivate;

    QJSManagedValue(QV4::ExecutionEngine *engine);
    QV4::Value *d = nullptr;
};

QT_END_NAMESPACE

#endif
