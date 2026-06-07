// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPNETWORKCONNECTIONCHANNEL_H
#define QHTTPNETWORKCONNECTIONCHANNEL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qabstractsocket.h>

#include <private/qobject_p.h>
#include <qauthenticator.h>
#include <qnetworkproxy.h>
#include <qbuffer.h>

#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qhttpnetworkreply_p.h>

#include <private/qhttpnetworkconnection_p.h>
#include <private/qabstractprotocolhandler_p.h>

#ifndef QT_NO_SSL
#    include <QtNetwork/qsslsocket.h>
#    include <QtNetwork/qsslerror.h>
#    include <QtNetwork/qsslconfiguration.h>
#else
#   include <QtNetwork/qtcpsocket.h>
#endif
#if QT_CONFIG(localserver)
#   include <QtNetwork/qlocalsocket.h>
#endif


#include <QtCore/qpointer.h>

#include <memory>
#include <optional>
#include <utility>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttpNetworkRequest;
class QHttpNetworkReply;
class QByteArray;

#ifndef HttpMessagePair
typedef std::pair<QHttpNetworkRequest, QHttpNetworkReply*> HttpMessagePair;
#endif

class QHttpNetworkConnectionChannel final : public QObject {
    Q_OBJECT
public:
    // TODO: Refactor this to add an EncryptingState (and remove pendingEncrypt).
    // Also add an Unconnected state so IdleState does not have double meaning.
    enum ChannelState {
        IdleState = 0,          // ready to send request
        ConnectingState = 1,    // connecting to host
        WritingState = 2,       // writing the data
        WaitingState = 4,       // waiting for reply
        ReadingState = 8,       // reading the reply
        ClosingState = 16,
        BusyState = (ConnectingState|WritingState|WaitingState|ReadingState|ClosingState)
    };
    QIODevice *socket;
    bool ssl;
    bool isInitialized;
    bool waitingForPotentialAbort = false;
    bool needInvokeReceiveReply = false;
    bool needInvokeReadyRead = false;
    bool needInvokeSendRequest = false;
    ChannelState state;
    QHttpNetworkRequest request; // current request, only used for HTTP
    QHttpNetworkReply *reply; // current reply for this request, only used for HTTP
    qint64 written;
    qint64 bytesTotal;
    bool resendCurrent;
    int lastStatus; // last status received on this channel
    bool pendingEncrypt; // for https (send after encrypted)
    int reconnectAttempts; // maximum 2 reconnection attempts
    QAuthenticator authenticator;
    QAuthenticator proxyAuthenticator;
    bool authenticationCredentialsSent;
    bool proxyCredentialsSent;
    std::unique_ptr<QAbstractProtocolHandler> protocolHandler;
    QMultiMap<int, HttpMessagePair> h2RequestsToSend;
    bool switchedToHttp2 = false;
#ifndef QT_NO_SSL
    bool ignoreAllSslErrors;
    QList<QSslError> ignoreSslErrorsList;
    std::optional<QSslConfiguration> sslConfiguration;
    void ignoreSslErrors();
    void ignoreSslErrors(const QList<QSslError> &errors);
    void setSslConfiguration(const QSslConfiguration &config);
    void requeueHttp2Requests(); // when we wanted HTTP/2 but got HTTP/1.1
#endif
    // to emit the signal for all in-flight replies:
    void emitFinishedWithError(QNetworkReply::NetworkError error, const char *message);

    // HTTP pipelining -> http://en.wikipedia.org/wiki/Http_pipelining
    enum PipeliningSupport {
        PipeliningSupportUnknown, // default for a new connection
        PipeliningProbablySupported, // after having received a server response that indicates support
        PipeliningNotSupported // currently not used
    };
    PipeliningSupport pipeliningSupported;
    QList<HttpMessagePair> alreadyPipelinedRequests;
    QByteArray pipeline; // temporary buffer that gets sent to socket in pipelineFlush
    void pipelineInto(HttpMessagePair &pair);
    void pipelineFlush();
    void requeueCurrentlyPipelinedRequests();
    void detectPipeliningSupport();

    QHttpNetworkConnectionChannel();

    QAbstractSocket::NetworkLayerProtocol networkLayerPreference;

    void setConnection(QHttpNetworkConnection *c);
    QPointer<QHttpNetworkConnection> connection;

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    void setProxy(const QNetworkProxy &networkProxy);
#endif

    void init();
    void close();
    void abort();

    void sendRequest();
    void sendRequestDelayed();

    bool ensureConnection();

    void allDone(); // reply header + body have been read
    void handleStatus(); // called from allDone()

    bool resetUploadData(); // return true if resetting worked or there is no upload data

    void handleUnexpectedEOF();
    void closeAndResendCurrentRequest();
    void resendCurrentRequest();

    void checkAndResumeCommunication();

    bool isSocketBusy() const;
    bool isSocketWriting() const;
    bool isSocketWaiting() const;
    bool isSocketReading() const;

    protected slots:
    void _q_receiveReply();
    void _q_bytesWritten(qint64 bytes); // proceed sending
    void _q_readyRead(); // pending data to read
    void _q_disconnected(); // disconnected from host
    void _q_connected_abstract_socket(QAbstractSocket *socket);
#if QT_CONFIG(localserver)
    void _q_connected_local_socket(QLocalSocket *socket);
#endif
    void _q_connected(); // start sending request
    void _q_error(QAbstractSocket::SocketError); // error from socket
#ifndef QT_NO_NETWORKPROXY
    void _q_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth); // from transparent proxy
#endif

    void _q_uploadDataReadyRead();

#ifndef QT_NO_SSL
    void _q_encrypted(); // start sending request (https)
    void _q_sslErrors(const QList<QSslError> &errors); // ssl errors from the socket
    void _q_preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*); // tls-psk auth necessary
    void _q_encryptedBytesWritten(qint64 bytes); // proceed sending
#endif

    friend class QHttpProtocolHandler;
};

QT_END_NAMESPACE

#endif
