// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMAKEARRAY_P_H
#define QMAKEARRAY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/private/qglobal_p.h"

#include <array>
#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
template<typename T>
constexpr T &&Forward(typename std::remove_reference<T>::type &t) noexcept
{
    return static_cast<T &&>(t);
}

template<typename T>
constexpr T &&Forward(typename std::remove_reference<T>::type &&t) noexcept
{
    static_assert(!std::is_lvalue_reference<T>::value,
                  "template argument substituting T is an lvalue reference type");
    return static_cast<T &&>(t);
}

template <typename ManualType, typename ...>
struct ArrayTypeHelper
{
    using type = ManualType;
};

template <typename ... Types>
struct ArrayTypeHelper<void, Types...> : std::common_type<Types...> { };

template <typename ManualType, typename... Types>
using ArrayType = std::array<typename ArrayTypeHelper<ManualType, Types...>::type,
                             sizeof...(Types)>;

template<typename ... Values>
struct QuickSortData { };

template <template <typename> class Predicate,
          typename ... Values>
struct QuickSortFilter;

template <typename ... Right, typename ... Left>
constexpr QuickSortData<Right..., Left...> quickSortConcat(
    QuickSortData<Right...>, QuickSortData<Left...>) noexcept;

template<typename ... Right, typename Middle, typename ... Left>
constexpr QuickSortData<Right..., Middle, Left...> quickSortConcat(
    QuickSortData<Right...>,
    QuickSortData<Middle>,
    QuickSortData<Left...>) noexcept;

template <template <typename> class Predicate,
          typename Head, typename ... Tail>
struct QuickSortFilter<Predicate, QuickSortData<Head, Tail...>>
{
    using TailFilteredData = typename QuickSortFilter<
        Predicate, QuickSortData<Tail...>>::Type;

    using Type = typename std::conditional<
        Predicate<Head>::value,
        decltype(quickSortConcat(QuickSortData<Head> {}, TailFilteredData{})),
        TailFilteredData>::type;
};

template <template <typename> class Predicate>
struct QuickSortFilter<Predicate, QuickSortData<>>
{
    using Type = QuickSortData<>;
};

template <typename ... Values>
struct QuickSort;

template <typename Pivot, typename ... Values>
struct QuickSort<QuickSortData<Pivot, Values...>>
{
    template <typename Left>
    struct LessThan {
        static constexpr const bool value = Left::data() <= Pivot::data();
    };

    template <typename Left>
    struct MoreThan {
        static constexpr const bool value = !(Left::data() <= Pivot::data());
    };

    using LeftSide = typename QuickSortFilter<LessThan, QuickSortData<Values...>>::Type;
    using RightSide = typename QuickSortFilter<MoreThan, QuickSortData<Values...>>::Type;

    using LeftQS = typename QuickSort<LeftSide>::Type;
    using RightQS = typename QuickSort<RightSide>::Type;

    using Type = decltype(quickSortConcat(LeftQS{}, QuickSortData<Pivot> {}, RightQS{}));

};

template <>
struct QuickSort<QuickSortData<>>
{
    using Type = QuickSortData<>;
};
} // namespace QtPrivate

template <typename ManualType = void, typename ... Types>
constexpr QtPrivate::ArrayType<ManualType, Types...> qMakeArray(Types && ... t) noexcept
{
    return {{QtPrivate::Forward<typename QtPrivate::ArrayType<ManualType, Types...>::value_type>(t)...}};
}

template<typename ... Values>
struct QSortedData {
    using Data = typename QtPrivate::QuickSort<typename QtPrivate::QuickSortData<Values...>>::Type;
};

template<typename ... Values>
constexpr auto qMakeArray(QtPrivate::QuickSortData<Values...>) noexcept -> decltype(qMakeArray(Values::data()...))
{
    return qMakeArray(Values::data() ...);
}


QT_END_NAMESPACE

#endif // QMAKEARRAY_P_H
