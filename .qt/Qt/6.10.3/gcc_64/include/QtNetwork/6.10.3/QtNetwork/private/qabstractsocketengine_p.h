// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTSOCKETENGINE_P_H
#define QABSTRACTSOCKETENGINE_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtNetwork/qhostaddress.h"
#include "QtNetwork/qabstractsocket.h"
#include <QtCore/qdeadlinetimer.h>
#include "private/qnetworkdatagram_p.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QAbstractSocketEnginePrivate;
#ifndef QT_NO_NETWORKINTERFACE
class QNetworkInterface;
#endif
class QNetworkProxy;

class QAbstractSocketEngineReceiver {
public:
    virtual ~QAbstractSocketEngineReceiver(){}
    virtual void readNotification()= 0;
    virtual void writeNotification()= 0;
    virtual void closeNotification()= 0;
    virtual void exceptionNotification()= 0;
    virtual void connectionNotification()= 0;
#ifndef QT_NO_NETWORKPROXY
    virtual void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)= 0;
#endif
};

static constexpr std::chrono::seconds DefaultTimeout{30};

class Q_AUTOTEST_EXPORT QAbstractSocketEngine : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE(<QtNetwork/qauthenticator.h>)
public:

    static QAbstractSocketEngine *createSocketEngine(QAbstractSocket::SocketType socketType, const QNetworkProxy &, QObject *parent);
    static QAbstractSocketEngine *createSocketEngine(qintptr socketDescriptor, QObject *parent);

    QAbstractSocketEngine(QObject *parent = nullptr);

    enum SocketOption {
        NonBlockingSocketOption,
        BroadcastSocketOption,
        ReceiveBufferSocketOption,
        SendBufferSocketOption,
        AddressReusable,
        BindExclusively,
        ReceiveOutOfBandData,
        LowDelayOption,
        KeepAliveOption,
        MulticastTtlOption,
        MulticastLoopbackOption,
        TypeOfServiceOption,
        ReceivePacketInformation,
        ReceiveHopLimit,
        MaxStreamsSocketOption,
        PathMtuInformation,
        BindInterfaceIndex,
    };

    enum PacketHeaderOption {
        WantNone = 0,
        WantDatagramSender = 0x01,
        WantDatagramDestination = 0x02,
        WantDatagramHopLimit = 0x04,
        WantStreamNumber = 0x08,
        WantEndOfRecord = 0x10,

        WantAll = 0xff
    };
    Q_DECLARE_FLAGS(PacketHeaderOptions, PacketHeaderOption)

    virtual bool initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::IPv4Protocol) = 0;

    virtual bool initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState = QAbstractSocket::ConnectedState) = 0;

    virtual qintptr socketDescriptor() const = 0;

    virtual bool isValid() const = 0;

    virtual bool connectToHost(const QHostAddress &address, quint16 port) = 0;
    virtual bool connectToHostByName(const QString &name, quint16 port) = 0;
    virtual bool bind(const QHostAddress &address, quint16 port) = 0;
    virtual bool listen(int backlog) = 0;
    virtual qintptr accept() = 0;
    virtual void close() = 0;

    virtual qint64 bytesAvailable() const = 0;

    virtual qint64 read(char *data, qint64 maxlen) = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;

#ifndef QT_NO_UDPSOCKET
#ifndef QT_NO_NETWORKINTERFACE
    virtual bool joinMulticastGroup(const QHostAddress &groupAddress,
                                    const QNetworkInterface &iface) = 0;
    virtual bool leaveMulticastGroup(const QHostAddress &groupAddress,
                                     const QNetworkInterface &iface) = 0;
    virtual QNetworkInterface multicastInterface() const = 0;
    virtual bool setMulticastInterface(const QNetworkInterface &iface) = 0;
#endif // QT_NO_NETWORKINTERFACE

    virtual bool hasPendingDatagrams() const = 0;
    virtual qint64 pendingDatagramSize() const = 0;
#endif // QT_NO_UDPSOCKET

    virtual qint64 readDatagram(char *data, qint64 maxlen, QIpPacketHeader *header = nullptr,
                                PacketHeaderOptions = WantNone) = 0;
    virtual qint64 writeDatagram(const char *data, qint64 len, const QIpPacketHeader &header) = 0;
    virtual qint64 bytesToWrite() const = 0;

    virtual int option(SocketOption option) const = 0;
    virtual bool setOption(SocketOption option, int value) = 0;

    virtual bool waitForRead(QDeadlineTimer deadline = QDeadlineTimer{DefaultTimeout},
                             bool *timedOut = nullptr) = 0;
    virtual bool waitForWrite(QDeadlineTimer deadline = QDeadlineTimer{DefaultTimeout},
                              bool *timedOut = nullptr) = 0;
    virtual bool waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                            bool checkRead, bool checkWrite,
                            QDeadlineTimer deadline = QDeadlineTimer{DefaultTimeout},
                            bool *timedOut = nullptr) = 0;

    QAbstractSocket::SocketError error() const;
    QString errorString() const;
    QAbstractSocket::SocketState state() const;
    QAbstractSocket::SocketType socketType() const;
    QAbstractSocket::NetworkLayerProtocol protocol() const;

    QHostAddress localAddress() const;
    quint16 localPort() const;
    QHostAddress peerAddress() const;
    quint16 peerPort() const;
    int inboundStreamCount() const;
    int outboundStreamCount() const;

    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;
    virtual bool isExceptionNotificationEnabled() const = 0;
    virtual void setExceptionNotificationEnabled(bool enable) = 0;

public Q_SLOTS:
    void readNotification();
    void writeNotification();
    void closeNotification();
    void exceptionNotification();
    void connectionNotification();
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#endif

public:
    void setReceiver(QAbstractSocketEngineReceiver *receiver);
protected:
    QAbstractSocketEngine(QAbstractSocketEnginePrivate &dd, QObject* parent = nullptr);

    void setError(QAbstractSocket::SocketError error, const QString &errorString) const;
    void setState(QAbstractSocket::SocketState state);
    void setSocketType(QAbstractSocket::SocketType socketType);
    void setProtocol(QAbstractSocket::NetworkLayerProtocol protocol);
    void setLocalAddress(const QHostAddress &address);
    void setLocalPort(quint16 port);
    void setPeerAddress(const QHostAddress &address);
    void setPeerPort(quint16 port);

private:
    Q_DECLARE_PRIVATE(QAbstractSocketEngine)
    Q_DISABLE_COPY_MOVE(QAbstractSocketEngine)
};

class QAbstractSocketEnginePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractSocketEngine)
public:
    QAbstractSocketEnginePrivate();

    mutable QAbstractSocket::SocketError socketError;
    mutable bool hasSetSocketError;
    mutable QString socketErrorString;
    QAbstractSocket::SocketState socketState;
    QAbstractSocket::SocketType socketType;
    QAbstractSocket::NetworkLayerProtocol socketProtocol;
    QHostAddress localAddress;
    quint16 localPort;
    QHostAddress peerAddress;
    quint16 peerPort;
    int inboundStreamCount;
    int outboundStreamCount;
    QAbstractSocketEngineReceiver *receiver;
};


class Q_AUTOTEST_EXPORT QSocketEngineHandler
{
protected:
    QSocketEngineHandler();
    virtual ~QSocketEngineHandler();
    virtual QAbstractSocketEngine *createSocketEngine(QAbstractSocket::SocketType socketType,
                                                      const QNetworkProxy &, QObject *parent) = 0;
    virtual QAbstractSocketEngine *createSocketEngine(qintptr socketDescriptor, QObject *parent) = 0;

private:
    friend class QAbstractSocketEngine;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractSocketEngine::PacketHeaderOptions)

QT_END_NAMESPACE

#endif // QABSTRACTSOCKETENGINE_P_H
