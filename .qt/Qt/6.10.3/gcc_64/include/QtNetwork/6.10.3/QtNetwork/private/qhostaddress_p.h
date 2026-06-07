// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHOSTADDRESSPRIVATE_H
#define QHOSTADDRESSPRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QHostAddress and QNetworkInterface classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qhostaddress.h"
#include "qabstractsocket.h"

QT_BEGIN_NAMESPACE

enum AddressClassification {
    LoopbackAddress = 1,
    LocalNetAddress,                // RFC 1122
    LinkLocalAddress,               // RFC 4291 (v6), RFC 3927 (v4)
    MulticastAddress,               // RFC 4291 (v6), RFC 3171 (v4)
    BroadcastAddress,               // RFC 919, 922

    GlobalAddress = 16,
    TestNetworkAddress,             // RFC 3849 (v6), RFC 5737 (v4),
    PrivateNetworkAddress,          // RFC 1918
    UniqueLocalAddress,             // RFC 4193
    SiteLocalAddress,               // RFC 4291 (deprecated by RFC 3879, should be treated as global)

    UnknownAddress = 0              // unclassified or reserved
};

class QNetmask
{
    // stores 0-32 for IPv4, 0-128 for IPv6, or 255 for invalid
    quint8 length;
public:
    constexpr QNetmask() : length(255) {}

    bool setAddress(const QHostAddress &address);
    QHostAddress address(QAbstractSocket::NetworkLayerProtocol protocol) const;

    int prefixLength() const { return length == 255 ? -1 : length; }
    void setPrefixLength(QAbstractSocket::NetworkLayerProtocol proto, int len)
    {
        int maxlen = -1;
        if (proto == QAbstractSocket::IPv4Protocol)
            maxlen = 32;
        else if (proto == QAbstractSocket::IPv6Protocol)
            maxlen = 128;
        if (len > maxlen || len < 0)
            length = 255U;
        else
            length = unsigned(len);
    }

    friend bool operator==(QNetmask n1, QNetmask n2)
    { return n1.length == n2.length; }
};

class QHostAddressPrivate : public QSharedData
{
public:
    void setAddress(quint32 a_ = 0);
    void setAddress(const quint8 *a_);
    void setAddress(const Q_IPV6ADDR &a_);

    bool parse(const QString &ipString);
    void clear()
    {
        a6 = {};
        a = 0;
        protocol = QHostAddress::UnknownNetworkLayerProtocol;
        scopeId.clear();
    }

    QString scopeId;

    union {
        Q_IPV6ADDR a6 = {};     // IPv6 address
        struct { quint64 c[2]; } a6_64;
        struct { quint32 c[4]; } a6_32;
    };
    quint32 a = 0;      // IPv4 address
    qint8 protocol = QHostAddress::UnknownNetworkLayerProtocol;

    AddressClassification classify() const;
    static AddressClassification classify(const QHostAddress &address)
    { return address.d->classify(); }

    friend class QHostAddress;
};

QT_END_NAMESPACE

#endif
