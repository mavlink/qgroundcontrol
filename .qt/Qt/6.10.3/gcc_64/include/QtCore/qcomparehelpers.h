// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARE_H
#error "Do not include qcomparehelpers.h directly. Use qcompare.h instead."
#endif

#ifndef QCOMPAREHELPERS_H
#define QCOMPAREHELPERS_H

#if 0
#pragma qt_no_master_include
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qflags.h>
#include <QtCore/qoverload.h>
#include <QtCore/qttypetraits.h>
#include <QtCore/qtypeinfo.h>
#include <QtCore/qtypes.h>

#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif
#include <QtCore/q20type_traits.h>

#include <functional> // std::less, std::hash

QT_BEGIN_NAMESPACE

class QPartialOrdering;

namespace QtOrderingPrivate {
template <typename T> struct is_std_ordering_type : std::false_type {};
template <typename T> struct is_qt_ordering_type : std::false_type {};

template <typename T> constexpr bool is_std_ordering_type_v = is_std_ordering_type<T>::value;
template <typename T> constexpr bool is_qt_ordering_type_v = is_qt_ordering_type<T>::value;

enum class QtOrderingType {
    QtOrder =  0x00,
    StdOrder = 0x01,
    Partial = 0x00,
    Weak = 0x20,
    Strong = 0x40,
    StrengthMask = Weak|Strong,
};
Q_DECLARE_FLAGS(QtOrderingTypeFlag, QtOrderingType)
Q_DECLARE_OPERATORS_FOR_FLAGS(QtOrderingPrivate::QtOrderingTypeFlag)

template <typename QtOrdering> struct StdOrdering;
template <typename StdOrdering> struct QtOrdering;

#ifdef __cpp_lib_three_way_comparison
#define QT_STD_MAP(x) \
    template <> struct StdOrdering< Qt::x##_ordering> : q20::type_identity<std::x##_ordering> {};\
    template <> struct StdOrdering<std::x##_ordering> : q20::type_identity<std::x##_ordering> {};\
    template <> struct  QtOrdering<std::x##_ordering> : q20::type_identity< Qt::x##_ordering> {};\
    template <> struct  QtOrdering< Qt::x##_ordering> : q20::type_identity< Qt::x##_ordering> {};\
    template <> struct is_std_ordering_type<std::x##_ordering> : std::true_type {};\
    template <> struct is_qt_ordering_type< Qt::x##_ordering> : std::true_type {};\
    /* end */
QT_STD_MAP(partial)
QT_STD_MAP(weak)
QT_STD_MAP(strong)
#undef QT_STD_MAP

template <> struct StdOrdering<QPartialOrdering> : q20::type_identity<std::partial_ordering> {};
template <> struct  QtOrdering<QPartialOrdering> : q20::type_identity< Qt::partial_ordering> {};
#else
template <> struct is_qt_ordering_type< Qt::partial_ordering> : std::true_type {};
template <> struct is_qt_ordering_type< Qt::weak_ordering> : std::true_type {};
template <> struct is_qt_ordering_type< Qt::strong_ordering> : std::true_type {};
#endif // __cpp_lib_three_way_comparison

template <typename In> constexpr auto to_std(In in) noexcept
    -> typename QtOrderingPrivate::StdOrdering<In>::type
{ return in; }

template <typename In> constexpr auto to_Qt(In in) noexcept
    -> typename QtOrderingPrivate::QtOrdering<In>::type
{ return in; }

template <typename T>
constexpr bool is_ordering_type_v
        = std::disjunction_v<is_qt_ordering_type<T>, is_std_ordering_type<T>>;

template <typename T>
constexpr std::enable_if_t<is_qt_ordering_type_v<T>, QtOrderingTypeFlag>
orderingFlagsFor(T t) noexcept
{
    QtOrderingTypeFlag flags = QtOrderingType::QtOrder;
    Qt::partial_ordering convertedOrder(t);
    if constexpr (std::is_same_v<T, Qt::strong_ordering>)
        flags = flags | QtOrderingType::Strong;
    else if constexpr (std::is_same_v<T, Qt::partial_ordering>)
        flags = flags | QtOrderingType::Partial;
    else if constexpr (std::is_same_v<T, Qt::weak_ordering>)
        flags = flags | QtOrderingType::Weak;
    return flags;
}

template <typename T>
constexpr std::enable_if_t<is_std_ordering_type_v<T>, QtOrderingTypeFlag>
orderingFlagsFor(T t) noexcept
{
    QtOrderingPrivate::QtOrderingTypeFlag flags = QtOrderingPrivate::QtOrderingType::StdOrder;
    return QtOrderingTypeFlag(flags
                              | QtOrderingPrivate::orderingFlagsFor(QtOrderingPrivate::to_Qt(t)));
}
} // namespace QtOrderingPrivate

/*
    For all the macros these parameter names are used:
    * LeftType - the type of the left operand of the comparison
    * RightType - the type of the right operand of the comparison
    * Constexpr - must be either constexpr or empty. Defines whether the
                  operator is constexpr or not
    * Noexcept - a noexcept specifier. By default the relational operators are
                 expected to be noexcept. However, there are some cases when
                 this cannot be achieved (e.g. QDir). The public macros will
                 pass noexcept(true) or noexcept(false) in this parameter,
                 because conditional noexcept is known to cause some issues.
                 However, internally we might want to pass a predicate here
                 for some specific classes (e.g. QList, etc).
    * Attributes... - an optional list of attributes. For example, pass
                      \c QT_ASCII_CAST_WARN when defining comparisons between
                      C-style string and an encoding-aware string type.
                      This is a variable argument, and can now include up to 7
                      comma-separated parameters.

    The macros require two helper functions. For operators to be constexpr,
    these must be constexpr, too. Additionally, other attributes (like
    Q_<Module>_EXPORT, Q_DECL_CONST_FUNCTION, etc) can be applied to them.
    Aside from that, their declaration should match:
        bool comparesEqual(LeftType, RightType) noexcept;
        ReturnType compareThreeWay(LeftType, RightType) noexcept;

    The ReturnType can be one of Qt::{partial,weak,strong}_ordering. The actual
    type depends on the macro being used.
    It makes sense to define the helper functions as hidden friends of the
    class, so that they could be found via ADL, and don't participate in
    unintended implicit conversions.
*/

/*
    Some systems (e.g. QNX and Integrity (GHS compiler)) have bugs in
    handling conditional noexcept in lambdas.
    This macro is needed to overcome such bugs and provide a noexcept check only
    on platforms that behave normally.
    It does nothing on the systems that have problems.

    This hack is explicitly disabled for C++20 because we want the compilers
    to fix the known issues before switching.
*/
#if defined(__cpp_lib_three_way_comparison) || !(defined(Q_OS_QNX) || defined(Q_CC_GHS))
# define QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, Func) \
    constexpr auto f = []() Noexcept {}; \
    static_assert(!noexcept(f()) || noexcept(Func(lhs, rhs)), \
                  "Use *_NON_NOEXCEPT version of the macro, " \
                  "or make the helper function noexcept")
#else
# define QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, Func) /* no check */
#endif


// Seems that qdoc uses C++20 even when Qt is compiled in C++17 mode.
// Or at least it defines __cpp_lib_three_way_comparison.
// Let qdoc see only the C++17 operators for now, because that's what our docs
// currently describe.
#if defined(__cpp_lib_three_way_comparison) && !defined(Q_QDOC)
// C++20 - provide operator==() for equality, and operator<=>() for ordering

#define QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, \
                                             Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr bool operator==(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, comparesEqual); \
        return comparesEqual(lhs, rhs); \
    }

#define QT_DECLARE_ORDERING_HELPER_STRONG(LeftType, RightType, Constexpr, Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr std::strong_ordering \
    operator<=>(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, compareThreeWay); \
        return compareThreeWay(lhs, rhs); \
    }

#define QT_DECLARE_ORDERING_HELPER_WEAK(LeftType, RightType, Constexpr, Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr std::weak_ordering \
    operator<=>(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, compareThreeWay); \
        return compareThreeWay(lhs, rhs); \
    }

#define QT_DECLARE_ORDERING_HELPER_PARTIAL(LeftType, RightType, Constexpr, Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr std::partial_ordering \
    operator<=>(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, compareThreeWay); \
        return compareThreeWay(lhs, rhs); \
    }

#define QT_DECLARE_ORDERING_HELPER_AUTO(LeftType, RightType, Constexpr, Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr auto \
    operator<=>(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, compareThreeWay);\
        return QtOrderingPrivate::to_std(compareThreeWay(lhs, rhs)); \
    }

#define QT_DECLARE_ORDERING_OPERATORS_HELPER(OrderingType, LeftType, RightType, Constexpr, \
                                             Noexcept, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, Noexcept, __VA_ARGS__) \
    QT_DECLARE_ORDERING_HELPER_ ## OrderingType (LeftType, RightType, Constexpr, Noexcept, \
                                                 __VA_ARGS__)

#ifdef Q_COMPILER_LACKS_THREE_WAY_COMPARE_SYMMETRY

// define reversed versions of the operators manually, because buggy MSVC versions do not do it
#define QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, \
                                                      Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr bool operator==(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return comparesEqual(rhs, lhs); }

#define QT_DECLARE_REVERSED_ORDERING_HELPER_STRONG(LeftType, RightType, Constexpr, \
                                                   Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr std::strong_ordering \
    operator<=>(RightType const &lhs, LeftType const &rhs) Noexcept \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        return QtOrderingPrivate::reversed(r); \
    }

#define QT_DECLARE_REVERSED_ORDERING_HELPER_WEAK(LeftType, RightType, Constexpr, \
                                                 Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr std::weak_ordering \
    operator<=>(RightType const &lhs, LeftType const &rhs) Noexcept \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        return QtOrderingPrivate::reversed(r); \
    }

#define QT_DECLARE_REVERSED_ORDERING_HELPER_PARTIAL(LeftType, RightType, Constexpr, \
                                                    Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr std::partial_ordering \
    operator<=>(RightType const &lhs, LeftType const &rhs) Noexcept \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        return QtOrderingPrivate::reversed(r); \
    }

#define QT_DECLARE_REVERSED_ORDERING_HELPER_AUTO(LeftType, RightType, Constexpr, Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr auto \
    operator<=>(RightType const &lhs, LeftType const &rhs) Noexcept \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        return QtOrderingPrivate::to_std(QtOrderingPrivate::reversed(r)); \
    }

#define QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(OrderingString, LeftType, RightType, \
                                                      Constexpr, Noexcept, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, \
                                                  Noexcept, __VA_ARGS__) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_ ## OrderingString (LeftType, RightType, Constexpr, \
                                                            Noexcept, __VA_ARGS__)

#else

// dummy macros for C++17 compatibility, reversed operators are generated by the compiler
#define QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, \
                                                      Noexcept, ...)
#define QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(OrderingString, LeftType, RightType, \
                                                      Constexpr, Noexcept, ...)

#endif // Q_COMPILER_LACKS_THREE_WAY_COMPARE_SYMMETRY

#else
// C++17 - provide operator==() and operator!=() for equality,
// and all 4 comparison operators for ordering

#define QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, \
                                             Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr bool operator==(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, comparesEqual); \
        return comparesEqual(lhs, rhs); \
    } \
    __VA_ARGS__ \
    friend Constexpr bool operator!=(LeftType const &lhs, RightType const &rhs) Noexcept \
    { return !comparesEqual(lhs, rhs); }

// Helpers for reversed comparison, using the existing comparesEqual() function.
#define QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, \
                                                      Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr bool operator==(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return comparesEqual(rhs, lhs); } \
    __VA_ARGS__ \
    friend Constexpr bool operator!=(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return !comparesEqual(rhs, lhs); }

#define QT_DECLARE_ORDERING_HELPER_TEMPLATE(OrderingType, LeftType, RightType, Constexpr, \
                                            Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr bool operator<(LeftType const &lhs, RightType const &rhs) Noexcept \
    { \
        QT_COMPARISON_NOEXCEPT_CHECK(Noexcept, compareThreeWay); \
        return is_lt(compareThreeWay(lhs, rhs)); \
    } \
    __VA_ARGS__ \
    friend Constexpr bool operator>(LeftType const &lhs, RightType const &rhs) Noexcept \
    { return is_gt(compareThreeWay(lhs, rhs)); } \
    __VA_ARGS__ \
    friend Constexpr bool operator<=(LeftType const &lhs, RightType const &rhs) Noexcept \
    { return is_lteq(compareThreeWay(lhs, rhs)); } \
    __VA_ARGS__ \
    friend Constexpr bool operator>=(LeftType const &lhs, RightType const &rhs) Noexcept \
    { return is_gteq(compareThreeWay(lhs, rhs)); }

#define QT_DECLARE_ORDERING_HELPER_AUTO(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(auto, LeftType, RightType, Constexpr, Noexcept, \
                                        __VA_ARGS__)

#define QT_DECLARE_ORDERING_HELPER_PARTIAL(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(Qt::partial_ordering, LeftType, RightType, Constexpr, \
                                        Noexcept, __VA_ARGS__)

#define QT_DECLARE_ORDERING_HELPER_WEAK(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(Qt::weak_ordering, LeftType, RightType, Constexpr, \
                                        Noexcept, __VA_ARGS__)

#define QT_DECLARE_ORDERING_HELPER_STRONG(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(Qt::strong_ordering, LeftType, RightType, Constexpr, \
                                        Noexcept, __VA_ARGS__)

#define QT_DECLARE_ORDERING_OPERATORS_HELPER(OrderingString, LeftType, RightType, Constexpr, \
                                             Noexcept, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, Noexcept, __VA_ARGS__) \
    QT_DECLARE_ORDERING_HELPER_ ## OrderingString (LeftType, RightType, Constexpr, Noexcept, \
                                                   __VA_ARGS__)

// Helpers for reversed ordering, using the existing compareThreeWay() function.
#define QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(OrderingType, LeftType, RightType, Constexpr, \
                                                     Noexcept, ...) \
    __VA_ARGS__ \
    friend Constexpr bool operator<(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return is_gt(compareThreeWay(rhs, lhs)); } \
    __VA_ARGS__ \
    friend Constexpr bool operator>(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return is_lt(compareThreeWay(rhs, lhs)); } \
    __VA_ARGS__ \
    friend Constexpr bool operator<=(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return is_gteq(compareThreeWay(rhs, lhs)); } \
    __VA_ARGS__ \
    friend Constexpr bool operator>=(RightType const &lhs, LeftType const &rhs) Noexcept \
    { return is_lteq(compareThreeWay(rhs, lhs)); }

#define QT_DECLARE_REVERSED_ORDERING_HELPER_AUTO(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(auto, LeftType, RightType, Constexpr, Noexcept, \
                                                 __VA_ARGS__)

#define QT_DECLARE_REVERSED_ORDERING_HELPER_PARTIAL(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(Qt::partial_ordering, LeftType, RightType, \
                                                 Constexpr, Noexcept, __VA_ARGS__)

#define QT_DECLARE_REVERSED_ORDERING_HELPER_WEAK(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(Qt::weak_ordering, LeftType, RightType, \
                                                 Constexpr, Noexcept, __VA_ARGS__)

#define QT_DECLARE_REVERSED_ORDERING_HELPER_STRONG(LeftType, RightType, Constexpr, Noexcept, ...) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(Qt::strong_ordering, LeftType, RightType, \
                                                 Constexpr, Noexcept, __VA_ARGS__)

#define QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(OrderingString, LeftType, RightType, \
                                                      Constexpr, Noexcept, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, Noexcept, \
                                                  __VA_ARGS__) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_ ## OrderingString (LeftType, RightType, Constexpr, \
                                                            Noexcept, __VA_ARGS__)

#endif // __cpp_lib_three_way_comparison

/* Public API starts here */

// Equality operators
#define QT_DECLARE_EQUALITY_COMPARABLE_1(Type) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(Type, Type, /* non-constexpr */, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_2(LeftType, RightType) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_3(LeftType, RightType, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), __VA_ARGS__) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_EQUALITY_COMPARABLE_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_3(__VA_ARGS__))

#define Q_DECLARE_EQUALITY_COMPARABLE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_EQUALITY_COMPARABLE, __VA_ARGS__)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_1(Type) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(Type, Type, constexpr, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, constexpr, noexcept(true), \
                                         /* no attributes */) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, constexpr, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(LeftType, RightType, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, constexpr, noexcept(true), \
                                         __VA_ARGS__) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, constexpr, noexcept(true), \
                                                  __VA_ARGS__)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(__VA_ARGS__))

#define Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE, __VA_ARGS__)

#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_1(Type) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(Type, Type, /* non-constexpr */, noexcept(false), \
                                         /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_2(LeftType, RightType) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(false), /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(LeftType, RightType, ...) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), __VA_ARGS__) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(false), __VA_ARGS__)

#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT_3(__VA_ARGS__))

#define Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT, __VA_ARGS__)

// Ordering operators that automatically deduce the strength:
#define QT_DECLARE_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, Type, Type, /* non-constexpr */, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_ORDERED_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_ORDERED_4(...) QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_5(...) QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_6(...) QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_7(...) QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_8(...) QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_9(...) QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_3(__VA_ARGS__))

#define Q_DECLARE_ORDERED(...) QT_OVERLOADED_MACRO(QT_DECLARE_ORDERED, __VA_ARGS__)

#define QT_DECLARE_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, Type, Type, constexpr, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, LeftType, RightType, constexpr, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(AUTO, LeftType, RightType, constexpr, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_ORDERED_LITERAL_TYPE_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, LeftType, RightType, constexpr, \
                                         noexcept(true), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(AUTO, LeftType, RightType, constexpr, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_ORDERED_LITERAL_TYPE_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_LITERAL_TYPE_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_LITERAL_TYPE_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_LITERAL_TYPE_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_LITERAL_TYPE_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_LITERAL_TYPE_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))

#define Q_DECLARE_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_ORDERED_LITERAL_TYPE, __VA_ARGS__)

#define QT_DECLARE_ORDERED_NON_NOEXCEPT_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, Type, Type, /* non-constexpr */, noexcept(false), \
                                         /* no attributes */)

#define QT_DECLARE_ORDERED_NON_NOEXCEPT_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(false), /* no attributes */)

#define QT_DECLARE_ORDERED_NON_NOEXCEPT_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(AUTO, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(false), __VA_ARGS__)

#define QT_DECLARE_ORDERED_NON_NOEXCEPT_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_NON_NOEXCEPT_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_NON_NOEXCEPT_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_NON_NOEXCEPT_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_NON_NOEXCEPT_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_ORDERED_NON_NOEXCEPT_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))

#define Q_DECLARE_ORDERED_NON_NOEXCEPT(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_ORDERED_NON_NOEXCEPT, __VA_ARGS__)

// Partial ordering operators
#define QT_DECLARE_PARTIALLY_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, Type, Type, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(true), \
                                                  /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(true), __VA_ARGS__)

#define QT_DECLARE_PARTIALLY_ORDERED_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_3(__VA_ARGS__))

#define Q_DECLARE_PARTIALLY_ORDERED(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_PARTIALLY_ORDERED, __VA_ARGS__)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, Type, Type, constexpr, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, constexpr, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, constexpr, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, constexpr, noexcept(true), \
                                         __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, constexpr, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))

#define Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE, __VA_ARGS__)

#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, Type, Type, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(false), \
                                                  /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(false), __VA_ARGS__)

#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))

#define Q_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT, __VA_ARGS__)

// Weak ordering operators
#define QT_DECLARE_WEAKLY_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, Type, Type, /* non-constexpr */, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_WEAKLY_ORDERED_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_3(__VA_ARGS__))

#define Q_DECLARE_WEAKLY_ORDERED(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_WEAKLY_ORDERED, __VA_ARGS__)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, Type, Type, constexpr, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, constexpr, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, constexpr, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, constexpr, noexcept(true), \
                                         __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, constexpr, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))

#define Q_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE, __VA_ARGS__)

#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, Type, Type, /* non-constexpr */, noexcept(false), \
                                         /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(false), /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                                  noexcept(false), __VA_ARGS__)

#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))

#define Q_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT, __VA_ARGS__)

// Strong ordering operators
#define QT_DECLARE_STRONGLY_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, Type, Type, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(true), \
                                                  /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(true), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(true), __VA_ARGS__)

#define QT_DECLARE_STRONGLY_ORDERED_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_3(__VA_ARGS__))

#define Q_DECLARE_STRONGLY_ORDERED(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_STRONGLY_ORDERED, __VA_ARGS__)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, Type, Type, constexpr, noexcept(true), \
                                         /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, constexpr, \
                                         noexcept(true), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, constexpr, \
                                                  noexcept(true), /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, constexpr, noexcept(true), \
                                         __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, constexpr, \
                                                  noexcept(true), __VA_ARGS__)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(__VA_ARGS__))

#define Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE, __VA_ARGS__)

#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, Type, Type, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(false), \
                                                  /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(LeftType, RightType, ...) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, /* non-constexpr */, \
                                         noexcept(false), __VA_ARGS__) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, \
                                                  /* non-constexpr */, noexcept(false), __VA_ARGS__)

#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_4(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_5(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_6(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_7(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_8(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))
#define QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_9(...) \
    QT_VA_ARGS_EXPAND(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT_3(__VA_ARGS__))

#define Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT, __VA_ARGS__)

namespace QtPrivate {

template <typename T>
constexpr bool IsIntegralType_v = std::numeric_limits<std::remove_const_t<T>>::is_specialized
                                  && std::numeric_limits<std::remove_const_t<T>>::is_integer;

template <typename T>
constexpr bool IsFloatType_v = std::is_floating_point_v<T>;

#if QFLOAT16_IS_NATIVE
template <>
inline constexpr bool IsFloatType_v<QtPrivate::NativeFloat16Type> = true;
#endif

} // namespace QtPrivate

namespace QtOrderingPrivate {

template <typename T, typename U>
constexpr Qt::strong_ordering
strongOrderingCompareDefaultImpl(T lhs, U rhs) noexcept
{
#ifdef __cpp_lib_three_way_comparison
    return lhs <=> rhs;
#else
    if (lhs == rhs)
        return Qt::strong_ordering::equivalent;
    else if (lhs < rhs)
        return Qt::strong_ordering::less;
    else
        return Qt::strong_ordering::greater;
#endif // __cpp_lib_three_way_comparison
}

} // namespace QtOrderingPrivate

namespace Qt {

template <typename T>
using if_integral = std::enable_if_t<QtPrivate::IsIntegralType_v<T>, bool>;

template <typename T>
using if_floating_point = std::enable_if_t<QtPrivate::IsFloatType_v<T>, bool>;

template <typename T, typename U>
using if_compatible_pointers =
        std::enable_if_t<std::disjunction_v<std::is_same<T, U>,
                                            std::is_base_of<T, U>,
                                            std::is_base_of<U, T>>,
                         bool>;

template <typename Enum>
using if_enum = std::enable_if_t<std::is_enum_v<Enum>, bool>;

template <typename LeftInt, typename RightInt,
          if_integral<LeftInt> = true,
          if_integral<RightInt> = true>
constexpr Qt::strong_ordering compareThreeWay(LeftInt lhs, RightInt rhs) noexcept
{
    static_assert(std::is_signed_v<LeftInt> == std::is_signed_v<RightInt>,
                  "Qt::compareThreeWay() does not allow mixed-sign comparison.");

#ifdef __cpp_lib_three_way_comparison
    return lhs <=> rhs;
#else
    if (lhs == rhs)
        return Qt::strong_ordering::equivalent;
    else if (lhs < rhs)
        return Qt::strong_ordering::less;
    else
        return Qt::strong_ordering::greater;
#endif // __cpp_lib_three_way_comparison
}

template <typename LeftFloat, typename RightFloat,
          if_floating_point<LeftFloat> = true,
          if_floating_point<RightFloat> = true>
constexpr Qt::partial_ordering compareThreeWay(LeftFloat lhs, RightFloat rhs) noexcept
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE
#ifdef __cpp_lib_three_way_comparison
    return lhs <=> rhs;
#else
    if (lhs < rhs)
        return Qt::partial_ordering::less;
    else if (lhs > rhs)
        return Qt::partial_ordering::greater;
    else if (lhs == rhs)
        return Qt::partial_ordering::equivalent;
    else
        return Qt::partial_ordering::unordered;
#endif // __cpp_lib_three_way_comparison
QT_WARNING_POP
}

template <typename IntType, typename FloatType,
          if_integral<IntType> = true,
          if_floating_point<FloatType> = true>
constexpr Qt::partial_ordering compareThreeWay(IntType lhs, FloatType rhs) noexcept
{
    return compareThreeWay(FloatType(lhs), rhs);
}

template <typename FloatType, typename IntType,
          if_floating_point<FloatType> = true,
          if_integral<IntType> = true>
constexpr Qt::partial_ordering compareThreeWay(FloatType lhs, IntType rhs) noexcept
{
    return compareThreeWay(lhs, FloatType(rhs));
}

#if QT_DEPRECATED_SINCE(6, 8)

template <typename LeftType, typename RightType,
          if_compatible_pointers<LeftType, RightType> = true>
QT_DEPRECATED_VERSION_X_6_8("Wrap the pointers into Qt::totally_ordered_wrapper and use the respective overload instead.")
constexpr Qt::strong_ordering compareThreeWay(const LeftType *lhs, const RightType *rhs) noexcept
{
#ifdef __cpp_lib_three_way_comparison
    return std::compare_three_way{}(lhs, rhs);
#else
    if (lhs == rhs)
        return Qt::strong_ordering::equivalent;
    else if (std::less<>{}(lhs, rhs))
        return Qt::strong_ordering::less;
    else
        return Qt::strong_ordering::greater;
#endif // __cpp_lib_three_way_comparison
}

template <typename T>
QT_DEPRECATED_VERSION_X_6_8("Wrap the pointer into Qt::totally_ordered_wrapper and use the respective overload instead.")
constexpr Qt::strong_ordering compareThreeWay(const T *lhs, std::nullptr_t rhs) noexcept
{
    return compareThreeWay(lhs, static_cast<const T *>(rhs));
}

template <typename T>
QT_DEPRECATED_VERSION_X_6_8("Wrap the pointer into Qt::totally_ordered_wrapper and use the respective overload instead.")
constexpr Qt::strong_ordering compareThreeWay(std::nullptr_t lhs, const T *rhs) noexcept
{
    return compareThreeWay(static_cast<const T *>(lhs), rhs);
}

#endif // QT_DEPRECATED_SINCE(6, 8)

template <class Enum, if_enum<Enum> = true>
constexpr Qt::strong_ordering compareThreeWay(Enum lhs, Enum rhs) noexcept
{
    return compareThreeWay(qToUnderlying(lhs), qToUnderlying(rhs));
}
} // namespace Qt

namespace QtOrderingPrivate {

template <typename Head, typename...Tail, std::size_t...Is>
constexpr std::tuple<Tail...> qt_tuple_pop_front_impl(const std::tuple<Head, Tail...> &t,
                                                      std::index_sequence<Is...>) noexcept
{
    return std::tuple<Tail...>(std::get<Is + 1>(t)...);
}

template <typename Head, typename...Tail>
constexpr std::tuple<Tail...> qt_tuple_pop_front(const std::tuple<Head, Tail...> &t) noexcept
{
    return qt_tuple_pop_front_impl(t, std::index_sequence_for<Tail...>{});
}

template <typename LhsHead, typename...LhsTail, typename RhsHead, typename...RhsTail>
constexpr auto compareThreeWayMulti(const std::tuple<LhsHead, LhsTail...> &lhs, // ie. not empty
                                    const std::tuple<RhsHead, RhsTail...> &rhs) noexcept
{
    static_assert(sizeof...(LhsTail) == sizeof...(RhsTail),
                                                            // expanded together below, but provide a nicer error message:
                  "The tuple arguments have to have the same size.");

    using Qt::compareThreeWay;
    using R = std::common_type_t<
            decltype(compareThreeWay(std::declval<LhsHead>(), std::declval<RhsHead>())),
            decltype(compareThreeWay(std::declval<LhsTail>(), std::declval<RhsTail>()))...
            >;

    const auto &l = std::get<0>(lhs);
    const auto &r = std::get<0>(rhs);
    static_assert(noexcept(compareThreeWay(l, r)),
                  "This function requires all relational operators to be noexcept.");
    const auto res = compareThreeWay(l, r);
    if constexpr (sizeof...(LhsTail) > 0) {
        if (is_eq(res))
            return R{compareThreeWayMulti(qt_tuple_pop_front(lhs), qt_tuple_pop_front(rhs))};
    }
    return R{res};
}

} //QtOrderingPrivate

namespace Qt {
// A wrapper class that adapts the wrappee to use the strongly-ordered
// <functional> function objects for implementing the relational operators.
// Mostly useful to avoid UB on pointers (which it currently mandates P to be),
// because all the comparison helpers (incl. std::compare_three_way on
// std::tuple<T*>!) will use the language-level operators.
//
template <typename P>
class totally_ordered_wrapper
{
    static_assert(std::is_pointer_v<P>);
    using T = std::remove_pointer_t<P>;

    P ptr;
public:
    totally_ordered_wrapper() noexcept = default;
    Q_IMPLICIT constexpr totally_ordered_wrapper(std::nullptr_t)
        // requires std::is_pointer_v<P>
        : totally_ordered_wrapper(P{nullptr}) {}
    explicit constexpr totally_ordered_wrapper(P p) noexcept : ptr(p) {}

    constexpr P get() const noexcept { return ptr; }
    constexpr void reset(P p) noexcept { ptr = p; }
    constexpr P operator->() const noexcept { return get(); }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    constexpr U &operator*() const noexcept { return *get(); }

    explicit constexpr operator bool() const noexcept { return get(); }

private:
    // TODO: Replace the constraints with std::common_type_t<P, U> when
    // a bug in VxWorks is fixed!
    template <typename T, typename U>
    using if_compatible_types =
            std::enable_if_t<std::conjunction_v<std::is_pointer<T>,
                                                std::is_pointer<U>,
                                                std::disjunction<std::is_convertible<T, U>,
                                                                 std::is_convertible<U, T>>>,
                             bool>;

#define MAKE_RELOP(Ret, op, Op) \
    template <typename U = P, if_compatible_types<P, U> = true> \
    friend constexpr Ret operator op (const totally_ordered_wrapper<P> &lhs, const totally_ordered_wrapper<U> &rhs) noexcept \
    { return std:: Op {}(lhs.ptr, rhs.get()); } \
    template <typename U = P, if_compatible_types<P, U> = true> \
    friend constexpr Ret operator op (const totally_ordered_wrapper<P> &lhs, const U &rhs) noexcept \
    { return std:: Op {}(lhs.ptr, rhs    ); } \
    template <typename U = P, if_compatible_types<P, U> = true> \
    friend constexpr Ret operator op (const U &lhs, const totally_ordered_wrapper<P> &rhs) noexcept \
    { return std:: Op {}(lhs,     rhs.ptr); } \
    friend constexpr Ret operator op (const totally_ordered_wrapper &lhs, std::nullptr_t) noexcept \
    { return std:: Op {}(lhs.ptr, P(nullptr)); } \
    friend constexpr Ret operator op (std::nullptr_t, const totally_ordered_wrapper &rhs) noexcept \
    { return std:: Op {}(P(nullptr), rhs.ptr); } \
    /* end */
    MAKE_RELOP(bool, ==, equal_to<>)
    MAKE_RELOP(bool, !=, not_equal_to<>)
    MAKE_RELOP(bool, < , less<>)
    MAKE_RELOP(bool, <=, less_equal<>)
    MAKE_RELOP(bool, > , greater<>)
    MAKE_RELOP(bool, >=, greater_equal<>)
#ifdef __cpp_lib_three_way_comparison
    MAKE_RELOP(auto, <=>, compare_three_way)
#endif
#undef MAKE_RELOP
    friend void qt_ptr_swap(totally_ordered_wrapper &lhs, totally_ordered_wrapper &rhs) noexcept
    { qt_ptr_swap(lhs.ptr, rhs.ptr); }
    friend void swap(totally_ordered_wrapper &lhs, totally_ordered_wrapper &rhs) noexcept
    { qt_ptr_swap(lhs, rhs); }
    friend size_t qHash(totally_ordered_wrapper key, size_t seed = 0) noexcept
    { return qHash(key.ptr, seed); }
};

template <typename T, typename U, if_compatible_pointers<T, U> = true>
constexpr Qt::strong_ordering
compareThreeWay(Qt::totally_ordered_wrapper<T*> lhs, Qt::totally_ordered_wrapper<U*> rhs) noexcept
{
    return QtOrderingPrivate::strongOrderingCompareDefaultImpl(lhs, rhs);
}

template <typename T, typename U, if_compatible_pointers<T, U> = true>
constexpr Qt::strong_ordering
compareThreeWay(Qt::totally_ordered_wrapper<T*> lhs, U *rhs) noexcept
{
    return QtOrderingPrivate::strongOrderingCompareDefaultImpl(lhs, rhs);
}

template <typename T, typename U, if_compatible_pointers<T, U> = true>
constexpr Qt::strong_ordering
compareThreeWay(U *lhs, Qt::totally_ordered_wrapper<T*> rhs) noexcept
{
    return QtOrderingPrivate::strongOrderingCompareDefaultImpl(lhs, rhs);
}

template <typename T>
constexpr Qt::strong_ordering
compareThreeWay(Qt::totally_ordered_wrapper<T*> lhs, std::nullptr_t rhs) noexcept
{
    return QtOrderingPrivate::strongOrderingCompareDefaultImpl(lhs, rhs);
}

template <typename T>
constexpr Qt::strong_ordering
compareThreeWay(std::nullptr_t lhs, Qt::totally_ordered_wrapper<T*> rhs) noexcept
{
    return QtOrderingPrivate::strongOrderingCompareDefaultImpl(lhs, rhs);
}

} //Qt

template <typename P>
class QTypeInfo<Qt::totally_ordered_wrapper<P>> : public QTypeInfo<P> {};

namespace QtOrderingPrivate {

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED // don't warn _here_ in case we hit the deprecated ptr/ptr overloads

namespace CompareThreeWayTester {

using Qt::compareThreeWay;

template <typename T>
using WrappedType = std::conditional_t<std::is_pointer_v<T>, Qt::totally_ordered_wrapper<T>, T>;

// Check if compareThreeWay is implemented for the (LT, RT) argument
// pair.
template <typename LT, typename RT, typename = void>
struct HasCompareThreeWay : std::false_type {};

template <typename LT, typename RT>
struct HasCompareThreeWay<
        LT, RT, std::void_t<decltype(compareThreeWay(std::declval<LT>(), std::declval<RT>()))>
    > : std::true_type {};

template <typename LT, typename RT>
struct HasCompareThreeWay<
        LT*, RT*,
        std::void_t<decltype(compareThreeWay(std::declval<WrappedType<LT>>(),
                                             std::declval<WrappedType<RT>>()))>
    > : std::true_type {};

template <typename LT, typename RT>
constexpr inline bool hasCompareThreeWay_v = HasCompareThreeWay<LT, RT>::value;

// Check if the operation is noexcept. We have two different overloads,
// depending on the available compareThreeWay() implementation.
// Both are declared, but not implemented. To be used only in unevaluated
// context.

template <typename LT, typename RT,
          std::enable_if_t<hasCompareThreeWay_v<LT, RT>, bool> = true>
constexpr bool compareThreeWayNoexcept() noexcept
{ return noexcept(compareThreeWay(std::declval<LT>(), std::declval<RT>())); }

template <typename LT, typename RT,
          std::enable_if_t<std::conjunction_v<std::negation<HasCompareThreeWay<LT, RT>>,
                                              HasCompareThreeWay<RT, LT>>,
                           bool> = true>
constexpr bool compareThreeWayNoexcept() noexcept
{ return noexcept(compareThreeWay(std::declval<RT>(), std::declval<LT>())); }

} // namespace CompareThreeWayTester

QT_WARNING_POP // QT_WARNING_DISABLE_DEPRECATED

#ifdef __cpp_lib_three_way_comparison
[[maybe_unused]] inline constexpr struct { /* Niebloid */
    template <typename LT, typename RT = LT>
    [[maybe_unused]] constexpr auto operator()(const LT &lhs, const RT &rhs) const
    {
        // like [expos.only.entity]/2
        if constexpr (QTypeTraits::has_operator_compare_three_way_with_v<LT, RT>) {
            return lhs <=> rhs;
        } else {
            if (lhs < rhs)
                return std::weak_ordering::less;
            if (rhs < lhs)
                return std::weak_ordering::greater;
            return std::weak_ordering::equivalent;
        }
    }
} synthThreeWay;

template <typename Container, typename T>
using if_has_op_less_or_op_compare_three_way =
        std::enable_if_t<
                std::disjunction_v<QTypeTraits::has_operator_less_than_container<Container, T>,
                                   QTypeTraits::has_operator_compare_three_way<T>>,
                bool>;
#endif // __cpp_lib_three_way_comparison

// These checks do not use Qt::compareThreeWay(), so only work for user-defined
// compareThreeWay() helper functions.
// We cannot use the same condition as in CompareThreeWayTester::hasCompareThreeWay,
// because GCC seems to cache and re-use the result.
// Created https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117174
// For now, modify the condition a bit without changing its meaning.
template <typename LT, typename RT = LT, typename = void>
struct HasCustomCompareThreeWay : std::false_type {};

template <typename LT, typename RT>
struct HasCustomCompareThreeWay<
        LT, RT,
        std::void_t<decltype(is_eq(compareThreeWay(std::declval<LT>(), std::declval<RT>())))>
    > : std::true_type {};

template <typename InputIt1, typename InputIt2, typename Compare>
auto lexicographicalCompareThreeWay(InputIt1 first1, InputIt1 last1,
                                    InputIt2 first2, InputIt2 last2,
                                    Compare cmp)
{
    using R = decltype(cmp(*first1, *first2));

    while (first1 != last1) {
        if (first2 == last2)
            return R::greater;
        const auto r = cmp(*first1, *first2);
        if (is_neq(r))
            return r;
        ++first1;
        ++first2;
    }
    return first2 == last2 ? R::equivalent : R::less;
}

template <typename InputIt1, typename InputIt2>
auto lexicographicalCompareThreeWay(InputIt1 first1, InputIt1 last1,
                                    InputIt2 first2, InputIt2 last2)
{
    using LT = typename std::iterator_traits<InputIt1>::value_type;
    using RT = typename std::iterator_traits<InputIt2>::value_type;

    // if LT && RT are pointers, and there is no user-defined compareThreeWay()
    // operation for the pointers, we need to wrap them into
    // Qt::totally_ordered_wrapper.
    constexpr bool UseWrapper =
            std::conjunction_v<std::is_pointer<LT>, std::is_pointer<RT>,
                               std::negation<HasCustomCompareThreeWay<LT, RT>>,
                               std::negation<HasCustomCompareThreeWay<RT, LT>>>;
    using WrapLT = std::conditional_t<UseWrapper,
                                      Qt::totally_ordered_wrapper<LT>,
                                      const LT &>;
    using WrapRT = std::conditional_t<UseWrapper,
                                      Qt::totally_ordered_wrapper<RT>,
                                      const RT &>;

    auto cmp = [](LT const &lhs, RT const &rhs) {
        using Qt::compareThreeWay;
        namespace Test = QtOrderingPrivate::CompareThreeWayTester;
        // Need this because the user might provide only
        // compareThreeWay(LT, RT), but not the reversed version.
        if constexpr (Test::hasCompareThreeWay_v<WrapLT, WrapRT>)
            return compareThreeWay(WrapLT(lhs), WrapRT(rhs));
        else
            return QtOrderingPrivate::reversed(compareThreeWay(WrapRT(rhs), WrapLT(lhs)));
    };
    return lexicographicalCompareThreeWay(first1, last1, first2, last2, cmp);
}

} // namespace QtOrderingPrivate

namespace Qt {

template <typename T, typename U>
using if_has_qt_compare_three_way =
        std::enable_if_t<
            std::disjunction_v<QtOrderingPrivate::CompareThreeWayTester::HasCompareThreeWay<T, U>,
                               QtOrderingPrivate::CompareThreeWayTester::HasCompareThreeWay<U, T>>,
        bool>;

} // namespace Qt

QT_END_NAMESPACE

namespace std {
    template <typename P>
    struct hash<QT_PREPEND_NAMESPACE(Qt::totally_ordered_wrapper)<P>>
    {
        using argument_type = QT_PREPEND_NAMESPACE(Qt::totally_ordered_wrapper)<P>;
        using result_type = size_t;
        constexpr result_type operator()(argument_type w) const noexcept
        { return std::hash<P>{}(w.get()); }
    };
}

#endif // QCOMPAREHELPERS_H
