// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMFUNCTIONREF_P_H
#define QQMLDOMFUNCTIONREF_P_H

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

#include <QtCore/private/qglobal_p.h>

#if !defined(Q_CC_MSVC) || Q_CC_MSVC >= 1930
#include <QtCore/qxpfunctional.h>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
template <typename T>
using function_ref = qxp::function_ref<T>;
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#else

#include <functional>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {
namespace _detail {
template <typename T>
struct function_ref_helper { using type = std::function<T>; };
// std::function doesn't grok the const in <int(int) const>, so remove:
template <typename R, typename...Args>
struct function_ref_helper<R(Args...) const> : function_ref_helper<R(Args...)> {};
// std::function doesn't grok the noexcept in <int(int) noexcept>, so remove:
template <typename R, typename...Args>
struct function_ref_helper<R(Args...) noexcept> : function_ref_helper<R(Args...)> {};
// and both together:
template <typename R, typename...Args>
struct function_ref_helper<R(Args...) const noexcept> : function_ref_helper<R(Args...)> {};
} // namespace _detail
template <typename T>
using function_ref = const typename _detail::function_ref_helper<T>::type &;
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif

#endif // QQMLDOMFUNCTIONREF_P_H
