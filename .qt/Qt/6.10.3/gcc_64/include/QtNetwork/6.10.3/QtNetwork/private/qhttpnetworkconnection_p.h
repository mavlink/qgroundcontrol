// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPNETWORKCONNECTION_H
#define QHTTPNETWORKCONNECTION_H

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

#include <qhttp2configuration.h>

#include <private/qobject_p.h>
#include <qauthenticator.h>
#include <qnetworkproxy.h>
#include <qbuffer.h>
#include <qtimer.h>
#include <qsharedpointer.h>

#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qhttpnetworkreply_p.h>
#include <private/qnetconmonitor_p.h>
#include <private/http2protocol_p.h>

#include <private/qhttpnetworkconnectionchannel_p.h>

#include <utility>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttpNetworkRequest;
class QHttpNetworkReply;
class QHttpThreadDelegate;
class QByteArray;
class QHostInfo;
#ifndef QT_NO_SSL
class QSslConfiguration;
class QSslContext;
#endif // !QT_NO_SSL

class QHttpNetworkConnectionPrivate;
class Q_NETWORK_EXPORT QHttpNetworkConnection : public QObject
{
    Q_OBJECT
public:

    enum ConnectionType {
        ConnectionTypeHTTP,
        ConnectionTypeHTTP2,
        ConnectionTypeHTTP2Direct
    };

    QHttpNetworkConnection(quint16 channelCount, const QString &hostName, quint16 port = 80,
                           bool encrypt = false, bool isLocalSocket = false,
                           QObject *parent = nullptr,
                           ConnectionType connectionType = ConnectionTypeHTTP);
    ~QHttpNetworkConnection();

    //The hostname to which this is connected to.
    QString hostName() const;
    //The HTTP port in use.
    quint16 port() const;

    //add a new HTTP request through this connection
    QHttpNetworkReply* sendRequest(const QHttpNetworkRequest &request);
    void fillHttp2Queue();

#ifndef QT_NO_NETWORKPROXY
    //set the proxy for this connection
    void setCacheProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy cacheProxy() const;
    void setTransparentProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy transparentProxy() const;
#endif

    bool isSsl() const;

    QHttpNetworkConnectionChannel *channels() const;

    ConnectionType connectionType() const;
    void setConnectionType(ConnectionType type);

    QHttp2Configuration http2Parameters() const;
    void setHttp2Parameters(const QHttp2Configuration &params);

#ifndef QT_NO_SSL
    void setSslConfiguration(const QSslConfiguration &config);
    void ignoreSslErrors(int channel = -1);
    void ignoreSslErrors(const QList<QSslError> &errors, int channel = -1);
    std::shared_ptr<QSslContext> sslContext() const;
    void setSslContext(std::shared_ptr<QSslContext> context);
#endif

    void preConnectFinished();

    QString peerVerifyName() const;
    void setPeerVerifyName(const QString &peerName);

public slots:
    void onlineStateChanged(bool isOnline);

private:
    Q_DECLARE_PRIVATE(QHttpNetworkConnection)
    Q_DISABLE_COPY_MOVE(QHttpNetworkConnection)
    friend class QHttpThreadDelegate;
    friend class QHttpNetworkReply;
    friend class QHttpNetworkReplyPrivate;
    friend class QHttpNetworkConnectionChannel;
    friend class QHttp2ProtocolHandler;
    friend class QHttpProtocolHandler;

    Q_PRIVATE_SLOT(d_func(), void _q_startNextRequest())
    Q_PRIVATE_SLOT(d_func(), void _q_hostLookupFinished(QHostInfo))
    Q_PRIVATE_SLOT(d_func(), void _q_connectDelayedChannel())
};


// private classes
typedef std::pair<QHttpNetworkRequest, QHttpNetworkReply*> HttpMessagePair;


class QHttpNetworkConnectionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QHttpNetworkConnection)
    Q_DISABLE_COPY_MOVE(QHttpNetworkConnectionPrivate)
public:
    // Note: Only used from auto tests, normal usage is via QHttp1Configuration
    static constexpr int defaultHttpChannelCount = 6;
    static const int defaultPipelineLength;
    static const int defaultRePipelineLength;

    enum ConnectionState {
        RunningState = 0,
        PausedState = 1
    };

    enum NetworkLayerPreferenceState {
        Unknown,
        HostLookupPending,
        IPv4,
        IPv6,
        IPv4or6
    };

    QHttpNetworkConnectionPrivate(quint16 connectionCount, const QString &hostName, quint16 port,
                                  bool encrypt, bool isLocalSocket,
                                  QHttpNetworkConnection::ConnectionType type);
    ~QHttpNetworkConnectionPrivate();
    void init();

    void pauseConnection();
    void resumeConnection();
    ConnectionState state = RunningState;
    NetworkLayerPreferenceState networkLayerState = Unknown;

    enum { ChunkSize = 4096 };

    int indexOf(QIODevice *socket) const;

    QHttpNetworkReply *queueRequest(const QHttpNetworkRequest &request);
    void requeueRequest(const HttpMessagePair &pair); // e.g. after pipeline broke
    void fillHttp2Queue();
    bool dequeueRequest(QIODevice *socket);
    void prepareRequest(HttpMessagePair &request);
    void updateChannel(int i, const HttpMessagePair &messagePair);
    QHttpNetworkRequest predictNextRequest() const;
    QHttpNetworkReply* predictNextRequestsReply() const;

    void fillPipeline(QIODevice *socket);
    bool fillPipeline(QList<HttpMessagePair> &queue, QHttpNetworkConnectionChannel &channel);

    // read more HTTP body after the next event loop spin
    void readMoreLater(QHttpNetworkReply *reply);

    void copyCredentials(int fromChannel, QAuthenticator *auth, bool isProxy);

    void startHostInfoLookup();
    void startNetworkLayerStateLookup();
    void networkLayerDetected(QAbstractSocket::NetworkLayerProtocol protocol);

    // private slots
    void _q_startNextRequest(); // send the next request from the queue

    void _q_hostLookupFinished(const QHostInfo &info);
    void _q_connectDelayedChannel();

    void createAuthorization(QIODevice *socket, QHttpNetworkRequest &request);

    QString errorDetail(QNetworkReply::NetworkError errorCode, QIODevice *socket,
                        const QString &extraDetail = QString());

    void removeReply(QHttpNetworkReply *reply);

    QString hostName;
    quint16 port;
    bool encrypt;
    bool isLocalSocket;
    bool delayIpv4 = true;

    // Number of channels we are trying to use at the moment:
    int activeChannelCount;
    // The total number of channels we reserved:
    const int channelCount;
    QTimer delayedConnectionTimer;
    QHttpNetworkConnectionChannel * const channels; // parallel connections to the server
    bool shouldEmitChannelError(QIODevice *socket);

    qint64 uncompressedBytesAvailable(const QHttpNetworkReply &reply) const;
    qint64 uncompressedBytesAvailableNextBlock(const QHttpNetworkReply &reply) const;


    void emitReplyError(QIODevice *socket, QHttpNetworkReply *reply, QNetworkReply::NetworkError errorCode);
    bool handleAuthenticateChallenge(QIODevice *socket, QHttpNetworkReply *reply, bool isProxy, bool &resend);
    struct ParseRedirectResult {
        QUrl redirectUrl;
        QNetworkReply::NetworkError errorCode;
    };
    static ParseRedirectResult parseRedirectResponse(QHttpNetworkReply *reply);
    // Used by the HTTP1 code-path
    QUrl parseRedirectResponse(QIODevice *socket, QHttpNetworkReply *reply);

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy networkProxy;
    void emitProxyAuthenticationRequired(const QHttpNetworkConnectionChannel *chan, const QNetworkProxy &proxy, QAuthenticator* auth);
#endif

    //The request queues
    QList<HttpMessagePair> highPriorityQueue;
    QList<HttpMessagePair> lowPriorityQueue;

    int preConnectRequests = 0;

    QHttpNetworkConnection::ConnectionType connectionType;

#ifndef QT_NO_SSL
    std::shared_ptr<QSslContext> sslContext;
#endif

    QHttp2Configuration http2Parameters;

    QString peerVerifyName;
    // If network status monitoring is enabled, we activate connectionMonitor
    // as soons as one of channels managed to connect to host (and we
    // have a pair of addresses (us,peer).
    // NETMONTODO: consider activating a monitor on a change from
    // HostLookUp state to ConnectingState (means we have both
    // local/remote addresses known and can start monitoring this
    // early).
    QNetworkConnectionMonitor connectionMonitor;

    friend class QHttpNetworkConnectionChannel;
};



QT_END_NAMESPACE

#endif
