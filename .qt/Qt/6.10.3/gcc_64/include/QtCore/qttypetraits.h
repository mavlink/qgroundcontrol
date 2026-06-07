// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTTYPETRAITS_H
#define QTTYPETRAITS_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtdeprecationmarkers.h>

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
#include <compare>
#include <concepts>
#endif
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#if 0
#pragma qt_class(QtTypeTraits)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

// like std::to_underlying
template <typename Enum>
constexpr std::underlying_type_t<Enum> qToUnderlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#ifndef QT_NO_QASCONST
#if QT_DEPRECATED_SINCE(6, 6)

// this adds const to non-const objects (like std::as_const)
template <typename T>
QT_DEPRECATED_VERSION_X_6_6("Use std::as_const() instead.")
constexpr typename std::add_const<T>::type &qAsConst(T &t) noexcept { return t; }
// prevent rvalue arguments:
template <typename T>
void qAsConst(const T &&) = delete;

#endif // QT_DEPRECATED_SINCE(6, 6)
#endif // QT_NO_QASCONST

#ifndef QT_NO_QEXCHANGE

// like std::exchange
template <typename T, typename U = T>
constexpr T qExchange(T &t, U &&newValue)
noexcept(std::conjunction_v<std::is_nothrow_move_constructible<T>,
                            std::is_nothrow_assignable<T &, U>>)
{
    T old = std::move(t);
    t = std::forward<U>(newValue);
    return old;
}

#endif // QT_NO_QEXCHANGE

namespace QtPrivate {
// helper to be used to trigger a "dependent static_assert(false)"
// (for instance, in a final `else` branch of a `if constexpr`.)
template <typename T> struct type_dependent_false : std::false_type {};
template <auto T> struct value_dependent_false : std::false_type {};

// helper detects standard integer types and some of extended integer types,
// see https://eel.is/c++draft/basic.fundamental#1
template <typename T> struct is_standard_or_extended_integer_type_helper : std::is_integral<T> {};
// these are integral, but not considered standard or extended integer types
// https://eel.is/c++draft/basic.fundamental#11:
#define QSEIT_EXCLUDE(X) \
    template <> struct is_standard_or_extended_integer_type_helper<X> : std::false_type {}
QSEIT_EXCLUDE(bool);
QSEIT_EXCLUDE(char);
#ifdef __cpp_char8_t
QSEIT_EXCLUDE(char8_t);
#endif
QSEIT_EXCLUDE(char16_t);
QSEIT_EXCLUDE(char32_t);
QSEIT_EXCLUDE(wchar_t);
#undef QSEIT_EXCLUDE
template <typename T>
struct is_standard_or_extended_integer_type : is_standard_or_extended_integer_type_helper<std::remove_cv_t<T>> {};
template <typename T>
constexpr bool is_standard_or_extended_integer_type_v = is_standard_or_extended_integer_type<T>::value;
} // QtPrivate

namespace QTypeTraits {

namespace detail {
template<typename T, typename U,
         typename = std::enable_if_t<std::is_arithmetic_v<T> && std::is_arithmetic_v<U> &&
                                     std::is_floating_point_v<T> == std::is_floating_point_v<U> &&
                                     std::is_signed_v<T> == std::is_signed_v<U> &&
                                     !std::is_same_v<T, bool> && !std::is_same_v<U, bool> &&
                                     !std::is_same_v<T, char> && !std::is_same_v<U, char>>>
struct Promoted
{
    using type = decltype(T() + U());
};
}

template <typename T, typename U>
using Promoted = typename detail::Promoted<T, U>::type;

/*
    The templates below aim to find out whether one can safely instantiate an operator==() or
    operator<() for a type.

    This is tricky for containers, as most containers have unconstrained comparison operators, even though they
    rely on the corresponding operators for its content.
    This is especially true for all of the STL template classes that have a comparison operator defined, and
    leads to the situation, that the compiler would try to instantiate the operator, and fail if any
    of its template arguments does not have the operator implemented.

    The code tries to cover the relevant cases for Qt and the STL, by checking (recusrsively) the value_type
    of a container (if it exists), and checking the template arguments of pair, tuple and variant.
*/
namespace detail {

// find out whether T is a conteiner
// this is required to check the value type of containers for the existence of the comparison operator
template <typename, typename = void>
struct is_container : std::false_type {};
template <typename T>
struct is_container<T, std::void_t<
        typename T::value_type,
        std::is_convertible<decltype(std::declval<T>().begin() != std::declval<T>().end()), bool>
>> : std::true_type {};


// Checks the existence of the comparison operator for the class itself
QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE
template <typename, typename = void>
struct has_operator_equal : std::false_type {};
template <typename T>
struct has_operator_equal<T, std::void_t<decltype(bool(std::declval<const T&>() == std::declval<const T&>()))>>
        : std::true_type {};
QT_WARNING_POP

// Two forward declarations
template<typename T, bool = is_container<T>::value>
struct expand_operator_equal_container;
template<typename T>
struct expand_operator_equal_tuple;

// the entry point for the public method
template<typename T>
using expand_operator_equal = expand_operator_equal_container<T>;

// if T isn't a container check if it's a tuple like object
template<typename T, bool>
struct expand_operator_equal_container : expand_operator_equal_tuple<T> {};
// if T::value_type exists, check first T::value_type, then T itself
template<typename T>
struct expand_operator_equal_container<T, true> :
        std::conjunction<
        std::disjunction<
            std::is_same<T, typename T::value_type>, // avoid endless recursion
            expand_operator_equal<typename T::value_type>
        >, expand_operator_equal_tuple<T>> {};

// recursively check the template arguments of a tuple like object
template<typename ...T>
using expand_operator_equal_recursive = std::conjunction<expand_operator_equal<T>...>;

template<typename T>
struct expand_operator_equal_tuple : has_operator_equal<T> {};
template<typename T>
struct expand_operator_equal_tuple<std::optional<T>> : expand_operator_equal_recursive<T> {};
template<typename T1, typename T2>
struct expand_operator_equal_tuple<std::pair<T1, T2>> : expand_operator_equal_recursive<T1, T2> {};
template<typename ...T>
struct expand_operator_equal_tuple<std::tuple<T...>> : expand_operator_equal_recursive<T...> {};
template<typename ...T>
struct expand_operator_equal_tuple<std::variant<T...>> : expand_operator_equal_recursive<T...> {};

// the same for operator<(), see above for explanations
template <typename, typename = void>
struct has_operator_less_than : std::false_type{};
template <typename T>
struct has_operator_less_than<T, std::void_t<decltype(bool(std::declval<const T&>() < std::declval<const T&>()))>>
        : std::true_type{};

template<typename T, bool = is_container<T>::value>
struct expand_operator_less_than_container;
template<typename T>
struct expand_operator_less_than_tuple;

template<typename T>
using expand_operator_less_than = expand_operator_less_than_container<T>;

template<typename T, bool>
struct expand_operator_less_than_container : expand_operator_less_than_tuple<T> {};
template<typename T>
struct expand_operator_less_than_container<T, true> :
        std::conjunction<
            std::disjunction<
                std::is_same<T, typename T::value_type>,
                expand_operator_less_than<typename T::value_type>
            >, expand_operator_less_than_tuple<T>
        > {};

template<typename ...T>
using expand_operator_less_than_recursive = std::conjunction<expand_operator_less_than<T>...>;

template<typename T>
struct expand_operator_less_than_tuple : has_operator_less_than<T> {};
template<typename T>
struct expand_operator_less_than_tuple<std::optional<T>> : expand_operator_less_than_recursive<T> {};
template<typename T1, typename T2>
struct expand_operator_less_than_tuple<std::pair<T1, T2>> : expand_operator_less_than_recursive<T1, T2> {};
template<typename ...T>
struct expand_operator_less_than_tuple<std::tuple<T...>> : expand_operator_less_than_recursive<T...> {};
template<typename ...T>
struct expand_operator_less_than_tuple<std::variant<T...>> : expand_operator_less_than_recursive<T...> {};

} // namespace detail

template<typename T, typename = void>
struct is_dereferenceable : std::false_type {};

template<typename T>
struct is_dereferenceable<T, std::void_t<decltype(std::declval<T>().operator->())> >
    : std::true_type {};

template <typename T>
inline constexpr bool is_dereferenceable_v = is_dereferenceable<T>::value;

template<typename T>
struct has_operator_equal : detail::expand_operator_equal<T> {};
template<typename T>
inline constexpr bool has_operator_equal_v = has_operator_equal<T>::value;

template <typename Container, typename T>
using has_operator_equal_container = std::disjunction<std::is_base_of<Container, T>, QTypeTraits::has_operator_equal<T>>;

template<typename T>
struct has_operator_less_than : detail::expand_operator_less_than<T> {};
template<typename T>
inline constexpr bool has_operator_less_than_v = has_operator_less_than<T>::value;

template <typename Container, typename T>
using has_operator_less_than_container = std::disjunction<std::is_base_of<Container, T>, QTypeTraits::has_operator_less_than<T>>;

template <typename ...T>
using compare_eq_result = std::enable_if_t<std::conjunction_v<QTypeTraits::has_operator_equal<T>...>, bool>;

template <typename Container, typename ...T>
using compare_eq_result_container = std::enable_if_t<std::conjunction_v<QTypeTraits::has_operator_equal_container<Container, T>...>, bool>;

template <typename ...T>
using compare_lt_result = std::enable_if_t<std::conjunction_v<QTypeTraits::has_operator_less_than<T>...>, bool>;

template <typename Container, typename ...T>
using compare_lt_result_container = std::enable_if_t<std::conjunction_v<QTypeTraits::has_operator_less_than_container<Container, T>...>, bool>;

template<typename T>
struct has_operator_compare_three_way : std::false_type {};
template <typename T, typename U>
struct has_operator_compare_three_way_with : std::false_type {};
#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
template<std::three_way_comparable T>
struct has_operator_compare_three_way<T> : std::true_type {};
template <typename T, typename U>
    requires std::three_way_comparable_with<T, U>
struct has_operator_compare_three_way_with<T, U> : std::true_type {};
#endif // __cpp_lib_three_way_comparison && __cpp_lib_concepts
template<typename T>
constexpr inline bool has_operator_compare_three_way_v = has_operator_compare_three_way<T>::value;
template<typename T, typename U>
constexpr inline bool has_operator_compare_three_way_with_v = has_operator_compare_three_way_with<T, U>::value;

// Intentionally no 'has_operator_compare_three_way_container', because the
// compilers fail to determine the proper return type in this case
// template <typename Container, typename T>
// using has_operator_compare_three_way_container =
//         std::disjunction<std::is_base_of<Container, T>, has_operator_compare_three_way<T>>;

namespace detail {

template<typename T>
const T &const_reference();
template<typename T>
T &reference();

}

template <typename Stream, typename, typename = void>
struct has_ostream_operator : std::false_type {};
template <typename Stream, typename T>
struct has_ostream_operator<Stream, T, std::void_t<decltype(detail::reference<Stream>() << detail::const_reference<T>())>>
        : std::true_type {};
template <typename Stream, typename T>
inline constexpr bool has_ostream_operator_v = has_ostream_operator<Stream, T>::value;

template <typename Stream, typename Container, typename T>
using has_ostream_operator_container = std::disjunction<std::is_base_of<Container, T>, QTypeTraits::has_ostream_operator<Stream, T>>;

template <typename Stream, typename, typename = void>
struct has_istream_operator : std::false_type {};
template <typename Stream, typename T>
struct has_istream_operator<Stream, T, std::void_t<decltype(detail::reference<Stream>() >> detail::reference<T>())>>
        : std::true_type {};
template <typename Stream, typename T>
inline constexpr bool has_istream_operator_v = has_istream_operator<Stream, T>::value;
template <typename Stream, typename Container, typename T>
using has_istream_operator_container = std::disjunction<std::is_base_of<Container, T>, QTypeTraits::has_istream_operator<Stream, T>>;

template <typename Stream, typename T>
inline constexpr bool has_stream_operator_v = has_ostream_operator_v<Stream, T> && has_istream_operator_v<Stream, T>;

} // namespace QTypeTraits

QT_END_NAMESPACE

#endif // QTTYPETRAITS_H
