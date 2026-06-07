// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QJSONVALUE_H
#define QJSONVALUE_H

#include <QtCore/qcborvalue.h>
#include <QtCore/qcompare.h>
#include <QtCore/qglobal.h>
#if (QT_VERSION < QT_VERSION_CHECK(7, 0, 0)) && !defined(QT_BOOTSTRAPPED)
#include <QtCore/qjsondocument.h>
#endif
#include <QtCore/qjsonparseerror.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QVariant;
class QJsonArray;
class QJsonObject;
class QCborContainerPrivate;

namespace QJsonPrivate {
class Value;
}

class Q_CORE_EXPORT QJsonValue
{
public:
    enum Type {
        Null =  0x0,
        Bool = 0x1,
        Double = 0x2,
        String = 0x3,
        Array = 0x4,
        Object = 0x5,
        Undefined = 0x80
    };

#if (QT_VERSION < QT_VERSION_CHECK(7, 0, 0)) && !defined(QT_BOOTSTRAPPED)
    using JsonFormat = QJsonDocument::JsonFormat;
#else
    enum class JsonFormat {
        Indented,
        Compact,
    };
#endif

    QJsonValue(Type = Null);
    QJsonValue(bool b);
    QJsonValue(double n);
    QJsonValue(int n);
    QJsonValue(qint64 v);
    QJsonValue(const QString &s);
    QJsonValue(QLatin1StringView s);
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN inline QJsonValue(const char *s)
        : QJsonValue(QString::fromUtf8(s)) {}
#endif
    QJsonValue(const QJsonArray &a);
    QJsonValue(QJsonArray &&a) noexcept;
    QJsonValue(const QJsonObject &o);
    QJsonValue(QJsonObject &&o) noexcept;

    ~QJsonValue();

    QJsonValue(const QJsonValue &other) noexcept;
    QJsonValue &operator =(const QJsonValue &other) noexcept;

    QJsonValue(QJsonValue &&other) noexcept;

    QJsonValue &operator =(QJsonValue &&other) noexcept
    {
        swap(other);
        return *this;
    }

    void swap(QJsonValue &other) noexcept;

    static QJsonValue fromVariant(const QVariant &variant);
    QVariant toVariant() const;

    static QJsonValue fromJson(QByteArrayView json, QJsonParseError *error = nullptr);

    QByteArray toJson(JsonFormat format = JsonFormat::Indented) const;

    Type type() const;
    inline bool isNull() const { return type() == Null; }
    inline bool isBool() const { return type() == Bool; }
    inline bool isDouble() const { return type() == Double; }
    inline bool isString() const { return type() == String; }
    inline bool isArray() const { return type() == Array; }
    inline bool isObject() const { return type() == Object; }
    inline bool isUndefined() const { return type() == Undefined; }

    bool toBool(bool defaultValue = false) const;
    int toInt(int defaultValue = 0) const;
    qint64 toInteger(qint64 defaultValue = 0) const;
    double toDouble(double defaultValue = 0) const;
    QString toString() const;
    QString toString(const QString &defaultValue) const;
    QAnyStringView toStringView(QAnyStringView defaultValue = {}) const;
    QJsonArray toArray() const;
    QJsonArray toArray(const QJsonArray &defaultValue) const;
    QJsonObject toObject() const;
    QJsonObject toObject(const QJsonObject &defaultValue) const;

    const QJsonValue operator[](const QString &key) const;
    const QJsonValue operator[](QStringView key) const;
    const QJsonValue operator[](QLatin1StringView key) const;
    const QJsonValue operator[](qsizetype i) const;

#if QT_CORE_REMOVED_SINCE(6, 8)
    bool operator==(const QJsonValue &other) const;
    bool operator!=(const QJsonValue &other) const;
#endif

private:
    friend Q_CORE_EXPORT bool comparesEqual(const QJsonValue &lhs,
                                            const QJsonValue &rhs);
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QJsonValue)

    // avoid implicit conversions from char * to bool
    QJsonValue(const void *) = delete;
    friend class QJsonPrivate::Value;
    friend class QJsonArray;
    friend class QJsonObject;
    friend class QCborValue;
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonValue &);
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QJsonValue &);

    QCborValue value;

    // Assert binary compatibility with pre-5.15 QJsonValue
    static_assert(sizeof(QExplicitlySharedDataPointer<QCborContainerPrivate>) == sizeof(void *));
    static_assert(sizeof(QCborValue::Type) == sizeof(QJsonValue::Type));
};

Q_DECLARE_SHARED(QJsonValue)

class QJsonValueConstRef
{
public:
    QJsonValueConstRef(const QJsonValueConstRef &) = default;
    QJsonValueConstRef &operator=(const QJsonValueConstRef &) = delete;
    inline operator QJsonValue() const { return concrete(*this); }

    Q_CORE_EXPORT QVariant toVariant() const;
    QJsonValue::Type type() const { return concreteType(*this); }
    bool isNull() const { return type() == QJsonValue::Null; }
    bool isBool() const { return type() == QJsonValue::Bool; }
    bool isDouble() const { return type() == QJsonValue::Double; }
    bool isString() const { return type() == QJsonValue::String; }
    bool isArray() const { return type() == QJsonValue::Array; }
    bool isObject() const { return type() == QJsonValue::Object; }
    bool isUndefined() const { return type() == QJsonValue::Undefined; }

    bool toBool(bool defaultValue = false) const
    { return concreteBool(*this, defaultValue); }
    int toInt(int defaultValue = 0) const
    { return int(concreteInt(*this, defaultValue, true)); }
    qint64 toInteger(qint64 defaultValue = 0) const
    { return concreteInt(*this, defaultValue, false); }
    double toDouble(double defaultValue = 0) const
    { return concreteDouble(*this, defaultValue); }
    QString toString(const QString &defaultValue = {}) const
    { return concreteString(*this, defaultValue); }
    QAnyStringView toStringView(QAnyStringView defaultValue = {}) const
    { return concreteStringView(*this, defaultValue); }
    Q_CORE_EXPORT QJsonArray toArray() const;
    Q_CORE_EXPORT QJsonObject toObject() const;

    const QJsonValue operator[](QStringView key) const { return concrete(*this)[key]; }
    const QJsonValue operator[](QLatin1StringView key) const { return concrete(*this)[key]; }
    const QJsonValue operator[](qsizetype i) const { return concrete(*this)[i]; }

protected:
    friend bool comparesEqual(const QJsonValueConstRef &lhs,
                              const QJsonValueConstRef &rhs)
    {
        return comparesEqual(concrete(lhs), concrete(rhs));
    }
    friend bool comparesEqual(const QJsonValueConstRef &lhs,
                              const QJsonValue &rhs)
    {
        return comparesEqual(concrete(lhs), rhs);
    }
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QJsonValueConstRef)
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QJsonValueConstRef, QJsonValue)

    Q_CORE_EXPORT static QJsonValue::Type
    concreteType(QJsonValueConstRef self) noexcept Q_DECL_PURE_FUNCTION;
    Q_CORE_EXPORT static bool
    concreteBool(QJsonValueConstRef self, bool defaultValue) noexcept Q_DECL_PURE_FUNCTION;
    Q_CORE_EXPORT static qint64
    concreteInt(QJsonValueConstRef self, qint64 defaultValue, bool clamp) noexcept Q_DECL_PURE_FUNCTION;
    Q_CORE_EXPORT static double
    concreteDouble(QJsonValueConstRef self, double defaultValue) noexcept Q_DECL_PURE_FUNCTION;
    Q_CORE_EXPORT static QString concreteString(QJsonValueConstRef self, const QString &defaultValue);
    Q_CORE_EXPORT static QAnyStringView concreteStringView(QJsonValueConstRef self, QAnyStringView defaultValue);
    Q_CORE_EXPORT static QJsonValue concrete(QJsonValueConstRef self) noexcept;

    // for iterators
    Q_CORE_EXPORT static QString objectKey(QJsonValueConstRef self);
    QString objectKey() const { return objectKey(*this); }

    Q_CORE_EXPORT static QAnyStringView objectKeyView(QJsonValueConstRef self);
    QAnyStringView objectKeyView() const { return objectKeyView(*this); }

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
    QJsonValueConstRef(QJsonArray *array, qsizetype idx)
        : a(array), is_object(false), index(static_cast<quint64>(idx)) {}
    QJsonValueConstRef(QJsonObject *object, qsizetype idx)
        : o(object), is_object(true), index(static_cast<quint64>(idx)) {}

    void rebind(QJsonValueConstRef other)
    {
        Q_ASSERT(is_object == other.is_object);
        if (is_object)
            o = other.o;
        else
            a = other.a;
        index = other.index;
    }

    union {
        QJsonArray *a;
        QJsonObject *o;
        void *d;
    };
    quint64 is_object : 1;
    quint64 index : 63;
#else
    constexpr QJsonValueConstRef(QCborContainerPrivate *d, size_t index, bool is_object)
        : d(d), is_object(is_object), index(index)
    {}

    // implemented in qjsonarray.h & qjsonobject.h, to get their d
    QJsonValueConstRef(QJsonArray *array, qsizetype idx);
    QJsonValueConstRef(QJsonObject *object, qsizetype idx);

    void rebind(QJsonValueConstRef other)
    {
        d = other.d;
        index = other.index;
    }

    QCborContainerPrivate *d = nullptr;
    size_t is_object : 1;
    size_t index : std::numeric_limits<size_t>::digits - 1;
#endif

    friend class QJsonArray;
    friend class QJsonObject;
    friend class QJsonPrivate::Value;
};

QT_WARNING_PUSH
QT6_ONLY(QT_WARNING_DISABLE_MSVC(4275)) // non dll-interface class 'QJsonValueConstRef' used as base for dll-interface class 'QJsonValueRef'
class QT6_ONLY(Q_CORE_EXPORT) QJsonValueRef : public QJsonValueConstRef
{
public:
    QJsonValueRef(const QJsonValueRef &) = default;
    QT7_ONLY(Q_CORE_EXPORT) QJsonValueRef &operator = (const QJsonValue &val);
    QT7_ONLY(Q_CORE_EXPORT) QJsonValueRef &operator = (const QJsonValueRef &val);

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
    // retained for binary compatibility (due to the Q_CORE_EXPORT) because at
    // least one compiler emits and exports all inlines in an exported class

    QJsonValueRef(QJsonArray *array, qsizetype idx)
        : QJsonValueConstRef(array, idx) {}
    QJsonValueRef(QJsonObject *object, qsizetype idx)
        : QJsonValueConstRef(object, idx) {}

    operator QJsonValue() const { return toValue(); }

    QVariant toVariant() const;
    inline QJsonValue::Type type() const { return QJsonValueConstRef::type(); }
    inline bool isNull() const { return type() == QJsonValue::Null; }
    inline bool isBool() const { return type() == QJsonValue::Bool; }
    inline bool isDouble() const { return type() == QJsonValue::Double; }
    inline bool isString() const { return type() == QJsonValue::String; }
    inline bool isArray() const { return type() == QJsonValue::Array; }
    inline bool isObject() const { return type() == QJsonValue::Object; }
    inline bool isUndefined() const { return type() == QJsonValue::Undefined; }

    inline bool toBool(bool defaultValue = false) const { return QJsonValueConstRef::toBool(defaultValue); }
    inline int toInt(int defaultValue = 0) const { return QJsonValueConstRef::toInt(defaultValue); }
    inline qint64 toInteger(qint64 defaultValue = 0) const { return QJsonValueConstRef::toInteger(defaultValue); }
    inline double toDouble(double defaultValue = 0) const { return QJsonValueConstRef::toDouble(defaultValue); }
    inline QString toString(const QString &defaultValue = {}) const { return QJsonValueConstRef::toString(defaultValue); }
    QAnyStringView toStringView(QAnyStringView defaultValue = {}) const
    { return QJsonValueConstRef::toStringView(defaultValue); }
    QJsonArray toArray() const;
    QJsonObject toObject() const;

    const QJsonValue operator[](QStringView key) const { return QJsonValueConstRef::operator[](key); }
    const QJsonValue operator[](QLatin1StringView key) const { return QJsonValueConstRef::operator[](key); }
    const QJsonValue operator[](qsizetype i) const { return QJsonValueConstRef::operator[](i); }

#if QT_CORE_REMOVED_SINCE(6, 8)
    inline bool operator==(const QJsonValue &other) const { return comparesEqual(*this, other); }
    inline bool operator!=(const QJsonValue &other) const { return !comparesEqual(*this, other); }
#endif

private:
    friend bool comparesEqual(const QJsonValueRef &lhs, const QJsonValueRef &rhs)
    {
        return comparesEqual(QJsonValue(lhs), QJsonValue(rhs));
    }
    friend bool comparesEqual(const QJsonValueRef &lhs, const QJsonValueConstRef &rhs)
    {
        return comparesEqual(QJsonValue(lhs), QJsonValue(rhs));
    }
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QJsonValueRef)
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QJsonValueRef, QJsonValueConstRef)

    QJsonValue toValue() const;
#else
    using QJsonValueConstRef::operator[];
    Q_CORE_EXPORT QJsonValueRef operator[](QAnyStringView key);
    Q_CORE_EXPORT QJsonValueRef operator[](qsizetype i);

private:
    using QJsonValueConstRef::QJsonValueConstRef;
#endif // < Qt 7

    QT7_ONLY(Q_CORE_EXPORT) void detach();
    friend class QJsonArray;
    friend class QJsonObject;
};
QT_WARNING_POP

inline QJsonValue QCborValueConstRef::toJsonValue() const
{
    return concrete().toJsonValue();
}

Q_CORE_EXPORT size_t qHash(const QJsonValue &value, size_t seed = 0);

#if !defined(QT_NO_DEBUG_STREAM)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonValue &);
#endif

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QJsonValue &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QJsonValue &);
#endif

QT_END_NAMESPACE

#endif // QJSONVALUE_H
