// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTCPSERVER_P_H
#define QTCPSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtNetwork/qtcpserver.h"
#include "private/qobject_p.h"
#include "private/qabstractsocketengine_p.h"
#include "QtNetwork/qabstractsocket.h"
#include "qnetworkproxy.h"
#include "QtCore/qlist.h"
#include "qhostaddress.h"

QT_BEGIN_NAMESPACE

class Q_NETWORK_EXPORT QTcpServerPrivate : public QObjectPrivate,
                                           public QAbstractSocketEngineReceiver
{
    Q_DECLARE_PUBLIC(QTcpServer)
public:
    QTcpServerPrivate();
    ~QTcpServerPrivate();

    QList<QTcpSocket *> pendingConnections;

    quint16 port;
    QHostAddress address;

    QAbstractSocket::SocketType socketType;
    QAbstractSocket::SocketState state;
    QAbstractSocketEngine *socketEngine;

    QAbstractSocket::SocketError serverSocketError;
    QString serverSocketErrorString;

    int listenBacklog = 50;
    int maxConnections;

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    QNetworkProxy resolveProxy(const QHostAddress &address, quint16 port);
#endif

    virtual void configureCreatedSocket();
    virtual int totalPendingConnections() const;

    // from QAbstractSocketEngineReceiver
    void readNotification() override;
    void closeNotification() override { readNotification(); }
    void writeNotification() override {}
    void exceptionNotification() override {}
    void connectionNotification() override {}
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *) override {}
#endif

};

QT_END_NAMESPACE

#endif // QTCPSERVER_P_H
