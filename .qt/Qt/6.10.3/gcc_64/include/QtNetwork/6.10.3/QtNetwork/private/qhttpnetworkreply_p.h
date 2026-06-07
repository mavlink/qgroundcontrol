// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPNETWORKREPLY_H
#define QHTTPNETWORKREPLY_H

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

#include <QtNetwork/private/qbytedatabuffer_p.h>

#include <qplatformdefs.h>

#include <QtNetwork/qtcpsocket.h>
// it's safe to include these even if SSL support is not enabled
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qsslerror.h>

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <qbuffer.h>

#include <private/qobject_p.h>
#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qauthenticator_p.h>
#include <private/qringbuffer_p.h>

#ifndef QT_NO_NETWORKPROXY
Q_MOC_INCLUDE(<QtNetwork/QNetworkProxy>)
#endif
Q_MOC_INCLUDE(<QtNetwork/QAuthenticator>)

#include <private/qdecompresshelper_p.h>
#include <QtNetwork/qhttpheaders.h>

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttpNetworkConnection;
class QHttpNetworkConnectionChannel;
class QHttpNetworkRequest;
class QHttpNetworkConnectionPrivate;
class QHttpNetworkReplyPrivate;
class Q_NETWORK_EXPORT QHttpNetworkReply : public QObject, public QHttpNetworkHeader
{
    Q_OBJECT
public:

    explicit QHttpNetworkReply(const QUrl &url = QUrl(), QObject *parent = nullptr);
    ~QHttpNetworkReply() override;

    QUrl url() const override;
    void setUrl(const QUrl &url) override;

    int majorVersion() const override;
    int minorVersion() const override;
    void setMajorVersion(int version);
    void setMinorVersion(int version);

    qint64 contentLength() const override;
    void setContentLength(qint64 length) override;

    QHttpHeaders header() const override;
    QByteArray headerField(QByteArrayView name, const QByteArray &defaultValue = QByteArray()) const override;
    void setHeaderField(const QByteArray &name, const QByteArray &data) override;
    void appendHeaderField(const QByteArray &name, const QByteArray &data);
    void parseHeader(QByteArrayView header); // used for testing

    QHttpNetworkRequest request() const;
    void setRequest(const QHttpNetworkRequest &request);

    int statusCode() const;
    void setStatusCode(int code);

    QString errorString() const;
    void setErrorString(const QString &error);

    QNetworkReply::NetworkError errorCode() const;

    QString reasonPhrase() const;
    void setReasonPhrase(const QString &reason);

    qint64 bytesAvailable() const;
    qint64 bytesAvailableNextBlock() const;
    bool readAnyAvailable() const;
    QByteArray readAny();
    QByteArray readAll();
    QByteArray read(qint64 amount);
    qint64 sizeNextBlock();
    void setDownstreamLimited(bool t);
    void setReadBufferSize(qint64 size);

    bool supportsUserProvidedDownloadBuffer();
    void setUserProvidedDownloadBuffer(char*);
    char* userProvidedDownloadBuffer();

    void abort();

    bool isAborted() const;
    bool isFinished() const;

    bool isPipeliningUsed() const;
    bool isHttp2Used() const;
    void setHttp2WasUsed(bool h2Used);
    qint64 removedContentLength() const;

    bool isRedirecting() const;

    QHttpNetworkConnection* connection();

    QUrl redirectUrl() const;
    void setRedirectUrl(const QUrl &url);

    static bool isHttpRedirect(int statusCode);

    bool isCompressed() const;

#ifndef QT_NO_SSL
    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &config);
    void ignoreSslErrors();
    void ignoreSslErrors(const QList<QSslError> &errors);

Q_SIGNALS:
    void encrypted();
    void sslErrors(const QList<QSslError> &errors);
    void preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator);
#endif

Q_SIGNALS:
    void socketStartedConnecting();
    void requestSent();
    void readyRead();
    void finished();
    void finishedWithError(QNetworkReply::NetworkError errorCode, const QString &detail = QString());
    void headerChanged();
    void dataReadProgress(qint64 done, qint64 total);
    void dataSendProgress(qint64 done, qint64 total);
    void cacheCredentials(const QHttpNetworkRequest &request, QAuthenticator *authenticator);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#endif
    void authenticationRequired(const QHttpNetworkRequest &request, QAuthenticator *authenticator);
    void redirected(const QUrl &url, int httpStatus, int maxRedirectsRemaining);
private:
    Q_DECLARE_PRIVATE(QHttpNetworkReply)
    friend class QHttpSocketEngine;
    friend class QHttpNetworkConnection;
    friend class QHttpNetworkConnectionPrivate;
    friend class QHttpNetworkConnectionChannel;
    friend class QHttp2ProtocolHandler;
    friend class QHttpProtocolHandler;
    friend class QSpdyProtocolHandler;
};


class Q_AUTOTEST_EXPORT QHttpNetworkReplyPrivate : public QObjectPrivate, public QHttpNetworkHeaderPrivate
{
public:
    QHttpNetworkReplyPrivate(const QUrl &newUrl = QUrl());
    ~QHttpNetworkReplyPrivate();
    qint64 readStatus(QIODevice *socket);
    bool parseStatus(QByteArrayView status);
    qint64 readHeader(QIODevice *socket);
    void parseHeader(QByteArrayView header);
    void appendHeaderField(const QByteArray &name, const QByteArray &data);
    qint64 readBody(QIODevice *socket, QByteDataBuffer *out);
    qint64 readBodyVeryFast(QIODevice *socket, char *b);
    qint64 readBodyFast(QIODevice *socket, QByteDataBuffer *rb);
    void clear();
    void clearHttpLayerInformation();

    qint64 readReplyBodyRaw(QIODevice *in, QByteDataBuffer *out, qint64 size);
    qint64 readReplyBodyChunked(QIODevice *in, QByteDataBuffer *out);
    qint64 getChunkSize(QIODevice *in, qint64 *chunkSize);

    bool isRedirecting() const;
    bool shouldEmitSignals();
    bool expectContent();
    void eraseData();

    qint64 bytesAvailable() const;
    bool isChunked();
    bool isConnectionCloseEnabled();

    bool isCompressed() const;
    void removeAutoDecompressHeader();

    enum ReplyState {
        NothingDoneState,
        ReadingStatusState,
        ReadingHeaderState,
        ReadingDataState,
        AllDoneState,
        Aborted
    } state;

    QHttpNetworkRequest request;
    bool ssl;
    QString errorString;
    qint64 bodyLength;
    qint64 contentRead;
    qint64 totalProgress;
    QByteArray fragment; // used for header, status, chunk header etc, not for reply data
    bool chunkedTransferEncoding;
    bool connectionCloseEnabled;
    bool forceConnectionCloseEnabled;
    bool lastChunkRead;
    qint64 currentChunkSize;
    qint64 currentChunkRead;
    qint64 readBufferMaxSize;
    qint64 totallyUploadedData; //  HTTP/2
    qint64 removedContentLength;
    QPointer<QHttpNetworkConnection> connection;
    QPointer<QHttpNetworkConnectionChannel> connectionChannel;
    QNetworkReply::NetworkError httpErrorCode = QNetworkReply::NoError;

    bool autoDecompress;

    QByteDataBuffer responseData; // uncompressed body
    bool requestIsPrepared;

    bool pipeliningUsed;
    bool h2Used;
    bool downstreamLimited;

    char* userProvidedDownloadBuffer;
    QUrl redirectUrl;
};




QT_END_NAMESPACE

#endif // QHTTPNETWORKREPLY_H
