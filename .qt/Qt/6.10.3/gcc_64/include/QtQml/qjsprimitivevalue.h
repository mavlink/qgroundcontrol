// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSPRIMITIVEVALUE_H
#define QJSPRIMITIVEVALUE_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qjsnumbercoercion.h>

#include <QtCore/qstring.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qvariant.h>

#include <variant>
#include <cmath>

QT_BEGIN_NAMESPACE

namespace QV4 { struct ExecutionEngine; }

struct QJSPrimitiveUndefined {};
struct QJSPrimitiveNull {};

class QJSPrimitiveValue
{
    template<typename Concrete>
    struct StringNaNOperators
    {
        static constexpr double op(const QString &, QJSPrimitiveUndefined)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        static constexpr double op(QJSPrimitiveUndefined, const QString &)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        static double op(const QString &lhs, QJSPrimitiveNull) { return op(lhs, 0); }
        static double op(QJSPrimitiveNull, const QString &rhs) { return op(0, rhs); }

        template<typename T>
        static double op(const QString &lhs, T rhs)
        {
            return Concrete::op(numberFromString(lhs).toDouble(), rhs);
        }

        template<typename T>
        static double op(T lhs, const QString &rhs)
        {
            return Concrete::op(lhs, numberFromString(rhs).toDouble());
        }

        static double op(const QString &lhs, const QString &rhs)
        {
            return Concrete::op(numberFromString(lhs).toDouble(), numberFromString(rhs).toDouble());
        }
    };

    struct AddOperators {
        static constexpr double op(double lhs, double rhs) { return lhs + rhs; }
        static bool opOverflow(int lhs, int rhs, int *result)
        {
            return qAddOverflow(lhs, rhs, result);
        }

        template<typename T>
        static QString op(const QString &lhs, T rhs)
        {
            return lhs + QJSPrimitiveValue(rhs).toString();
        }

        template<typename T>
        static QString op(T lhs, const QString &rhs)
        {
            return QJSPrimitiveValue(lhs).toString() + rhs;
        }

        static QString op(const QString &lhs, const QString &rhs) { return lhs + rhs; }
    };

    struct SubOperators : private StringNaNOperators<SubOperators> {
        static constexpr double op(double lhs, double rhs) { return lhs - rhs; }
        static bool opOverflow(int lhs, int rhs, int *result)
        {
            return qSubOverflow(lhs, rhs, result);
        }

        using StringNaNOperators::op;
    };

    struct MulOperators : private StringNaNOperators<MulOperators> {
        static constexpr double op(double lhs, double rhs) { return lhs * rhs; }
        static bool opOverflow(int lhs, int rhs, int *result)
        {
            // compare mul_int32 in qv4math_p.h
            auto hadOverflow = qMulOverflow(lhs, rhs, result);
            if (((lhs < 0) ^ (rhs < 0)) && (*result == 0))
                return true; // result must be negative 0, does not fit into int
            return hadOverflow;
        }

        using StringNaNOperators::op;
    };

    struct DivOperators : private StringNaNOperators<DivOperators> {
        static constexpr double op(double lhs, double rhs)  {
            // Without is_iec559, we don't get proper JS semantics
#ifndef Q_OS_INTEGRITY
            static_assert(std::numeric_limits<double>::is_iec559);
#endif
            QT_WARNING_PUSH
            // divide by zero: not an issue with iec559
            QT_WARNING_DISABLE_MSVC(4723)
            return lhs / rhs;
            QT_WARNING_POP
        }
        static constexpr bool opOverflow(int, int, int *)
        {
            return true;
        }

        using StringNaNOperators::op;
    };

public:
    enum Type : quint8 {
        Undefined,
        Null,
        Boolean,
        Integer,
        Double,
        String
    };

    constexpr Type type() const { return Type(d.type()); }

    // Prevent casting from Type to int
    QJSPrimitiveValue(Type) = delete;

    Q_IMPLICIT constexpr QJSPrimitiveValue() noexcept = default;
    Q_IMPLICIT constexpr QJSPrimitiveValue(QJSPrimitiveUndefined undefined) noexcept : d(undefined) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(QJSPrimitiveNull null) noexcept : d(null) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(bool value) noexcept : d(value) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(int value) noexcept : d(value) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(double value) noexcept : d(value) {}
    Q_IMPLICIT QJSPrimitiveValue(QString string) noexcept : d(std::move(string)) {}

    explicit QJSPrimitiveValue(const QMetaType type, const void *value) noexcept
    {
        switch (type.id()) {
        case QMetaType::UnknownType:
            d = QJSPrimitiveUndefined();
            break;
        case QMetaType::Nullptr:
            d = QJSPrimitiveNull();
            break;
        case QMetaType::Bool:
            d = *static_cast<const bool *>(value);
            break;
        case QMetaType::Int:
            d = *static_cast<const int *>(value);
            break;
        case QMetaType::Double:
            d = *static_cast<const double *>(value);
            break;
        case QMetaType::QString:
            d = *static_cast<const QString *>(value);
            break;
        default:
            // Unsupported. Remains undefined.
            break;
        }
    }

    explicit QJSPrimitiveValue(QMetaType type) noexcept
    {
        switch (type.id()) {
        case QMetaType::UnknownType:
            d = QJSPrimitiveUndefined();
            break;
        case QMetaType::Nullptr:
            d = QJSPrimitiveNull();
            break;
        case QMetaType::Bool:
            d = false;
            break;
        case QMetaType::Int:
            d = 0;
            break;
        case QMetaType::Double:
            d = 0.0;
            break;
        case QMetaType::QString:
            d = QString();
            break;
        default:
            // Unsupported. Remains undefined.
            break;
        }
    }

    explicit QJSPrimitiveValue(const QVariant &variant) noexcept
        : QJSPrimitiveValue(variant.metaType(), variant.data())
    {
    }

    constexpr QMetaType metaType() const  { return d.metaType(); }
    constexpr void *data() { return d.data(); }
    constexpr const void *data() const { return d.data(); }
    constexpr const void *constData() const { return d.data(); }

    template<Type type>
    QJSPrimitiveValue to() const {
        if constexpr (type == Undefined)
            return QJSPrimitiveUndefined();
        if constexpr (type == Null)
            return QJSPrimitiveNull();
        if constexpr (type == Boolean)
            return toBoolean();
        if constexpr (type == Integer)
            return toInteger();
        if constexpr (type == Double)
            return toDouble();
        if constexpr (type == String)
            return toString();

        Q_UNREACHABLE_RETURN(QJSPrimitiveUndefined());
    }

    constexpr bool toBoolean() const
    {
        switch (type()) {
        case Undefined: return false;
        case Null:      return false;
        case Boolean:   return asBoolean();
        case Integer:   return asInteger() != 0;
        case Double: {
            const double v = asDouble();
            return !QJSNumberCoercion::equals(v, 0) && !std::isnan(v);
        }
        case String:    return !asString().isEmpty();
        }

        // GCC 8.x does not treat __builtin_unreachable() as constexpr
    #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        Q_UNREACHABLE_RETURN(false);
    #else
        return false;
    #endif
    }

    constexpr int toInteger() const
    {
        switch (type()) {
        case Undefined: return 0;
        case Null:      return 0;
        case Boolean:   return asBoolean();
        case Integer:   return asInteger();
        case Double:    return QJSNumberCoercion::toInteger(asDouble());
        case String:    return numberFromString(asString()).toInteger();
        }

        // GCC 8.x does not treat __builtin_unreachable() as constexpr
    #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        Q_UNREACHABLE_RETURN(0);
    #else
        return 0;
    #endif
    }

    constexpr double toDouble() const
    {
        switch (type()) {
        case Undefined: return std::numeric_limits<double>::quiet_NaN();
        case Null:      return 0;
        case Boolean:   return asBoolean();
        case Integer:   return asInteger();
        case Double:    return asDouble();
        case String:    return numberFromString(asString()).toDouble();
        }

        // GCC 8.x does not treat __builtin_unreachable() as constexpr
    #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        Q_UNREACHABLE_RETURN({});
    #else
        return {};
    #endif
    }

    QString toString() const
    {
        switch (type()) {
        case Undefined: return QStringLiteral("undefined");
        case Null:      return QStringLiteral("null");
        case Boolean:   return asBoolean() ? QStringLiteral("true") : QStringLiteral("false");
        case Integer:   return QString::number(asInteger());
        case Double: {
            const double result = asDouble();
            if (std::isnan(result))
                return QStringLiteral("NaN");
            if (std::isfinite(result))
                return toString(result);
            if (result > 0)
                return QStringLiteral("Infinity");
            return QStringLiteral("-Infinity");
        }
        case String: return asString();
        }

        Q_UNREACHABLE_RETURN(QString());
    }

    QVariant toVariant() const
    {
        switch (type()) {
        case Undefined: return QVariant();
        case Null:      return QVariant::fromValue<std::nullptr_t>(nullptr);
        case Boolean:   return QVariant(asBoolean());
        case Integer:   return QVariant(asInteger());
        case Double:    return QVariant(asDouble());
        case String:    return QVariant(asString());
        }

        Q_UNREACHABLE_RETURN(QVariant());
    }

    friend inline QJSPrimitiveValue operator+(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<AddOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator-(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<SubOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator*(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<MulOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator/(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<DivOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator%(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        switch (lhs.type()) {
        case Null:
        case Boolean:
        case Integer:
            switch (rhs.type()) {
            case Boolean:
            case Integer: {
                const int leftInt = lhs.toInteger();
                const int rightInt = rhs.toInteger();
                if (leftInt >= 0 && rightInt > 0)
                    return leftInt % rightInt;
                Q_FALLTHROUGH();
            }
            case Undefined:
            case Null:
            case Double:
            case String:
                break;
            }
            Q_FALLTHROUGH();
        case Undefined:
        case Double:
        case String:
            break;
        }

        return std::fmod(lhs.toDouble(), rhs.toDouble());
    }

    QJSPrimitiveValue &operator++()
    {
        // ++a is modeled as a -= (-1) to avoid the potential string concatenation
        return (*this = operate<SubOperators>(*this, -1));
    }

    QJSPrimitiveValue operator++(int)
    {
        // a++ is modeled as a -= (-1) to avoid the potential string concatenation
        QJSPrimitiveValue other = operate<SubOperators>(*this, -1);
        std::swap(other, *this);
        return +other; // We still need to coerce the original value.
    }

    QJSPrimitiveValue &operator--()
    {
        return (*this = operate<SubOperators>(*this, 1));
    }

    QJSPrimitiveValue operator--(int)
    {
        QJSPrimitiveValue other = operate<SubOperators>(*this, 1);
        std::swap(other, *this);
        return +other; // We still need to coerce the original value.
    }

    QJSPrimitiveValue operator+()
    {
        // +a is modeled as a -= 0. That should force it to number.
        return (*this = operate<SubOperators>(*this, 0));
    }

    QJSPrimitiveValue operator-()
    {
        return (*this = operate<MulOperators>(*this, -1));
    }

    constexpr bool strictlyEquals(const QJSPrimitiveValue &other) const
    {
        const Type myType = type();
        const Type otherType = other.type();

        if (myType != otherType) {
            // int -> double promotion is OK in strict mode
            if (myType == Double && otherType == Integer)
                return strictlyEquals(double(other.asInteger()));
            if (myType == Integer && otherType == Double)
                return QJSPrimitiveValue(double(asInteger())).strictlyEquals(other);
            return false;
        }

        switch (myType) {
        case Undefined:
        case Null:
            return true;
        case Boolean:
            return asBoolean() == other.asBoolean();
        case Integer:
            return asInteger() == other.asInteger();
        case Double: {
            const double l = asDouble();
            const double r = other.asDouble();
            if (std::isnan(l) || std::isnan(r))
                return false;
            if (qIsNull(l) && qIsNull(r))
                return true;
            return QJSNumberCoercion::equals(l, r);
        }
        case String:
            return asString() == other.asString();
        }

        return false;
    }

    // Loose operator==, in contrast to strict ===
    constexpr bool equals(const QJSPrimitiveValue &other) const
    {
        const Type myType = type();
        const Type otherType = other.type();

        if (myType == otherType)
            return strictlyEquals(other);

        switch (myType) {
        case Undefined:
            return otherType == Null;
        case Null:
            return otherType == Undefined;
        case Boolean:
            return QJSPrimitiveValue(int(asBoolean())).equals(other);
        case Integer:
            // prefer rhs bool -> int promotion over promoting both to double
            return otherType == Boolean
                    ? QJSPrimitiveValue(asInteger()).equals(int(other.asBoolean()))
                    : QJSPrimitiveValue(double(asInteger())).equals(other);
        case Double:
            // Promote the other side to double (or recognize lhs as undefined/null)
            return other.equals(*this);
        case String:
            return numberFromString(asString()).parsedEquals(other);
        }

        return false;
    }

    friend constexpr inline bool operator==(const QJSPrimitiveValue &lhs, const
                                            QJSPrimitiveValue &rhs)
    {
        return lhs.strictlyEquals(rhs);
    }

    friend constexpr inline bool operator!=(const QJSPrimitiveValue &lhs,
                                            const QJSPrimitiveValue &rhs)
    {
        return !lhs.strictlyEquals(rhs);
    }

    friend constexpr inline bool operator<(const QJSPrimitiveValue &lhs,
                                           const QJSPrimitiveValue &rhs)
    {
        switch (lhs.type()) {
        case Undefined:
            return false;
        case Null: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return false;
            case Boolean:   return 0 < int(rhs.asBoolean());
            case Integer:   return 0 < rhs.asInteger();
            case Double:    return double(0) < rhs.asDouble();
            case String:    return double(0) < rhs.toDouble();
            }
            break;
        }
        case Boolean: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return int(lhs.asBoolean()) < 0;
            case Boolean:   return lhs.asBoolean() < rhs.asBoolean();
            case Integer:   return int(lhs.asBoolean()) < rhs.asInteger();
            case Double:    return double(lhs.asBoolean()) < rhs.asDouble();
            case String:    return double(lhs.asBoolean()) < rhs.toDouble();
            }
            break;
        }
        case Integer: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return lhs.asInteger() < 0;
            case Boolean:   return lhs.asInteger() < int(rhs.asBoolean());
            case Integer:   return lhs.asInteger() < rhs.asInteger();
            case Double:    return double(lhs.asInteger()) < rhs.asDouble();
            case String:    return double(lhs.asInteger()) < rhs.toDouble();
            }
            break;
        }
        case Double: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return lhs.asDouble() < double(0);
            case Boolean:   return lhs.asDouble() < double(rhs.asBoolean());
            case Integer:   return lhs.asDouble() < double(rhs.asInteger());
            case Double:    return lhs.asDouble() < rhs.asDouble();
            case String:    return lhs.asDouble() < rhs.toDouble();
            }
            break;
        }
        case String: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return lhs.toDouble() < double(0);
            case Boolean:   return lhs.toDouble() < double(rhs.asBoolean());
            case Integer:   return lhs.toDouble() < double(rhs.asInteger());
            case Double:    return lhs.toDouble() < rhs.asDouble();
            case String:    return lhs.asString() < rhs.asString();
            }
            break;
        }
        }

        return false;
    }

    friend constexpr inline bool operator>(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        return rhs < lhs;
    }

    friend constexpr inline bool operator<=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        if (lhs.type() == String) {
           if (rhs.type() == String)
               return lhs.asString() <= rhs.asString();
           else
               return numberFromString(lhs.asString()) <= rhs;
        }
        if (rhs.type() == String)
            return lhs <= numberFromString(rhs.asString());

        if (lhs.isNanOrUndefined() || rhs.isNanOrUndefined())
            return false;
        return !(lhs > rhs);
    }

    friend constexpr inline bool operator>=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        if (lhs.type() == String) {
           if (rhs.type() == String)
               return lhs.asString() >= rhs.asString();
           else
               return numberFromString(lhs.asString()) >= rhs;
        }
        if (rhs.type() == String)
            return lhs >= numberFromString(rhs.asString());

        if (lhs.isNanOrUndefined() || rhs.isNanOrUndefined())
            return false;
        return !(lhs < rhs);
    }

private:
    friend class QJSManagedValue;
    friend class QJSValue;
    friend struct QV4::ExecutionEngine;

    constexpr bool asBoolean() const { return d.getBool(); }
    constexpr int asInteger() const { return d.getInt(); }
    constexpr double asDouble() const { return d.getDouble(); }
    QString asString() const { return d.getString(); }

    constexpr bool parsedEquals(const QJSPrimitiveValue &other) const
    {
        return type() != Undefined && equals(other);
    }

    static QJSPrimitiveValue numberFromString(const QString &string)
    {
        bool ok;
        const int intValue = string.toInt(&ok);
        if (ok)
            return intValue;

        const double doubleValue = string.toDouble(&ok);
        if (ok)
            return doubleValue;
        if (string.isEmpty())
            return 0;
        if (string == QStringLiteral("Infinity"))
            return std::numeric_limits<double>::infinity();
        if (string == QStringLiteral("-Infinity"))
            return -std::numeric_limits<double>::infinity();
        if (string == QStringLiteral("NaN"))
            return std::numeric_limits<double>::quiet_NaN();
        return QJSPrimitiveUndefined();
    }

    static Q_QML_EXPORT QString toString(double d);

    template<typename Operators, typename Lhs, typename Rhs>
    static QJSPrimitiveValue operateOnIntegers(const QJSPrimitiveValue &lhs,
                                               const QJSPrimitiveValue &rhs)
    {
        int result;
        if (Operators::opOverflow(lhs.d.get<Lhs>(), rhs.d.get<Rhs>(), &result))
            return Operators::op(lhs.d.get<Lhs>(), rhs.d.get<Rhs>());
        return result;
    }

    template<typename Operators>
    static QJSPrimitiveValue operate(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        switch (lhs.type()) {
        case Undefined:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return std::numeric_limits<double>::quiet_NaN();
            case Boolean:   return std::numeric_limits<double>::quiet_NaN();
            case Integer:   return std::numeric_limits<double>::quiet_NaN();
            case Double:    return std::numeric_limits<double>::quiet_NaN();
            case String:    return Operators::op(QJSPrimitiveUndefined(), rhs.asString());
            }
            break;
        case Null:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return operateOnIntegers<Operators, int, int>(0, 0);
            case Boolean:   return operateOnIntegers<Operators, int, bool>(0, rhs);
            case Integer:   return operateOnIntegers<Operators, int, int>(0, rhs);
            case Double:    return Operators::op(0, rhs.asDouble());
            case String:    return Operators::op(QJSPrimitiveNull(), rhs.asString());
            }
            break;
        case Boolean:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return operateOnIntegers<Operators, bool, int>(lhs, 0);
            case Boolean:   return operateOnIntegers<Operators, bool, bool>(lhs, rhs);
            case Integer:   return operateOnIntegers<Operators, bool, int>(lhs, rhs);
            case Double:    return Operators::op(lhs.asBoolean(), rhs.asDouble());
            case String:    return Operators::op(lhs.asBoolean(), rhs.asString());
            }
            break;
        case Integer:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return operateOnIntegers<Operators, int, int>(lhs, 0);
            case Boolean:   return operateOnIntegers<Operators, int, bool>(lhs, rhs);
            case Integer:   return operateOnIntegers<Operators, int, int>(lhs, rhs);
            case Double:    return Operators::op(lhs.asInteger(), rhs.asDouble());
            case String:    return Operators::op(lhs.asInteger(), rhs.asString());
            }
            break;
        case Double:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return Operators::op(lhs.asDouble(), 0);
            case Boolean:   return Operators::op(lhs.asDouble(), rhs.asBoolean());
            case Integer:   return Operators::op(lhs.asDouble(), rhs.asInteger());
            case Double:    return Operators::op(lhs.asDouble(), rhs.asDouble());
            case String:    return Operators::op(lhs.asDouble(), rhs.asString());
            }
            break;
        case String:
            switch (rhs.type()) {
            case Undefined: return Operators::op(lhs.asString(), QJSPrimitiveUndefined());
            case Null:      return Operators::op(lhs.asString(), QJSPrimitiveNull());
            case Boolean:   return Operators::op(lhs.asString(), rhs.asBoolean());
            case Integer:   return Operators::op(lhs.asString(), rhs.asInteger());
            case Double:    return Operators::op(lhs.asString(), rhs.asDouble());
            case String:    return Operators::op(lhs.asString(), rhs.asString());
            }
            break;
        }

        Q_UNREACHABLE_RETURN(QJSPrimitiveUndefined());
    }

    constexpr bool isNanOrUndefined() const
    {
        switch (type()) {
        case Undefined: return true;
        case Null:      return false;
        case Boolean:   return false;
        case Integer:   return false;
        case Double:    return std::isnan(asDouble());
        case String:    return false;
        }
        // GCC 8.x does not treat __builtin_unreachable() as constexpr
    #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        Q_UNREACHABLE_RETURN(false);
    #else
        return false;
    #endif
    }

    struct QJSPrimitiveValuePrivate
    {
        // Can't be default because QString has a non-trivial ctor.
        constexpr QJSPrimitiveValuePrivate() noexcept {}

        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(QJSPrimitiveUndefined) noexcept  {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(QJSPrimitiveNull) noexcept
            : m_type(Null) {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(bool b) noexcept
            : m_bool(b), m_type(Boolean) {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(int i) noexcept
            : m_int(i), m_type(Integer) {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(double d) noexcept
            : m_double(d), m_type(Double) {}
        Q_IMPLICIT QJSPrimitiveValuePrivate(QString s) noexcept
            : m_string(std::move(s)), m_type(String) {}

        constexpr QJSPrimitiveValuePrivate(const QJSPrimitiveValuePrivate &other) noexcept
            : m_type(other.m_type)
        {
            // Not copy-and-swap since swap() would be much more complicated.
            if (!assignSimple(other))
                new (&m_string) QString(other.m_string);
        }

        constexpr QJSPrimitiveValuePrivate(QJSPrimitiveValuePrivate &&other) noexcept
            : m_type(other.m_type)
        {
            // Not move-and-swap since swap() would be much more complicated.
            if (!assignSimple(other))
                new (&m_string) QString(std::move(other.m_string));
        }

        constexpr QJSPrimitiveValuePrivate &operator=(const QJSPrimitiveValuePrivate &other) noexcept
        {
            if (this == &other)
                return *this;

            if (m_type == String) {
                if (other.m_type == String) {
                    m_type = other.m_type;
                    m_string = other.m_string;
                    return *this;
                }
                m_string.~QString();
            }

            m_type = other.m_type;
            if (!assignSimple(other))
                new (&m_string) QString(other.m_string);
            return *this;
        }

        constexpr QJSPrimitiveValuePrivate &operator=(QJSPrimitiveValuePrivate &&other) noexcept
        {
            if (this == &other)
                return *this;

            if (m_type == String) {
                if (other.m_type == String) {
                    m_type = other.m_type;
                    m_string = std::move(other.m_string);
                    return *this;
                }
                m_string.~QString();
            }

            m_type = other.m_type;
            if (!assignSimple(other))
                new (&m_string) QString(std::move(other.m_string));
            return *this;
        }

        ~QJSPrimitiveValuePrivate()
        {
            if (m_type == String)
                m_string.~QString();
        }

        constexpr Type type() const noexcept { return m_type; }
        constexpr bool getBool() const noexcept { return m_bool; }
        constexpr int getInt() const noexcept { return m_int; }
        constexpr double getDouble() const noexcept { return m_double; }
        QString getString() const noexcept { return m_string; }

        template<typename T>
        constexpr T get() const noexcept {
            if constexpr (std::is_same_v<T, QJSPrimitiveUndefined>)
                return QJSPrimitiveUndefined();
            else if constexpr (std::is_same_v<T, QJSPrimitiveNull>)
                return QJSPrimitiveNull();
            else if constexpr (std::is_same_v<T, bool>)
                return getBool();
            else if constexpr (std::is_same_v<T, int>)
                return getInt();
            else if constexpr (std::is_same_v<T, double>)
                return getDouble();
            else if constexpr (std::is_same_v<T, QString>)
                return getString();

            // GCC 8.x does not treat __builtin_unreachable() as constexpr
        #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
            Q_UNREACHABLE_RETURN(T());
        #else
            return T();
        #endif
        }

        constexpr QMetaType metaType() const noexcept {
            switch (m_type) {
            case Undefined:
                return QMetaType();
            case Null:
                return QMetaType::fromType<std::nullptr_t>();
            case Boolean:
                return QMetaType::fromType<bool>();
            case Integer:
                return QMetaType::fromType<int>();
            case Double:
                return QMetaType::fromType<double>();
            case String:
                return QMetaType::fromType<QString>();
            }

            // GCC 8.x does not treat __builtin_unreachable() as constexpr
        #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
            Q_UNREACHABLE_RETURN(QMetaType());
        #else
            return QMetaType();
        #endif
        }

        constexpr void *data() noexcept {
            switch (m_type) {
            case Undefined:
            case Null:
                return nullptr;
            case Boolean:
                return &m_bool;
            case Integer:
                return &m_int;
            case Double:
                return &m_double;
            case String:
                return &m_string;
            }

            // GCC 8.x does not treat __builtin_unreachable() as constexpr
        #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
            Q_UNREACHABLE_RETURN(nullptr);
        #else
            return nullptr;
        #endif
        }

        constexpr const void *data() const noexcept {
            switch (m_type) {
            case Undefined:
            case Null:
                return nullptr;
            case Boolean:
                return &m_bool;
            case Integer:
                return &m_int;
            case Double:
                return &m_double;
            case String:
                return &m_string;
            }

            // GCC 8.x does not treat __builtin_unreachable() as constexpr
        #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
            Q_UNREACHABLE_RETURN(nullptr);
        #else
            return nullptr;
        #endif
        }

    private:
        constexpr bool assignSimple(const QJSPrimitiveValuePrivate &other) noexcept
        {
            switch (other.m_type) {
            case Undefined:
            case Null:
                return true;
            case Boolean:
                m_bool = other.m_bool;
                return true;
            case Integer:
                m_int = other.m_int;
                return true;
            case Double:
                m_double = other.m_double;
                return true;
            case String:
                return false;
            }

            // GCC 8.x does not treat __builtin_unreachable() as constexpr
        #if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
            Q_UNREACHABLE_RETURN(false);
        #else
            return false;
        #endif
        }

        union {
            bool m_bool;
            int m_int;
            double m_double;
            QString m_string;

            // Dummy value to trigger initialization of the whole storage.
            // We don't want to see maybe-uninitialized warnings every time we access m_string.
            std::byte m_zeroInitialize[sizeof(QString)] = {};
        };

        Type m_type = Undefined;
    };

    QJSPrimitiveValuePrivate d;
};

namespace QQmlPrivate {
    // TODO: Make this constexpr once std::isnan is constexpr.
    inline double jsExponentiate(double base, double exponent)
    {
        constexpr double qNaN = std::numeric_limits<double>::quiet_NaN();
        constexpr double inf = std::numeric_limits<double>::infinity();

        if (qIsNull(exponent))
            return 1.0;

        if (std::isnan(exponent))
            return qNaN;

        if (QJSNumberCoercion::equals(base, 1.0) || QJSNumberCoercion::equals(base, -1.0))
            return std::isinf(exponent) ? qNaN : std::pow(base, exponent);

        if (!qIsNull(base))
            return std::pow(base, exponent);

        if (std::copysign(1.0, base) > 0.0)
            return exponent < 0.0 ? inf : std::pow(base, exponent);

        if (exponent < 0.0)
            return QJSNumberCoercion::equals(std::fmod(-exponent, 2.0), 1.0) ? -inf : inf;

        return QJSNumberCoercion::equals(std::fmod(exponent, 2.0), 1.0)
                ? std::copysign(0, -1.0)
                : 0.0;
    }
}

QT_END_NAMESPACE

#endif // QJSPRIMITIVEVALUE_H
