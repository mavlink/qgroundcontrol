// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGCONNECTION_P_H
#define QQMLDEBUGCONNECTION_P_H

#include <QtCore/qobject.h>
#include <QtNetwork/qabstractsocket.h>
#include <QtCore/private/qglobal_p.h>

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

class QQmlDebugClient;
class QQmlDebugConnectionPrivate;
class QQmlDebugConnection : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlDebugConnection)
public:
    QQmlDebugConnection(QObject *parent = nullptr);
    ~QQmlDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);
    void startLocalServer(const QString &fileName);

    int currentDataStreamVersion() const;
    void setMaximumDataStreamVersion(int maximumVersion);

    bool isConnected() const;
    bool isConnecting() const;

    void close();
    bool waitForConnected(int msecs = 30000);

    QQmlDebugClient *client(const QString &name) const;
    bool addClient(const QString &name, QQmlDebugClient *client);
    bool removeClient(const QString &name);

    float serviceVersion(const QString &serviceName) const;
    bool sendMessage(const QString &name, const QByteArray &message);

Q_SIGNALS:
    void connected();
    void disconnected();
    void socketError(QAbstractSocket::SocketError socketError);
    void socketStateChanged(QAbstractSocket::SocketState socketState);

private:
    void newConnection();
    void socketConnected();
    void socketDisconnected();
    void protocolReadyRead();
    void handshakeTimeout();
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCONNECTION_P_H
