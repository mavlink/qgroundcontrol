// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMINMAX_H
#define QMINMAX_H

#if 0
#pragma qt_class(QtMinMax)
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qassert.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qttypetraits.h>

QT_BEGIN_NAMESPACE

template <typename T>
constexpr inline const T &qMin(const T &a, const T &b) { return (a < b) ? a : b; }
template <typename T>
constexpr inline const T &qMax(const T &a, const T &b) { return (a < b) ? b : a; }
template <typename T>
constexpr inline const T &qBound(const T &min, const T &val, const T &max)
{
    Q_ASSERT(!(max < min));
    return qMax(min, qMin(max, val));
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qMin(const T &a, const U &b)
{
    using P = QTypeTraits::Promoted<T, U>;
    P _a = a;
    P _b = b;
    return (_a < _b) ? _a : _b;
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qMax(const T &a, const U &b)
{
    using P = QTypeTraits::Promoted<T, U>;
    P _a = a;
    P _b = b;
    return (_a < _b) ? _b : _a;
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qBound(const T &min, const U &val, const T &max)
{
    Q_ASSERT(!(max < min));
    return qMax(min, qMin(max, val));
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qBound(const T &min, const T &val, const U &max)
{
    using P = QTypeTraits::Promoted<T, U>;
    Q_ASSERT(!(P(max) < P(min)));
    return qMax(min, qMin(max, val));
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qBound(const U &min, const T &val, const T &max)
{
    using P = QTypeTraits::Promoted<T, U>;
    Q_ASSERT(!(P(max) < P(min)));
    return qMax(min, qMin(max, val));
}

QT_END_NAMESPACE

#endif // QMINMAX_H
