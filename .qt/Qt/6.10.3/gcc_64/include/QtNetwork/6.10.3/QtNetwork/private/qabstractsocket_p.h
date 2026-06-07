// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTSOCKET_P_H
#define QABSTRACTSOCKET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QAbstractSocket class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtNetwork/qabstractsocket.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qlist.h"
#include "QtCore/qtimer.h"
#include "private/qiodevice_p.h"
#include "private/qabstractsocketengine_p.h"
#include "qnetworkproxy.h"

QT_BEGIN_NAMESPACE

class QHostInfo;
class QNetworkInterface;

class Q_NETWORK_EXPORT QAbstractSocketPrivate : public QIODevicePrivate,
                                                public QAbstractSocketEngineReceiver
{
    Q_DECLARE_PUBLIC(QAbstractSocket)
public:
    QAbstractSocketPrivate(decltype(QObjectPrivateVersion) version = QObjectPrivateVersion);
    virtual ~QAbstractSocketPrivate();

    // from QAbstractSocketEngineReceiver
    inline void readNotification() override { canReadNotification(); }
    inline void writeNotification() override { canWriteNotification(); }
    inline void exceptionNotification() override {}
    inline void closeNotification() override { canCloseNotification(); }
    void connectionNotification() override;
#ifndef QT_NO_NETWORKPROXY
    inline void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator) override {
        Q_Q(QAbstractSocket);
        emit q->proxyAuthenticationRequired(proxy, authenticator);
    }
#endif

    virtual bool bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode,
                      const QNetworkInterface *iface = nullptr);

    virtual bool canReadNotification();
    bool canWriteNotification();
    void canCloseNotification();

    // slots
    void _q_connectToNextAddress();
    void _q_startConnecting(const QHostInfo &hostInfo);
    void _q_testConnection();
    void _q_abortConnectionAttempt();

    bool emittedReadyRead = false;
    bool emittedBytesWritten = false;

    bool abortCalled = false;
    bool pendingClose = false;

    QAbstractSocket::PauseModes pauseMode = QAbstractSocket::PauseNever;

    QString hostName;
    quint16 port = 0;
    QHostAddress host;
    QList<QHostAddress> addresses;

    quint16 localPort = 0;
    quint16 peerPort = 0;
    QHostAddress localAddress;
    QHostAddress peerAddress;
    QString peerName;

    QAbstractSocketEngine *socketEngine = nullptr;
    qintptr cachedSocketDescriptor = -1;

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    QNetworkProxy proxyInUse;
    QString protocolTag;
    void resolveProxy(const QString &hostName, quint16 port);
#else
    inline void resolveProxy(const QString &, quint16) { }
#endif
    inline void resolveProxy(quint16 port) { resolveProxy(QString(), port); }

    void resetSocketLayer();
    virtual bool flush();

    bool initSocketLayer(QAbstractSocket::NetworkLayerProtocol protocol);
    virtual void configureCreatedSocket();
    void startConnectingByName(const QString &host);
    void fetchConnectionParameters();
    bool readFromSocket();
    virtual bool writeToSocket();
    void emitReadyRead(int channel = 0);
    void emitBytesWritten(qint64 bytes, int channel = 0);

    void setError(QAbstractSocket::SocketError errorCode, const QString &errorString);
    void setErrorAndEmit(QAbstractSocket::SocketError errorCode, const QString &errorString);

    qint64 readBufferMaxSize = 0;
    bool isBuffered = false;
    bool hasPendingData = false;
    bool hasPendingDatagram = false;

    quint32 bytesWrittenEmissionCount = 0;

    QTimer *connectTimer = nullptr;

    int hostLookupId = -1;

    QAbstractSocket::SocketType socketType = QAbstractSocket::UnknownSocketType;
    QAbstractSocket::SocketState state = QAbstractSocket::UnconnectedState;

    // Must be kept in sync with QIODevicePrivate::errorString.
    QAbstractSocket::SocketError socketError = QAbstractSocket::UnknownSocketError;

    QAbstractSocket::NetworkLayerProtocol preferredNetworkLayerProtocol =
            QAbstractSocket::UnknownNetworkLayerProtocol;

    bool prePauseReadSocketNotifierState = false;
    bool prePauseWriteSocketNotifierState = false;
    bool prePauseExceptionSocketNotifierState = false;
    static void pauseSocketNotifiers(QAbstractSocket*);
    static void resumeSocketNotifiers(QAbstractSocket*);
    static QAbstractSocketEngine* getSocketEngine(QAbstractSocket*);
};

QT_END_NAMESPACE

#endif // QABSTRACTSOCKET_P_H
