// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTCORE_QSMALLBYTEARRAY_P_H
#define QTCORE_QSMALLBYTEARRAY_P_H

#include <QtCore/qbytearrayview.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtypes.h>

#include <array>
#include <limits>

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

QT_BEGIN_NAMESPACE

//
// A fixed-max-size version of QByteArray. Since it's fixed-max-size, it's
// never going to the heap. Can contain a maximum of 256 octets. Never
// NUL-terminates on its own.
//

template <size_t N>
class QSmallByteArray
{
    std::array<quint8, N> m_data;
    static_assert(N <= (std::numeric_limits<std::uint8_t>::max)());
    quint8 m_size = 0;
public:
    QSmallByteArray() = default;
    // all compiler-generated SMFs are ok!
    template <std::size_t M, std::enable_if_t<M < N, bool> = true> // M == N is for copy ctor!
    constexpr QSmallByteArray(const QSmallByteArray<M> &other) noexcept
    {
        assign(other);
    }
    template <std::size_t M, std::enable_if_t<M < N, bool> = true> // M == N is for copy-assignment op!
    constexpr QSmallByteArray &operator=(const QSmallByteArray<M> &other) noexcept
    {
        assign(other);
        return *this;
    }

    template <typename Container> // ### underconstrained
    constexpr void assign(const Container &c)
    {
        const size_t otherSize = size_t(std::size(c));
        Q_ASSERT(otherSize < N);
        memcpy(data(), std::data(c), otherSize);
        m_size = quint8(otherSize);
    }

    constexpr quint8 *data() noexcept { return m_data.data(); }
    constexpr const quint8 *data() const noexcept { return m_data.data(); }
    constexpr qsizetype size() const noexcept { return qsizetype{m_size}; }
    constexpr quint8 &operator[](qsizetype n)
    {
        Q_ASSERT(n < size());
        return data()[n];
    }
    constexpr const quint8 &operator[](qsizetype n) const
    {
        Q_ASSERT(n < size());
        return data()[n];
    }
    constexpr bool isEmpty() const noexcept { return size() == 0; }
    constexpr void clear() noexcept { m_size = 0; }
    constexpr void resizeForOverwrite(qsizetype s)
    {
        Q_ASSERT(s >= 0);
        Q_ASSERT(size_t(s) <= N);
        m_size = std::uint8_t(s);
    }
    constexpr void resize(qsizetype s, quint8 v)
    {
        const auto oldSize = size();
        resizeForOverwrite(s);
        if (s > oldSize)
            memset(data() + oldSize, v, size() - oldSize);
    }
    constexpr QByteArrayView toByteArrayView() const noexcept
    { return *this; }

    constexpr auto begin() noexcept { return data(); }
    constexpr auto begin() const noexcept { return data(); }
    constexpr auto cbegin() const noexcept { return begin(); }
    constexpr auto end() noexcept { return data() + size(); }
    constexpr auto end() const noexcept { return data() + size(); }
    constexpr auto cend() const noexcept { return end(); }
};

QT_END_NAMESPACE

#endif // QTCORE_QSMALLBYTEARRAY_P_H
