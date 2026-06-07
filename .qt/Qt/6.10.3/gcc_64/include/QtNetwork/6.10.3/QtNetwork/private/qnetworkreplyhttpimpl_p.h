// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREPLYHTTPIMPL_P_H
#define QNETWORKREPLYHTTPIMPL_P_H

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
#include "qnetworkrequest.h"
#include "qnetworkreply.h"

#include "QtCore/qpointer.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qsharedpointer.h"
#include "QtCore/qscopedpointer.h"
#include "QtCore/qtimer.h"
#include "qatomic.h"

#include <QtNetwork/QNetworkCacheMetaData>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qnetworkreply_p.h>
#include <QtNetwork/QNetworkProxy>

#ifndef QT_NO_SSL
#include <QtNetwork/QSslConfiguration>
#endif

Q_MOC_INCLUDE(<QtNetwork/QAuthenticator>)

#include <private/qdecompresshelper_p.h>

#include <memory>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QIODevice;

class QNetworkReplyHttpImplPrivate;
class QNetworkReplyHttpImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyHttpImpl(QNetworkAccessManager* const, const QNetworkRequest&, QNetworkAccessManager::Operation&, QIODevice* outgoingData);
    virtual ~QNetworkReplyHttpImpl();

    void close() override;
    void abort() override;
    void abortImpl(QNetworkReply::NetworkError error);
    qint64 bytesAvailable() const override;
    bool isSequential () const override;
    qint64 size() const override;
    qint64 readData(char*, qint64) override;
    void setReadBufferSize(qint64 size) override;
    bool canReadLine () const override;

    Q_DECLARE_PRIVATE(QNetworkReplyHttpImpl)
    Q_PRIVATE_SLOT(d_func(), void _q_startOperation())
    Q_PRIVATE_SLOT(d_func(), void _q_cacheLoadReadyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingData())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingDataFinished())
    Q_PRIVATE_SLOT(d_func(), void _q_transferTimedOut())
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
    Q_PRIVATE_SLOT(d_func(), void _q_error(QNetworkReply::NetworkError, const QString &))

    // From reply
    Q_PRIVATE_SLOT(d_func(), void replyDownloadData(QByteArray))
    Q_PRIVATE_SLOT(d_func(), void replyFinished())
    Q_PRIVATE_SLOT(d_func(), void replyDownloadProgressSlot(qint64,qint64))
    Q_PRIVATE_SLOT(d_func(), void httpAuthenticationRequired(const QHttpNetworkRequest &, QAuthenticator *))
    Q_PRIVATE_SLOT(d_func(), void httpError(QNetworkReply::NetworkError, const QString &))
#ifndef QT_NO_SSL
    Q_PRIVATE_SLOT(d_func(), void replyEncrypted())
    Q_PRIVATE_SLOT(d_func(), void replySslErrors(const QList<QSslError> &, bool *, QList<QSslError> *))
    Q_PRIVATE_SLOT(d_func(), void replySslConfigurationChanged(const QSslConfiguration&))
    Q_PRIVATE_SLOT(d_func(), void replyPreSharedKeyAuthenticationRequiredSlot(QSslPreSharedKeyAuthenticator *))
#endif
#ifndef QT_NO_NETWORKPROXY
    Q_PRIVATE_SLOT(d_func(), void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth))
#endif

    Q_PRIVATE_SLOT(d_func(), void resetUploadDataSlot(bool *r))
    Q_PRIVATE_SLOT(d_func(), void wantUploadDataSlot(qint64))
    Q_PRIVATE_SLOT(d_func(), void sentUploadDataSlot(qint64,qint64))
    Q_PRIVATE_SLOT(d_func(), void uploadByteDeviceReadyReadSlot())
    Q_PRIVATE_SLOT(d_func(), void emitReplyUploadProgress(qint64, qint64))
    Q_PRIVATE_SLOT(d_func(), void _q_cacheSaveDeviceAboutToClose())
    Q_PRIVATE_SLOT(d_func(), void _q_metaDataChanged())
    Q_PRIVATE_SLOT(d_func(), void onRedirected(const QUrl &, int, int))
    Q_PRIVATE_SLOT(d_func(), void followRedirect())

#ifndef QT_NO_SSL
protected:
    void ignoreSslErrors() override;
    void ignoreSslErrorsImplementation(const QList<QSslError> &errors) override;
    void setSslConfigurationImplementation(const QSslConfiguration &configuration) override;
    void sslConfigurationImplementation(QSslConfiguration &configuration) const override;
#endif

signals:
    // To HTTP thread:
    void startHttpRequest();
    void abortHttpRequest();
    void readBufferSizeChanged(qint64 size);
    void readBufferFreed(qint64 size);

    void startHttpRequestSynchronously();

    void haveUploadData(const qint64 pos, const QByteArray &dataArray, bool dataAtEnd, qint64 dataSize);
};

class QNetworkReplyHttpImplPrivate: public QNetworkReplyPrivate
{
public:

    static QHttpNetworkRequest::Priority convert(QNetworkRequest::Priority prio);

    QNetworkReplyHttpImplPrivate();
    ~QNetworkReplyHttpImplPrivate();

    void _q_startOperation();

    void _q_cacheLoadReadyRead();

    void _q_bufferOutgoingData();
    void _q_bufferOutgoingDataFinished();

    void _q_cacheSaveDeviceAboutToClose();

    void _q_transferTimedOut();
    void setupTransferTimeout();

    void _q_finished();

    void finished();
    void error(QNetworkReply::NetworkError code, const QString &errorString);
    void _q_error(QNetworkReply::NetworkError code, const QString &errorString);
    void _q_metaDataChanged();

    void checkForRedirect(const int statusCode);

    // incoming from user
    QNetworkAccessManager *manager;
    QNetworkAccessManagerPrivate *managerPrivate;
    QHttpNetworkRequest httpRequest; // There is also a copy in the HTTP thread
    bool synchronous;

    State state;

    // from http thread
    int statusCode;
    QString reasonPhrase;

    // upload
    void maybeDropUploadDevice(const QNetworkRequest &newHttpRequest);
    QNonContiguousByteDevice* createUploadByteDevice();
    std::shared_ptr<QNonContiguousByteDevice> uploadByteDevice;
    qint64 uploadByteDevicePosition;
    bool uploadDeviceChoking; // if we couldn't readPointer() any data at the moment
    QIODevice *outgoingData;
    std::shared_ptr<QRingBuffer> outgoingDataBuffer;
    void emitReplyUploadProgress(qint64 bytesSent, qint64 bytesTotal); // dup?
    void onRedirected(const QUrl &redirectUrl, int httpStatus, int maxRedirectsRemainig);
    void followRedirect();
    qint64 bytesUploaded;


    // cache
    void createCache();
    void completeCacheSave();
    void setCachingEnabled(bool enable);
    bool isCachingEnabled() const;
    bool isCachingAllowed() const;
    void initCacheSaveDevice();
    QIODevice *cacheLoadDevice;
    bool loadingFromCache;

    QIODevice *cacheSaveDevice;
    bool cacheEnabled; // is this for saving?


    QUrl urlForLastAuthentication;
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy lastProxyAuthentication;
#endif


    bool canResume() const;
    void setResumeOffset(quint64 offset);
    quint64 resumeOffset;

    qint64 bytesDownloaded;
    qint64 bytesBuffered;
    // We use this to keep track of whether or not we need to emit readyRead
    // when we deal with signal compression (delaying emission) + decompressing
    // data (potentially receiving bytes that don't end up in the final output):
    qint64 lastReadyReadEmittedSize = 0;

    QTimer *transferTimeout;

    // Only used when the "zero copy" style is used.
    // Please note that the whole "zero copy" download buffer API is private right now. Do not use it.
    qint64 downloadBufferReadPosition;
    qint64 downloadBufferCurrentSize;
    QSharedPointer<char> downloadBufferPointer;
    char* downloadZerocopyBuffer;

    // Will be increased by HTTP thread:
    std::shared_ptr<QAtomicInt> pendingDownloadDataEmissions;
    std::shared_ptr<QAtomicInt> pendingDownloadProgressEmissions;


#ifndef QT_NO_SSL
    QScopedPointer<QSslConfiguration> sslConfiguration;
    bool pendingIgnoreAllSslErrors;
    QList<QSslError> pendingIgnoreSslErrorsList;
#endif

    QNetworkRequest redirectRequest;

    QDecompressHelper decompressHelper;

    bool loadFromCacheIfAllowed(QHttpNetworkRequest &httpRequest);
    void invalidateCache();
    bool sendCacheContents(const QNetworkCacheMetaData &metaData);
    QNetworkCacheMetaData fetchCacheMetaData(const QNetworkCacheMetaData &metaData) const;


    void postRequest(const QNetworkRequest& newHttpRequest);
    QNetworkAccessManager::Operation getRedirectOperation(QNetworkAccessManager::Operation currentOp, int httpStatus);
    QNetworkRequest createRedirectRequest(const QNetworkRequest &originalRequests, const QUrl &url, int maxRedirectsRemainig);
    bool isHttpRedirectResponse() const;

public:
    // From HTTP thread:
    void replyDownloadData(QByteArray);
    void replyFinished();
    void replyDownloadMetaData(const QHttpHeaders &, int, const QString &,
                               bool, QSharedPointer<char>, qint64, qint64, bool, bool);
    void replyDownloadProgressSlot(qint64,qint64);
    void httpAuthenticationRequired(const QHttpNetworkRequest &request, QAuthenticator *auth);
    void httpError(QNetworkReply::NetworkError error, const QString &errorString);
#ifndef QT_NO_SSL
    void replyEncrypted();
    void replySslErrors(const QList<QSslError> &, bool *, QList<QSslError> *);
    void replySslConfigurationChanged(const QSslConfiguration &newSslConfiguration);
    void replyPreSharedKeyAuthenticationRequiredSlot(QSslPreSharedKeyAuthenticator *);
#endif
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);
#endif

    // From QNonContiguousByteDeviceThreadForwardImpl in HTTP thread:
    void resetUploadDataSlot(bool *r);
    void wantUploadDataSlot(qint64);
    void sentUploadDataSlot(qint64, qint64);

    // From user's QNonContiguousByteDevice
    void uploadByteDeviceReadyReadSlot();

    Q_DECLARE_PUBLIC(QNetworkReplyHttpImpl)
};

QT_END_NAMESPACE

#endif
