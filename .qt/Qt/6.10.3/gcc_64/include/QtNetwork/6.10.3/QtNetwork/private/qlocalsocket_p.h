// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOCALSOCKET_P_H
#define QLOCALSOCKET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLocalSocket class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qlocalsocket.h"
#include "private/qiodevice_p.h"

#include <qtimer.h>

QT_REQUIRE_CONFIG(localserver);

#if defined(QT_LOCALSOCKET_TCP)
#   include "qtcpsocket.h"
#elif defined(Q_OS_WIN)
#   include "private/qwindowspipereader_p.h"
#   include "private/qwindowspipewriter_p.h"
#   include <qwineventnotifier.h>
#else
#   include "private/qabstractsocketengine_p.h"
#   include <qtcpsocket.h>
#   include <qsocketnotifier.h>
#   include <errno.h>
#endif

struct sockaddr_un;

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_WIN) || defined(QT_LOCALSOCKET_TCP)

class QLocalUnixSocket : public QTcpSocket
{

public:
    QLocalUnixSocket() : QTcpSocket()
    {
    }

    inline void setSocketState(QAbstractSocket::SocketState state)
    {
        QTcpSocket::setSocketState(state);
    }

    inline void setErrorString(const QString &string)
    {
        QTcpSocket::setErrorString(string);
    }

    inline void setSocketError(QAbstractSocket::SocketError error)
    {
        QTcpSocket::setSocketError(error);
    }

    inline qint64 readData(char *data, qint64 maxSize) override
    {
        return QTcpSocket::readData(data, maxSize);
    }

    inline qint64 writeData(const char *data, qint64 maxSize) override
    {
        return QTcpSocket::writeData(data, maxSize);
    }
};
#endif //#if !defined(Q_OS_WIN) || defined(QT_LOCALSOCKET_TCP)

class QLocalSocketPrivate : public QIODevicePrivate
{
public:
    Q_DECLARE_PUBLIC(QLocalSocket)

    QLocalSocketPrivate();
    void init();

#if defined(QT_LOCALSOCKET_TCP)
    QLocalUnixSocket* tcpSocket;
    bool ownsTcpSocket;
    void setSocket(QLocalUnixSocket*);
    QString generateErrorString(QLocalSocket::LocalSocketError, const QString &function) const;
    void setErrorAndEmit(QLocalSocket::LocalSocketError, const QString &function);
    void _q_stateChanged(QAbstractSocket::SocketState newState);
    void _q_errorOccurred(QAbstractSocket::SocketError newError);
#elif defined(Q_OS_WIN)
    ~QLocalSocketPrivate();
    qint64 pipeWriterBytesToWrite() const;
    void _q_canRead();
    void _q_bytesWritten(qint64 bytes);
    void _q_pipeClosed();
    void _q_winError(ulong windowsError, const QString &function);
    void _q_writeFailed();
    HANDLE handle;
    QWindowsPipeWriter *pipeWriter;
    QWindowsPipeReader *pipeReader;
    QLocalSocket::LocalSocketError error;
#else
    QLocalUnixSocket unixSocket;
    QString generateErrorString(QLocalSocket::LocalSocketError, const QString &function) const;
    void setErrorAndEmit(QLocalSocket::LocalSocketError, const QString &function);
    void _q_stateChanged(QAbstractSocket::SocketState newState);
    void _q_errorOccurred(QAbstractSocket::SocketError newError);
    void _q_connectToSocket();
    void _q_abortConnectionAttempt();
    void cancelDelayedConnect();
    void describeSocket(qintptr socketDescriptor);
    static bool parseSockaddr(const sockaddr_un &addr, uint len,
                              QString &fullServerName, QString &serverName, bool &abstractNamespace);
    QSocketNotifier *delayConnect;
    QTimer *connectTimer;
    QString connectingName;
    int connectingSocket;
    QIODevice::OpenMode connectingOpenMode;
#endif
    QLocalSocket::LocalSocketState state;
    QString serverName;
    QString fullServerName;
#if defined(Q_OS_WIN) && !defined(QT_LOCALSOCKET_TCP)
    bool emittedReadyRead;
    bool emittedBytesWritten;
#endif

    Q_OBJECT_BINDABLE_PROPERTY(QLocalSocketPrivate, QLocalSocket::SocketOptions, socketOptions)
};

QT_END_NAMESPACE

#endif // QLOCALSOCKET_P_H

