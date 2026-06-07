// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOCALSERVER_P_H
#define QLOCALSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLocalServer class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qlocalserver.h"
#include "private/qobject_p.h"
#include <qqueue.h>

QT_REQUIRE_CONFIG(localserver);

#if defined(QT_LOCALSOCKET_TCP)
#   include <qtcpserver.h>
#   include <QtCore/qmap.h>
#elif defined(Q_OS_WIN)
#   include <qt_windows.h>
#   include <qwineventnotifier.h>
#else
#   include <private/qabstractsocketengine_p.h>
#   include <qsocketnotifier.h>
#endif

QT_BEGIN_NAMESPACE

class QLocalServerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QLocalServer)

public:
    QLocalServerPrivate() :
#if !defined(QT_LOCALSOCKET_TCP) && !defined(Q_OS_WIN)
            listenSocket(-1), socketNotifier(nullptr),
#endif
            maxPendingConnections(30), error(QAbstractSocket::UnknownSocketError),
            socketOptions(QLocalServer::NoOptions)
    {
    }

    void init();
    bool listen(const QString &name);
    bool listen(qintptr socketDescriptor);
    static bool removeServer(const QString &name);
    void closeServer();
    void waitForNewConnection(int msec, bool *timedOut);
    void _q_onNewConnection();

#if defined(QT_LOCALSOCKET_TCP)

    QTcpServer tcpServer;
    QMap<quintptr, QTcpSocket*> socketMap;
#elif defined(Q_OS_WIN)
    struct Listener {
        Listener() = default;
        HANDLE handle = nullptr;
        OVERLAPPED overlapped;
        bool connected = false;
    private:
        Q_DISABLE_COPY(Listener)
    };

    void setError(const QString &function);
    bool addListener();

    std::vector<std::unique_ptr<Listener>> listeners;
    HANDLE eventHandle = nullptr;
    QWinEventNotifier *connectionEventNotifier;
#else
    void setError(const QString &function);

    int listenSocket;
    QSocketNotifier *socketNotifier;
#endif

    QString serverName;
    QString fullServerName;
    int maxPendingConnections;
    QQueue<QLocalSocket*> pendingConnections;
    QString errorString;
    QAbstractSocket::SocketError error;
    int listenBacklog = 50;

    Q_OBJECT_BINDABLE_PROPERTY(QLocalServerPrivate, QLocalServer::SocketOptions, socketOptions)
};

QT_END_NAMESPACE

#endif // QLOCALSERVER_P_H

