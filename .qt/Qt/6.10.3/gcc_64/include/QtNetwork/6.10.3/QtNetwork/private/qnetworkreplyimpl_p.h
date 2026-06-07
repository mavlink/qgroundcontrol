// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREPLYIMPL_P_H
#define QNETWORKREPLYIMPL_P_H

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
#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"
#include "qnetworkproxy.h"

#include "QtCore/qmap.h"
#include "QtCore/qqueue.h"
#include "QtCore/qbuffer.h"
#include "private/qringbuffer_p.h"
#include <QSharedPointer>

#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractNetworkCache;
class QNetworkAccessBackend;

class QNetworkReplyImplPrivate;
class QNetworkReplyImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyImpl(QObject *parent = nullptr);
    ~QNetworkReplyImpl();
    virtual void abort() override;

    // reimplemented from QNetworkReply / QIODevice
    virtual void close() override;
    virtual qint64 bytesAvailable() const override;
    virtual void setReadBufferSize(qint64 size) override;

    virtual qint64 readData(char *data, qint64 maxlen) override;
    virtual bool event(QEvent *) override;

    Q_DECLARE_PRIVATE(QNetworkReplyImpl)
    Q_PRIVATE_SLOT(d_func(), void _q_startOperation())
    Q_PRIVATE_SLOT(d_func(), void _q_copyReadyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_copyReadChannelFinished())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingData())
    Q_PRIVATE_SLOT(d_func(), void _q_bufferOutgoingDataFinished())

#ifndef QT_NO_SSL
protected:
    void sslConfigurationImplementation(QSslConfiguration &configuration) const override;
    void setSslConfigurationImplementation(const QSslConfiguration &configuration) override;
    virtual void ignoreSslErrors() override;
    virtual void ignoreSslErrorsImplementation(const QList<QSslError> &errors) override;
#endif
};

class QNetworkReplyImplPrivate: public QNetworkReplyPrivate
{
public:
    enum InternalNotifications {
        NotifyDownstreamReadyWrite,
    };

    QNetworkReplyImplPrivate();

    void _q_startOperation();
    void _q_copyReadyRead();
    void _q_copyReadChannelFinished();
    void _q_bufferOutgoingData();
    void _q_bufferOutgoingDataFinished();

    void setup(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
               QIODevice *outgoingData);

    void pauseNotificationHandling();
    void resumeNotificationHandling();
    void backendNotify(InternalNotifications notification);
    void handleNotifications();
    void createCache();
    void completeCacheSave();

    // callbacks from the backend (through the manager):
    void setCachingEnabled(bool enable);
    bool isCachingEnabled() const;
    void consume(qint64 count);
    void emitUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    qint64 nextDownstreamBlockSize() const;

    void initCacheSaveDevice();
    void appendDownstreamDataSignalEmissions();
    void appendDownstreamData(QByteDataBuffer &data);
    void appendDownstreamData(QIODevice *data);

    void setDownloadBuffer(QSharedPointer<char> sp, qint64 size);
    char* getDownloadBuffer(qint64 size);
    void appendDownstreamDataDownloadBuffer(qint64, qint64);

    void finished();
    void error(QNetworkReply::NetworkError code, const QString &errorString);
    void metaDataChanged();
    void redirectionRequested(const QUrl &target);
    void encrypted();
    void sslErrors(const QList<QSslError> &errors);

    void readFromBackend();

    QNetworkAccessBackend *backend;
    QIODevice *outgoingData;
    std::shared_ptr<QRingBuffer> outgoingDataBuffer;
    QIODevice *copyDevice;
    QAbstractNetworkCache *networkCache() const;

    bool cacheEnabled;
    QIODevice *cacheSaveDevice;

    std::vector<InternalNotifications> pendingNotifications;
    bool notificationHandlingPaused;

    QUrl urlForLastAuthentication;
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy lastProxyAuthentication;
    QList<QNetworkProxy> proxyList;
#endif

    qint64 bytesDownloaded;
    qint64 bytesUploaded;

    QString httpReasonPhrase;
    int httpStatusCode;

    State state;

    // Only used when the "zero copy" style is used.
    // Please note that the whole "zero copy" download buffer API is private right now. Do not use it.
    qint64 downloadBufferReadPosition;
    qint64 downloadBufferCurrentSize;
    qint64 downloadBufferMaximumSize;
    QSharedPointer<char> downloadBufferPointer;
    char* downloadBuffer;

    Q_DECLARE_PUBLIC(QNetworkReplyImpl)
};
Q_DECLARE_TYPEINFO(QNetworkReplyImplPrivate::InternalNotifications, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif
