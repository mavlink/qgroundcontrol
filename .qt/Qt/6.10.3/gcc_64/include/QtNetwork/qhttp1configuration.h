// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTP1CONFIGURATION_H
#define QHTTP1CONFIGURATION_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtypes.h>
#include <QtCore/qtypeinfo.h>

#include <utility>
#include <cstdint>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttp1ConfigurationPrivate;
class QHttp1Configuration
{
public:
    Q_NETWORK_EXPORT QHttp1Configuration();
    Q_NETWORK_EXPORT QHttp1Configuration(const QHttp1Configuration &other);
    QHttp1Configuration(QHttp1Configuration &&other) noexcept
        : u{other.u} { other.u.d = nullptr; }

    Q_NETWORK_EXPORT QHttp1Configuration &operator=(const QHttp1Configuration &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QHttp1Configuration)

    Q_NETWORK_EXPORT ~QHttp1Configuration();

    Q_NETWORK_EXPORT void setNumberOfConnectionsPerHost(qsizetype amount);
    Q_NETWORK_EXPORT qsizetype numberOfConnectionsPerHost() const;

    void swap(QHttp1Configuration &other) noexcept
    { std::swap(u, other.u); }

private:
    struct ShortData {
        std::uint8_t numConnectionsPerHost;
        char reserved[sizeof(void*) - sizeof(numConnectionsPerHost)];
    };
    union U {
        U(ShortData _data) : data(_data) {}
        QHttp1ConfigurationPrivate *d;
        ShortData data;
    } u;

    Q_NETWORK_EXPORT bool equals(const QHttp1Configuration &other) const noexcept;
    Q_NETWORK_EXPORT size_t hash(size_t seed) const noexcept;

    friend bool operator==(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept
    { return lhs.equals(rhs); }
    friend bool operator!=(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept
    { return !lhs.equals(rhs); }

    friend size_t qHash(const QHttp1Configuration &key, size_t seed = 0) noexcept { return key.hash(seed); }
};

Q_DECLARE_SHARED(QHttp1Configuration)

QT_END_NAMESPACE

#endif // QHTTP1CONFIGURATION_H
