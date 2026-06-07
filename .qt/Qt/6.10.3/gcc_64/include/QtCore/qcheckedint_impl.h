// Copyright (C) 2025 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCHECKEDINT_H
#define QCHECKEDINT_H

#include <QtCore/qassert.h>
#include <QtCore/qcompare.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qnumeric.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
namespace QCheckedIntegers {

template <typename Int>
struct CheckIntTypeHelper
{
    static_assert(std::is_integral_v<Int>, "Only integer types are supported");
    static_assert(std::is_signed_v<Int>, "Only signed types are supported");
    static_assert(std::is_same_v<Int, decltype(+Int{})>, "Only fully promoted types are supported");
};

// Implements wraparound semantics, and checks for overflow.
// Never causes UB on any operation.
template <typename Int>
struct SafeCheckImpl : private CheckIntTypeHelper<Int>
{
    static inline constexpr Int MinInt = (std::numeric_limits<Int>::min)();
    static inline constexpr Int MaxInt = (std::numeric_limits<Int>::max)();

    [[nodiscard]]
    static constexpr bool add(Int a, Int b, Int *result) noexcept
    {
        return !qAddOverflow(a, b, result);
    }

    [[nodiscard]]
    static constexpr bool sub(Int a, Int b, Int *result) noexcept
    {
        return !qSubOverflow(a, b, result);
    }

    [[nodiscard]]
    static constexpr bool mul(Int a, Int b, Int *result) noexcept
    {
        return !qMulOverflow(a, b, result);
    }

    [[nodiscard]]
    static constexpr bool div(Int a, Int b, Int *result) noexcept
    {
        if (Q_UNLIKELY(b == 0))
            return false;

        *result = a / b;
        return true;
    }
};

// Reports failed checks through Q_ASSERT.
struct AssertReportPolicy
{
    static constexpr void check(bool ok, const char *where, const char *description)
    {
        Q_ASSERT_X(ok, where, description);
    }
};

template <typename Int,
          typename Impl = SafeCheckImpl<Int>,
          typename FailureReportPolicy = AssertReportPolicy>
class QCheckedInt : private CheckIntTypeHelper<Int>
{
    Int m_i;

#define Q_CHECKEDINT_POLICY_CHECK(cond, what) \
    FailureReportPolicy::check(cond, Q_FUNC_INFO, what)

public:
    template <typename AInt>
    using if_is_same_int = std::enable_if_t<std::is_same_v<Int, AInt>, bool>;

    QCheckedInt() = default;

    explicit constexpr QCheckedInt(Int i) noexcept
        : m_i(i)
    {}

    // Conversions
    explicit constexpr operator Int() const noexcept
    {
        return m_i;
    }

    // Accessors
    constexpr Int value() const noexcept { return m_i; }
    template <typename AInt, if_is_same_int<AInt> = true>
    constexpr void setValue(AInt i) noexcept { m_i = i; }

    constexpr Int &as_underlying() & noexcept { return m_i; }
    constexpr const Int &as_underlying() const & noexcept { return m_i; }
    constexpr Int &&as_underlying() && noexcept { return std::move(m_i); }
    constexpr const Int &&as_underlying() const && noexcept { return std::move(m_i); }

    // Unary ops
    constexpr QCheckedInt operator+() const noexcept { return *this; }
    constexpr QCheckedInt operator-() const
    {
        QCheckedInt result{};
        const bool ok = Impl::sub(Int(0), m_i, &result.m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Overflow in unary negation");
        return result;
    }

    constexpr QCheckedInt &operator++()
    {
        const bool ok = Impl::add(m_i, Int(1), &m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Overflow in operator++");
        return *this;
    }

    constexpr QCheckedInt operator++(int)
    {
        QCheckedInt result = *this;
        ++*this;
        return result;
    }

    constexpr QCheckedInt &operator--()
    {
        const bool ok = Impl::sub(m_i, Int(1), &m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Overflow in operator--");
        return *this;
    }

    constexpr QCheckedInt operator--(int)
    {
        QCheckedInt result = *this;
        --*this;
        return result;
    }

    // Addition
    friend constexpr QCheckedInt operator+(QCheckedInt lhs, QCheckedInt rhs)
    {
        QCheckedInt result{};
        const bool ok = Impl::add(lhs.m_i, rhs.m_i, &result.m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Overflow in operator+");
        return result;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator+(QCheckedInt lhs, AInt rhs)
    {
        return lhs + QCheckedInt(rhs);
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator+(AInt lhs, QCheckedInt rhs)
    {
        return QCheckedInt(lhs) + rhs;
    }

    constexpr QCheckedInt &operator+=(QCheckedInt other)
    {
        return *this = *this + other;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    constexpr QCheckedInt &operator+=(AInt other)
    {
        return *this = *this + QCheckedInt(other);
    }

    // Subtraction
    friend constexpr QCheckedInt operator-(QCheckedInt lhs, QCheckedInt rhs)
    {
        QCheckedInt result{};
        const bool ok = Impl::sub(lhs.m_i, rhs.m_i, &result.m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Overflow in operator-");
        return result;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator-(QCheckedInt lhs, AInt rhs)
    {
        return lhs - QCheckedInt(rhs);
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator-(AInt lhs, QCheckedInt rhs)
    {
        return QCheckedInt(lhs) - rhs;
    }

    constexpr QCheckedInt &operator-=(QCheckedInt other)
    {
        return *this = *this - other;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    constexpr QCheckedInt &operator-=(AInt other)
    {
        return *this = *this - QCheckedInt(other);
    }

    // Multiplication
    friend constexpr QCheckedInt operator*(QCheckedInt lhs, QCheckedInt rhs)
    {
        QCheckedInt result{};
        const bool ok = Impl::mul(lhs.m_i, rhs.m_i, &result.m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Overflow in operator*");
        return result;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator*(QCheckedInt lhs, AInt rhs)
    {
        return lhs * QCheckedInt(rhs);
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator*(AInt lhs, QCheckedInt rhs)
    {
        return QCheckedInt(lhs) * rhs;
    }

    constexpr QCheckedInt &operator*=(QCheckedInt other)
    {
        return *this = *this * other;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    constexpr QCheckedInt &operator*=(AInt other)
    {
        return *this = *this * QCheckedInt(other);
    }

    // Division
    friend constexpr QCheckedInt operator/(QCheckedInt lhs, QCheckedInt rhs)
    {
        QCheckedInt result{};
        const bool ok = Impl::div(lhs.m_i, rhs.m_i, &result.m_i);
        Q_CHECKEDINT_POLICY_CHECK(ok, "Division by zero");
        return result;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator/(QCheckedInt lhs, AInt rhs)
    {
        return lhs / QCheckedInt(rhs);
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr QCheckedInt operator/(AInt lhs, QCheckedInt rhs)
    {
        return QCheckedInt(lhs) / rhs;
    }

    constexpr QCheckedInt &operator/=(QCheckedInt other)
    {
        return *this = *this / other;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    constexpr QCheckedInt &operator/=(AInt other)
    {
        return *this = *this / QCheckedInt(other);
    }

#undef Q_CHECKEDINT_POLICY_CHECK

    // Comparisons
    friend constexpr bool comparesEqual(QCheckedInt lhs, QCheckedInt rhs) noexcept
    {
        return lhs.m_i == rhs.m_i;
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr bool comparesEqual(QCheckedInt lhs, AInt rhs) noexcept
    {
        return lhs.m_i == rhs;
    }

    friend constexpr Qt::strong_ordering compareThreeWay(QCheckedInt lhs, QCheckedInt rhs) noexcept
    {
        return Qt::compareThreeWay(lhs.m_i, rhs.m_i);
    }

    template <typename AInt, if_is_same_int<AInt> = true>
    friend constexpr Qt::strong_ordering compareThreeWay(QCheckedInt lhs, AInt rhs) noexcept
    {
        return Qt::compareThreeWay(lhs.m_i, rhs);
    }

    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QCheckedInt)
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(
        QCheckedInt,
        AInt,
        template <typename AInt, if_is_same_int<AInt> = true>)

private:
    friend size_t constexpr qHash(QCheckedInt key, size_t seed = 0) noexcept
    {
        using QT_PREPEND_NAMESPACE(qHash); // ### needed?
        return qHash(key.value(), seed);
    }
};

} // namespace QCheckedIntegers
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QCHECKEDINT_H
