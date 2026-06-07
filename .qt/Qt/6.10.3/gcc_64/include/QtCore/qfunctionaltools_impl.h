// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QFUNCTIONALTOOLS_IMPL_H
#define QFUNCTIONALTOOLS_IMPL_H

#include <QtCore/qtconfigmacros.h>

#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

namespace detail {

#define FOR_EACH_CVREF(op) \
    op(&) \
    op(const &) \
    op(&&) \
    op(const &&) \
    /* end */


template <typename Object, typename = void>
struct StorageByValue
{
    Object o;
#define MAKE_GETTER(cvref) \
    constexpr Object cvref object() cvref noexcept \
    { return static_cast<Object cvref>(o); }
    FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
};

template <typename Object, typename Tag = void>
struct StorageEmptyBaseClassOptimization : Object
{
    StorageEmptyBaseClassOptimization() = default;
    StorageEmptyBaseClassOptimization(Object &&o)
        : Object(std::move(o))
    {}
    StorageEmptyBaseClassOptimization(const Object &o)
        : Object(o)
    {}

#define MAKE_GETTER(cvref) \
    constexpr Object cvref object() cvref noexcept \
    { return static_cast<Object cvref>(*this); }
    FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
};
} // namespace detail

template <typename Object, typename Tag = void>
using CompactStorage = typename std::conditional_t<
        std::conjunction_v<
            std::is_empty<Object>,
            std::negation<std::is_final<Object>>
        >,
        detail::StorageEmptyBaseClassOptimization<Object, Tag>,
        detail::StorageByValue<Object, Tag>
    >;

} // namespace QtPrivate

#undef FOR_EACH_CVREF

QT_END_NAMESPACE

#endif // QFUNCTIONALTOOLS_IMPL_H
