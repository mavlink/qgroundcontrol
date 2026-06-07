// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNATIVESOCKETENGINE_P_P_H
#define QNATIVESOCKETENGINE_P_P_H

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

#include "private/qabstractsocketengine_p.h"
#include "private/qnativesocketengine_p.h"

#ifndef Q_OS_WIN
#  include <netinet/in.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <mswsock.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN

// The following definitions are copied from the MinGW header mswsock.h which
// was placed in the public domain. The WSASendMsg and WSARecvMsg functions
// were introduced with Windows Vista, so some Win32 headers are lacking them.
// There are no known versions of Windows CE or Embedded that contain them.
#  ifndef WSAID_WSARECVMSG
typedef INT (WINAPI *LPFN_WSARECVMSG)(SOCKET s, LPWSAMSG lpMsg,
                                      LPDWORD lpdwNumberOfBytesRecvd,
                                      LPWSAOVERLAPPED lpOverlapped,
                                      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
#    define WSAID_WSARECVMSG {0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22}}
#  endif // !WSAID_WSARECVMSG
#  ifndef WSAID_WSASENDMSG
typedef struct {
  LPWSAMSG lpMsg;
  DWORD dwFlags;
  LPDWORD lpNumberOfBytesSent;
  LPWSAOVERLAPPED lpOverlapped;
  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;
} WSASENDMSG, *LPWSASENDMSG;

typedef INT (WSAAPI *LPFN_WSASENDMSG)(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags,
                                      LPDWORD lpNumberOfBytesSent,
                                      LPWSAOVERLAPPED lpOverlapped,
                                      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

#    define WSAID_WSASENDMSG {0xa441e712,0x754f,0x43ca,{0x84,0xa7,0x0d,0xee,0x44,0xcf,0x60,0x6d}}
#  endif // !WSAID_WSASENDMSG
#endif // Q_OS_WIN

union qt_sockaddr {
    sockaddr a;
    sockaddr_in a4;
    sockaddr_in6 a6;
};

class QSocketNotifier;

class QNativeSocketEnginePrivate : public QAbstractSocketEnginePrivate
{
    Q_DECLARE_PUBLIC(QNativeSocketEngine)
public:
    QNativeSocketEnginePrivate();
    ~QNativeSocketEnginePrivate();

    qintptr socketDescriptor;

    QSocketNotifier *readNotifier, *writeNotifier, *exceptNotifier;

#if defined(Q_OS_WIN)
    LPFN_WSASENDMSG sendmsg;
    LPFN_WSARECVMSG recvmsg;
#  endif
    enum ErrorString {
        NonBlockingInitFailedErrorString,
        BroadcastingInitFailedErrorString,
        NoIpV6ErrorString,
        RemoteHostClosedErrorString,
        TimeOutErrorString,
        ResourceErrorString,
        OperationUnsupportedErrorString,
        ProtocolUnsupportedErrorString,
        InvalidSocketErrorString,
        HostUnreachableErrorString,
        NetworkUnreachableErrorString,
        AccessErrorString,
        ConnectionTimeOutErrorString,
        ConnectionRefusedErrorString,
        AddressInuseErrorString,
        AddressNotAvailableErrorString,
        AddressProtectedErrorString,
        DatagramTooLargeErrorString,
        SendDatagramErrorString,
        ReceiveDatagramErrorString,
        WriteErrorString,
        ReadErrorString,
        PortInuseErrorString,
        NotSocketErrorString,
        InvalidProxyTypeString,
        TemporaryErrorString,
        NetworkDroppedConnectionErrorString,
        ConnectionResetErrorString,

        UnknownSocketErrorString = -1
    };

    void setError(QAbstractSocket::SocketError error, ErrorString errorString) const;
    QHostAddress adjustAddressProtocol(const QHostAddress &address) const;

    // native functions
    int option(QNativeSocketEngine::SocketOption option) const;
    bool setOption(QNativeSocketEngine::SocketOption option, int value);

    bool createNewSocket(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol &protocol);

    bool nativeConnect(const QHostAddress &address, quint16 port);
    bool nativeBind(const QHostAddress &address, quint16 port);
    bool nativeListen(int backlog);
    qintptr nativeAccept();
#ifndef QT_NO_NETWORKINTERFACE
    bool nativeJoinMulticastGroup(const QHostAddress &groupAddress,
                                  const QNetworkInterface &iface);
    bool nativeLeaveMulticastGroup(const QHostAddress &groupAddress,
                                   const QNetworkInterface &iface);
    QNetworkInterface nativeMulticastInterface() const;
    bool nativeSetMulticastInterface(const QNetworkInterface &iface);
#endif
    qint64 nativeBytesAvailable() const;

    bool nativeHasPendingDatagrams() const;
    qint64 nativePendingDatagramSize() const;
    qint64 nativeReceiveDatagram(char *data, qint64 maxLength, QIpPacketHeader *header,
                                 QAbstractSocketEngine::PacketHeaderOptions options);
    qint64 nativeSendDatagram(const char *data, qint64 length, const QIpPacketHeader &header);
    qint64 nativeRead(char *data, qint64 maxLength);
    qint64 nativeWrite(const char *data, qint64 length);
    int nativeSelect(QDeadlineTimer deadline, bool selectForRead) const;
    int nativeSelect(QDeadlineTimer deadline, bool checkRead, bool checkWrite,
                     bool *selectForRead, bool *selectForWrite) const;

    void nativeClose();

    bool checkProxy(const QHostAddress &address);
    bool fetchConnectionParameters();

    /*! \internal
        Sets \a address and \a port in the \a aa sockaddr structure and the size in \a sockAddrSize.
        The address \a is converted to IPv6 if the current socket protocol is also IPv6.
     */
    void setPortAndAddress(quint16 port, const QHostAddress &address, qt_sockaddr *aa, QT_SOCKLEN_T *sockAddrSize)
    {
        switch (socketProtocol) {
        case QHostAddress::IPv6Protocol:
        case QHostAddress::AnyIPProtocol:
            // force to IPv6
            setSockaddr(&aa->a6, address, port);
            *sockAddrSize = sizeof(sockaddr_in6);
            return;

        case QHostAddress::IPv4Protocol:
            // force to IPv4
            setSockaddr(&aa->a4, address, port);
            *sockAddrSize = sizeof(sockaddr_in);
            return;

        case QHostAddress::UnknownNetworkLayerProtocol:
            // don't force
            break;
        }
        *sockAddrSize = setSockaddr(&aa->a, address, port);
    }

};

QT_END_NAMESPACE

#endif // QNATIVESOCKETENGINE_P_P_H
