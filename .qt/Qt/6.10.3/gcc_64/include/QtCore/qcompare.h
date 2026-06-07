// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARE_H
#define QCOMPARE_H

#if 0
#pragma qt_class(QtCompare)
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qcompare_impl.h>
#include <QtCore/qstdlibdetection.h>

#ifdef __cpp_lib_bit_cast
#include <bit>
#endif
#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif

QT_BEGIN_NAMESPACE

namespace QtPrivate {
using CompareUnderlyingType = qint8;
constexpr CompareUnderlyingType LegacyUncomparableValue = -127; // historic Qt value

// [cmp.categories.pre] / 1
enum class Ordering : CompareUnderlyingType
{
    Equal = 0,
    Equivalent = Equal,
    Less = -1,
    Greater = 1
};

enum class Uncomparable : CompareUnderlyingType
{
    // We choose the value of our Uncomparable to be the same that the C++
    // Standard Library chooses for its own std::partial_ordering::unordered,
    // so we can convert from their type to ours via simple std::bit_cast.
#if 0
    // GCC 16 broke ABI, so we cannot use the std::*_ordering types
    // in our ABI until we drop support for GCC 15 and earlier. When that
    // happens and std::bit_cast is guaranteed, this can be simplified to:
    Unordered = std::bit_cast<CompareUnderlyingType>(std::partial_ordering::unordered);
#else
    Unordered =
        #if   defined(Q_STL_LIBCPP)
                -127
        #elif defined(Q_STL_LIBSTDCPP)
                   2
        #elif defined(Q_STL_MSSTL)
                -128
        #elif defined(Q_STL_DINKUMWARE) || \
              defined(Q_STL_ROGUEWAVE)  || \
              defined(Q_STL_STLPORT)    || \
              defined(Q_STL_SGI)
                QtPrivate::LegacyUncomparableValue
        // We haven't seen C++20 of these libraries, so we haven't chosen a value yet.
        # ifdef __cpp_lib_three_way_comparison
        #  error Please report the numeric value of std::partial_ordering::unordered in your STL in a bug report.
        # endif
        #else
        #   error Please handle any newly-added Q_STL_ checks in the above ifdef-ery.
        #endif
#endif // future Qt
};
} // namespace QtPrivate

namespace QtOrderingPrivate {

using QtPrivate::Ordering;
using QtPrivate::Uncomparable;

#if defined(__cpp_lib_bit_cast) && defined(__cpp_lib_three_way_comparison)
inline constexpr bool OrderingValuesAreEqual =
        std::bit_cast<Ordering>(std::weak_ordering::equivalent) == Ordering::Equivalent &&
        std::bit_cast<Ordering>(std::strong_ordering::equal) == Ordering::Equal &&
        std::bit_cast<Ordering>(std::strong_ordering::less) == Ordering::Less &&
        std::bit_cast<Ordering>(std::strong_ordering::greater) == Ordering::Greater;
inline constexpr bool UnorderedValueIsEqual =
        std::bit_cast<Uncomparable>(std::partial_ordering::unordered) == Uncomparable::Unordered;
#endif

template <typename O>
constexpr O reversed(O o) noexcept
{
    // https://eel.is/c++draft/cmp.partialord#5
    return is_lt(o) ? O::greater :
           is_gt(o) ? O::less :
           /*else*/ o ;
}

} // namespace QtOrderingPrivate

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4702)   // unreachable code

namespace Qt {

class weak_ordering;
class strong_ordering;

class partial_ordering
{
public:
    static const partial_ordering less;
    static const partial_ordering equivalent;
    static const partial_ordering greater;
    static const partial_ordering unordered;

    friend constexpr bool operator==(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order == 0; }

    friend constexpr bool operator!=(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return !lhs.isOrdered() || lhs.m_order != 0; }

    friend constexpr bool operator< (partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <  0; }

    friend constexpr bool operator<=(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <= 0; }

    friend constexpr bool operator> (partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >  0; }

    friend constexpr bool operator>=(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return !rhs.isOrdered() || 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::partial_ordering
    operator<=>(partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.partialord#4

    friend constexpr std::partial_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, partial_ordering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT partial_ordering(std::partial_ordering stdorder) noexcept
        : m_order{} // == equivalent
    {
        if (stdorder == std::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (stdorder == std::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Uncomparable::Unordered);
    }

    constexpr Q_IMPLICIT operator std::partial_ordering() const noexcept
    {
        static_assert(sizeof(*this) == sizeof(std::partial_ordering));
        using O = QtPrivate::Ordering;
        using U = QtPrivate::Uncomparable;
        using R = std::partial_ordering;
#ifdef __cpp_lib_bit_cast
        if constexpr (QtOrderingPrivate::OrderingValuesAreEqual) {
            if constexpr (!QtOrderingPrivate::UnorderedValueIsEqual) {
                if (m_order == qToUnderlying(U::Unordered))
                    return R::unordered;
            }
            return std::bit_cast<R>(*this);
        }
#endif // __cpp_lib_bit_cast
        switch (m_order) {
        case qToUnderlying(O::Less):        return R::less;
        case qToUnderlying(O::Greater):     return R::greater;
        case qToUnderlying(O::Equivalent):  return R::equivalent;
        case qToUnderlying(U::Unordered):   return R::unordered;
        }
        Q_UNREACHABLE_RETURN(R::unordered);
    }

    friend constexpr bool operator==(partial_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(partial_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator==(partial_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(partial_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::strong_ordering lhs, partial_ordering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, partial_ordering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator==(partial_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(partial_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::weak_ordering lhs, partial_ordering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, partial_ordering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    friend class weak_ordering;
    friend class strong_ordering;

    constexpr explicit partial_ordering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}
    constexpr explicit partial_ordering(QtPrivate::Uncomparable order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    QT_WARNING_DISABLE_CLANG("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (partial_ordering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (partial_ordering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (partial_ordering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(partial_ordering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (partial_ordering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(partial_ordering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    // instead of the exposition only is_ordered member in [cmp.partialord],
    // use a private function
    constexpr bool isOrdered() const noexcept
    { return m_order != static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Uncomparable::Unordered); }

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr partial_ordering partial_ordering::less(QtPrivate::Ordering::Less);
inline constexpr partial_ordering partial_ordering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr partial_ordering partial_ordering::greater(QtPrivate::Ordering::Greater);
inline constexpr partial_ordering partial_ordering::unordered(QtPrivate::Uncomparable::Unordered);

class weak_ordering
{
public:
    static const weak_ordering less;
    static const weak_ordering equivalent;
    static const weak_ordering greater;

    constexpr Q_IMPLICIT operator partial_ordering() const noexcept
    { return partial_ordering(static_cast<QtPrivate::Ordering>(m_order)); }

    friend constexpr bool operator==(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order == 0; }

    friend constexpr bool operator!=(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order != 0; }

    friend constexpr bool operator< (weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <  0; }

    friend constexpr bool operator<=(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <= 0; }

    friend constexpr bool operator> (weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >  0; }

    friend constexpr bool operator>=(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 > rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::weak_ordering
    operator<=>(weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.weakord#5

    friend constexpr std::weak_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, weak_ordering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    friend constexpr bool operator==(weak_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<partial_ordering>(rhs); }

    friend constexpr bool operator!=(partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<partial_ordering>(rhs); }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT weak_ordering(std::weak_ordering stdorder) noexcept
        : m_order{} // == equivalent
    {
        if (stdorder == std::weak_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::weak_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
    }

    constexpr Q_IMPLICIT operator std::weak_ordering() const noexcept
    {
        static_assert(sizeof(*this) == sizeof(std::weak_ordering));
        using O = QtPrivate::Ordering;
        using R = std::weak_ordering;
#ifdef __cpp_lib_bit_cast
        if constexpr (QtOrderingPrivate::OrderingValuesAreEqual)
            return std::bit_cast<R>(*this);
#endif // __cpp_lib_bit_cast
        switch (m_order) {
        case qToUnderlying(O::Less):          return R::less;
        case qToUnderlying(O::Greater):       return R::greater;
        case qToUnderlying(O::Equivalent):    return R::equivalent;
        }
        Q_UNREACHABLE_RETURN(R::equivalent);
    }

    friend constexpr bool operator==(weak_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(weak_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(weak_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator==(std::partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator==(std::strong_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    friend class strong_ordering;

    constexpr explicit weak_ordering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    QT_WARNING_DISABLE_CLANG("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (weak_ordering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (weak_ordering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (weak_ordering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(weak_ordering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (weak_ordering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(weak_ordering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr weak_ordering weak_ordering::less(QtPrivate::Ordering::Less);
inline constexpr weak_ordering weak_ordering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr weak_ordering weak_ordering::greater(QtPrivate::Ordering::Greater);

class strong_ordering
{
public:
    static const strong_ordering less;
    static const strong_ordering equivalent;
    static const strong_ordering equal;
    static const strong_ordering greater;

    constexpr Q_IMPLICIT operator partial_ordering() const noexcept
    { return partial_ordering(static_cast<QtPrivate::Ordering>(m_order)); }

    constexpr Q_IMPLICIT operator weak_ordering() const noexcept
    { return weak_ordering(static_cast<QtPrivate::Ordering>(m_order)); }

    friend constexpr bool operator==(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order == 0; }

    friend constexpr bool operator!=(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order != 0; }

    friend constexpr bool operator< (strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <  0; }

    friend constexpr bool operator<=(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <= 0; }

    friend constexpr bool operator> (strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >  0; }

    friend constexpr bool operator>=(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                    strong_ordering rhs) noexcept
    { return 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                    strong_ordering rhs) noexcept
    { return 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::strong_ordering
    operator<=>(strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.strongord#6

    friend constexpr std::strong_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, strong_ordering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    friend constexpr bool operator==(strong_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator==(partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<partial_ordering>(rhs); }

    friend constexpr bool operator!=(partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<partial_ordering>(rhs); }

    friend constexpr bool operator==(strong_ordering lhs, weak_ordering rhs) noexcept
    { return static_cast<weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, weak_ordering rhs) noexcept
    { return static_cast<weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator==(weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<weak_ordering>(rhs); }

    friend constexpr bool operator!=(weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<weak_ordering>(rhs); }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT strong_ordering(std::strong_ordering stdorder) noexcept
        : m_order{} // == equivalent
    {
        if (stdorder == std::strong_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::strong_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
    }

    constexpr Q_IMPLICIT operator std::strong_ordering() const noexcept
    {
        static_assert(sizeof(*this) == sizeof(std::strong_ordering));
        using O = QtPrivate::Ordering;
        using R = std::strong_ordering;
#ifdef __cpp_lib_bit_cast
        if constexpr (QtOrderingPrivate::OrderingValuesAreEqual)
            return std::bit_cast<R>(*this);
#endif // __cpp_lib_bit_cast
        switch (m_order) {
        case qToUnderlying(O::Less):    return R::less;
        case qToUnderlying(O::Greater): return R::greater;
        case qToUnderlying(O::Equal):   return R::equal;
        }
        Q_UNREACHABLE_RETURN(R::equal);
    }

    friend constexpr bool operator==(strong_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(strong_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(strong_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator==(std::partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator==(std::weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

    private:
    constexpr explicit strong_ordering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    QT_WARNING_DISABLE_CLANG("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (strong_ordering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (strong_ordering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (strong_ordering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(strong_ordering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (strong_ordering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(strong_ordering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr strong_ordering strong_ordering::less(QtPrivate::Ordering::Less);
inline constexpr strong_ordering strong_ordering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr strong_ordering strong_ordering::equal(QtPrivate::Ordering::Equal);
inline constexpr strong_ordering strong_ordering::greater(QtPrivate::Ordering::Greater);

} // namespace Qt

QT_WARNING_POP

QT_BEGIN_INCLUDE_NAMESPACE

// This is intentionally included after Qt::*_ordering types and before
// qCompareThreeWay. Do not change!
#include <QtCore/qcomparehelpers.h>

QT_END_INCLUDE_NAMESPACE

#if defined(Q_QDOC)

template <typename LeftType, typename RightType>
auto qCompareThreeWay(const LeftType &lhs, const RightType &rhs);

#else

template <typename LT, typename RT,
          std::enable_if_t<
                  std::disjunction_v<
                          QtOrderingPrivate::CompareThreeWayTester::HasCompareThreeWay<LT, RT>,
                          QtOrderingPrivate::CompareThreeWayTester::HasCompareThreeWay<RT, LT>>,
                  bool> = true>
auto qCompareThreeWay(const LT &lhs, const RT &rhs)
        noexcept(QtOrderingPrivate::CompareThreeWayTester::compareThreeWayNoexcept<LT, RT>())
{
    using Qt::compareThreeWay;
    if constexpr (QtOrderingPrivate::CompareThreeWayTester::hasCompareThreeWay_v<LT, RT>) {
        return compareThreeWay(lhs, rhs);
    } else {
        const auto retval = compareThreeWay(rhs, lhs);
        return QtOrderingPrivate::reversed(retval);
    }
}

#endif // defined(Q_QDOC)

//
// Legacy QPartialOrdering
//

namespace QtPrivate {
enum class LegacyUncomparable : CompareUnderlyingType
{
    Unordered = QtPrivate::LegacyUncomparableValue
};
}

// [cmp.partialord]
class QPartialOrdering
{
public:
    static const QPartialOrdering Less;
    static const QPartialOrdering Equivalent;
    static const QPartialOrdering Greater;
    static const QPartialOrdering Unordered;

    static const QPartialOrdering less;
    static const QPartialOrdering equivalent;
    static const QPartialOrdering greater;
    static const QPartialOrdering unordered;

    friend constexpr bool operator==(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order == 0; }

    friend constexpr bool operator!=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return !lhs.isOrdered() || lhs.m_order != 0; }

    friend constexpr bool operator< (QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <  0; }

    friend constexpr bool operator<=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <= 0; }

    friend constexpr bool operator> (QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >  0; }

    friend constexpr bool operator>=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return !rhs.isOrdered() || 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::partial_ordering
    operator<=>(QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.partialord#4

    friend constexpr std::partial_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(QPartialOrdering lhs, QPartialOrdering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(QPartialOrdering lhs, QPartialOrdering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    constexpr Q_IMPLICIT QPartialOrdering(Qt::partial_ordering order) noexcept
        : m_order{} // == equivalent
    {
        if (order == Qt::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (order == Qt::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (order == Qt::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::LegacyUncomparable::Unordered);
    }

    constexpr Q_IMPLICIT QPartialOrdering(Qt::weak_ordering stdorder) noexcept
        : QPartialOrdering(Qt::partial_ordering{stdorder}) {}

    constexpr Q_IMPLICIT QPartialOrdering(Qt::strong_ordering stdorder) noexcept
        : QPartialOrdering(Qt::partial_ordering{stdorder}) {}

    constexpr Q_IMPLICIT operator Qt::partial_ordering() const noexcept
    {
        using O = QtPrivate::Ordering;
        using U = QtPrivate::LegacyUncomparable;
        using R = Qt::partial_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):       return R::less;
        case qToUnderlying(O::Greater):    return R::greater;
        case qToUnderlying(O::Equivalent): return R::equivalent;
        case qToUnderlying(U::Unordered):  return R::unordered;
        }
        // GCC 8.x does not treat __builtin_unreachable() as constexpr
#if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
        Q_UNREACHABLE();
#endif
        return R::unordered;
    }

    friend constexpr bool operator==(QPartialOrdering lhs, Qt::partial_ordering rhs) noexcept
    { Qt::partial_ordering qt = lhs; return qt == rhs; }

    friend constexpr bool operator!=(QPartialOrdering lhs, Qt::partial_ordering rhs) noexcept
    { Qt::partial_ordering qt = lhs; return qt != rhs; }

    friend constexpr bool operator==(Qt::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { Qt::partial_ordering qt = rhs; return lhs == qt; }

    friend constexpr bool operator!=(Qt::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { Qt::partial_ordering qt = rhs; return lhs != qt; }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT QPartialOrdering(std::partial_ordering stdorder) noexcept
        : m_order{} // == equivalent
    {
        if (stdorder == std::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (stdorder == std::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::LegacyUncomparable::Unordered);
    }

    constexpr Q_IMPLICIT QPartialOrdering(std::weak_ordering stdorder) noexcept
        : QPartialOrdering(std::partial_ordering(stdorder)) {}

    constexpr Q_IMPLICIT QPartialOrdering(std::strong_ordering stdorder) noexcept
        : QPartialOrdering(std::partial_ordering(stdorder)) {}

    constexpr Q_IMPLICIT operator std::partial_ordering() const noexcept
    {
        using O = QtPrivate::Ordering;
        using U = QtPrivate::LegacyUncomparable;
        using R = std::partial_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):       return R::less;
        case qToUnderlying(O::Greater):    return R::greater;
        case qToUnderlying(O::Equivalent): return R::equivalent;
        case qToUnderlying(U::Unordered):  return R::unordered;
        }
        Q_UNREACHABLE_RETURN(R::unordered);
    }

    friend constexpr bool operator==(QPartialOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QPartialOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    constexpr explicit QPartialOrdering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}
    constexpr explicit QPartialOrdering(QtPrivate::LegacyUncomparable order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    QT_WARNING_DISABLE_CLANG("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (QPartialOrdering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (QPartialOrdering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (QPartialOrdering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(QPartialOrdering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (QPartialOrdering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(QPartialOrdering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    // instead of the exposition only is_ordered member in [cmp.partialord],
    // use a private function
    constexpr bool isOrdered() const noexcept
    { return m_order != static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::LegacyUncomparable::Unordered); }

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr QPartialOrdering QPartialOrdering::Less(QtPrivate::Ordering::Less);
inline constexpr QPartialOrdering QPartialOrdering::Equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QPartialOrdering QPartialOrdering::Greater(QtPrivate::Ordering::Greater);
inline constexpr QPartialOrdering QPartialOrdering::Unordered(QtPrivate::LegacyUncomparable::Unordered);

inline constexpr QPartialOrdering QPartialOrdering::less(QtPrivate::Ordering::Less);
inline constexpr QPartialOrdering QPartialOrdering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QPartialOrdering QPartialOrdering::greater(QtPrivate::Ordering::Greater);
inline constexpr QPartialOrdering QPartialOrdering::unordered(QtPrivate::LegacyUncomparable::Unordered);

QT_END_NAMESPACE

#endif // QCOMPARE_H
