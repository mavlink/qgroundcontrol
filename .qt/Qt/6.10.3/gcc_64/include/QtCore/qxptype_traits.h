// Copyright (C) 2023 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>, Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXPTYPE_TRAITS_H
#define QXPTYPE_TRAITS_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qcompilerdetection.h>

#include <QtCore/q23type_traits.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// like std::experimental::{nonesuch,is_detected/_v}(LFTSv2)
namespace qxp {

struct nonesuch {
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
};

namespace _detail {
    template <typename T, typename Void, template <typename...> class Op, typename...Args>
    struct detector {
        using value_t = std::false_type;
        using type = T;
    };
    template <typename T, template <typename...> class Op, typename...Args>
    struct detector<T, std::void_t<Op<Args...>>, Op, Args...> {
        using value_t = std::true_type;
        using type = Op<Args...>;
    };
} // namespace _detail

template <template <typename...> class Op, typename...Args>
using is_detected = typename _detail::detector<qxp::nonesuch, void, Op, Args...>::value_t;

template <template <typename...> class Op, typename...Args>
constexpr inline bool is_detected_v = qxp::is_detected<Op, Args...>::value;

template <typename Default, template <typename...> class Op, typename...Args>
using detected_or = _detail::detector<Default, void, Op, Args...>;

template <typename Default, template <typename...> class Op, typename...Args>
using detected_or_t = typename qxp::detected_or<Default, Op, Args...>::type;

template <template <typename...> class Op, typename...Args>
using detected_t = qxp::detected_or_t<qxp::nonesuch, Op, Args...>;

// qxp::is_virtual_base_of_v<B, D> is true if and only if B is a virtual base class of D.
// Just like is_base_of:
// * only works on complete types;
// * B and D must be class types;
// * ignores cv-qualifications;
// * B may be inaccessibile.

#ifdef __cpp_lib_is_virtual_base_of
using std::is_virtual_base_of;
using std::is_virtual_base_of_v;
#else
namespace _detail {
    // Check that From* can be converted to To*, ignoring accessibility.
    // This can be done using a C cast (see [expr.cast]/4).
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wold-style-cast")
QT_WARNING_DISABLE_CLANG("-Wold-style-cast")
    template <typename From, typename To>
    using is_virtual_base_conversion_test = decltype(
        (To *)std::declval<From *>()
    );
QT_WARNING_POP

    template <typename Base, typename Derived, typename = void>
    struct is_virtual_base_of : std::false_type {};

    template <typename Base, typename Derived>
    struct is_virtual_base_of<
        Base, Derived,
        std::enable_if_t<
            std::conjunction_v<
                // Base is a base class of Derived.
                std::is_base_of<Base, Derived>,

                // Check that Derived* can be converted to Base*, ignoring
                // accessibility. If this is possible, then Base is
                // an unambiguous base of Derived (=> virtual bases are always
                // unambiguous).
                qxp::is_detected<is_virtual_base_conversion_test, Derived, Base>,

                // Check that Base* can _not_ be converted to Derived*,
                // again ignoring accessibility. This seals the deal:
                // if this conversion cannot happen, it means that Base is an
                // ambiguous base and/or it is a virtual base.
                // But we have already established that Base is an unambiguous
                // base, hence: Base is a virtual base.
                std::negation<
                    qxp::is_detected<is_virtual_base_conversion_test, Base, Derived>
                >
            >
        >
    > : std::true_type {};
}

template <typename Base, typename Derived>
using is_virtual_base_of = _detail::is_virtual_base_of<std::remove_cv_t<Base>, std::remove_cv_t<Derived>>;

template <typename Base, typename Derived>
constexpr inline bool is_virtual_base_of_v = is_virtual_base_of<Base, Derived>::value;
#endif // __cpp_lib_is_virtual_base_of

} // namespace qxp

QT_END_NAMESPACE

#endif // QXPTYPE_TRAITS_H

