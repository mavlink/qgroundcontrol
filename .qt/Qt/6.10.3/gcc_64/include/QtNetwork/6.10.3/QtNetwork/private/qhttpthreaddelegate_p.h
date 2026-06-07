// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPTHREADDELEGATE_H
#define QHTTPTHREADDELEGATE_H


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
#include <QObject>
#include <QThreadStorage>
#include <QNetworkProxy>
#include <QSslConfiguration>
#include <QSslError>
#include <QList>
#include <QNetworkReply>
#include "qhttpnetworkrequest_p.h"
#include "qhttpnetworkconnection_p.h"
#include "qhttp1configuration.h"
#include "qhttp2configuration.h"
#include <QSharedPointer>
#include <QScopedPointer>
#include "private/qnoncontiguousbytedevice_p.h"
#include "qnetworkaccessauthenticationmanager_p.h"
#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/qhttpheaders.h>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QHttpNetworkReply;
class QEventLoop;
class QNetworkAccessCache;
class QNetworkAccessCachedHttpConnection;

class QHttpThreadDelegate : public QObject
{
    Q_OBJECT
public:
    explicit QHttpThreadDelegate(QObject *parent = nullptr);

    ~QHttpThreadDelegate();

    // incoming
    bool ssl;
#ifndef QT_NO_SSL
    QScopedPointer<QSslConfiguration> incomingSslConfiguration;
#endif
    QHttpNetworkRequest httpRequest;
    qint64 downloadBufferMaximumSize;
    qint64 readBufferMaxSize;
    qint64 bytesEmitted;
    // From backend, modified by us for signal compression
    std::shared_ptr<QAtomicInt> pendingDownloadData;
    std::shared_ptr<QAtomicInt> pendingDownloadProgress;
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy cacheProxy;
    QNetworkProxy transparentProxy;
#endif
    std::shared_ptr<QNetworkAccessAuthenticationManager> authenticationManager;
    bool synchronous;
    qint64 connectionCacheExpiryTimeoutSeconds;

    // outgoing, Retrieved in the synchronous HTTP case
    QByteArray synchronousDownloadData;
    QHttpHeaders incomingHeaders;
    int incomingStatusCode;
    QString incomingReasonPhrase;
    bool isPipeliningUsed;
    bool isHttp2Used;
    bool isCompressed = false;
    qint64 incomingContentLength;
    qint64 removedContentLength;
    QNetworkReply::NetworkError incomingErrorCode;
    QString incomingErrorDetail;
    QHttp1Configuration http1Parameters;
    QHttp2Configuration http2Parameters;

protected:
    // The zerocopy download buffer, if used:
    QSharedPointer<char> downloadBuffer;
    // The QHttpNetworkConnection that is used
    QNetworkAccessCachedHttpConnection *httpConnection;
    QByteArray cacheKey;
    QHttpNetworkReply *httpReply;

    // Used for implementing the synchronous HTTP, see startRequestSynchronously()
    QEventLoop *synchronousRequestLoop;

signals:
    void authenticationRequired(const QHttpNetworkRequest &request, QAuthenticator *);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *);
#endif
#ifndef QT_NO_SSL
    void encrypted();
    void sslErrors(const QList<QSslError> &, bool *, QList<QSslError> *);
    void sslConfigurationChanged(const QSslConfiguration &);
    void preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *);
#endif
    void socketStartedConnecting();
    void requestSent();
    void downloadMetaData(const QHttpHeaders &, int, const QString &, bool,
                          QSharedPointer<char>, qint64, qint64, bool, bool);
    void downloadProgress(qint64, qint64);
    void downloadData(const QByteArray &);
    void error(QNetworkReply::NetworkError, const QString &);
    void downloadFinished();
    void redirected(const QUrl &url, int httpStatus, int maxRedirectsRemainig);

public slots:
    // This are called via QueuedConnection from user thread
    void startRequest();
    void abortRequest();
    void readBufferSizeChanged(qint64 size);
    void readBufferFreed(qint64 size);

    // This is called with a BlockingQueuedConnection from user thread
    void startRequestSynchronously();
protected slots:
    // From QHttp*
    void readyReadSlot();
    void finishedSlot();
    void finishedWithErrorSlot(QNetworkReply::NetworkError errorCode, const QString &detail = QString());
    void synchronousFinishedSlot();
    void synchronousFinishedWithErrorSlot(QNetworkReply::NetworkError errorCode, const QString &detail = QString());
    void headerChangedSlot();
    void synchronousHeaderChangedSlot();
    void dataReadProgressSlot(qint64 done, qint64 total);
    void cacheCredentialsSlot(const QHttpNetworkRequest &request, QAuthenticator *authenticator);
#ifndef QT_NO_SSL
    void encryptedSlot();
    void sslErrorsSlot(const QList<QSslError> &errors);
    void preSharedKeyAuthenticationRequiredSlot(QSslPreSharedKeyAuthenticator *authenticator);
#endif

    void synchronousAuthenticationRequiredSlot(const QHttpNetworkRequest &request, QAuthenticator *);
#ifndef QT_NO_NETWORKPROXY
    void synchronousProxyAuthenticationRequiredSlot(const QNetworkProxy &, QAuthenticator *);
#endif

protected:
    // Cache for all the QHttpNetworkConnection objects.
    // This is per thread.
    static QThreadStorage<QNetworkAccessCache *> connections;

};

// This QNonContiguousByteDevice is connected to the QNetworkAccessHttpBackend
// and represents the PUT/POST data.
class QNonContiguousByteDeviceThreadForwardImpl : public QNonContiguousByteDevice
{
    Q_OBJECT
protected:
    bool wantDataPending = false;
    qint64 m_amount = 0;
    char *m_data = nullptr;
    QByteArray m_dataArray;
    bool m_atEnd = false;
    qint64 m_size = 0;
    qint64 m_pos = 0; // to match calls of haveDataSlot with the expected position
public:
    QNonContiguousByteDeviceThreadForwardImpl(bool aE, qint64 s)
        : QNonContiguousByteDevice(),
          m_atEnd(aE),
          m_size(s)
    {
    }

    ~QNonContiguousByteDeviceThreadForwardImpl()
    {
    }

    qint64 pos() const override
    {
        return m_pos;
    }

    const char* readPointer(qint64 maximumLength, qint64 &len) override
    {
        if (m_amount > 0) {
            len = m_amount;
            return m_data;
        }

        if (m_atEnd) {
            len = -1;
        } else if (!wantDataPending) {
            len = 0;
            wantDataPending = true;
            emit wantData(maximumLength);
        } else {
            // Do nothing, we already sent a wantData signal and wait for results
            len = 0;
        }
        return nullptr;
    }

    bool advanceReadPointer(qint64 a) override
    {
        if (m_data == nullptr)
            return false;

        m_amount -= a;
        m_data += a;
        m_pos += a;

        // To main thread to inform about our state. The m_pos will be sent as a sanity check.
        emit processedData(m_pos, a);

        return true;
    }

    bool atEnd() const override
    {
        if (m_amount > 0)
            return false;
        else
            return m_atEnd;
    }

    bool reset() override
    {
        m_amount = 0;
        m_data = nullptr;
        m_dataArray.clear();

        if (wantDataPending) {
            // had requested the user thread to send some data (only 1 in-flight at any moment)
            wantDataPending = false;
        }

        // Communicate as BlockingQueuedConnection
        bool b = false;
        emit resetData(&b);
        if (b) {
            // the reset succeeded, we're at pos 0 again
            m_pos = 0;
            m_atEnd = false;
            // the HTTP code will anyway abort the request if !b.
        }
        return b;
    }

    qint64 size() const override
    {
        return m_size;
    }

public slots:
    // From user thread:
    void haveDataSlot(qint64 pos, const QByteArray &dataArray, bool dataAtEnd, qint64 dataSize)
    {
        if (pos != m_pos) {
            // Sometimes when re-sending a request in the qhttpnetwork* layer there is a pending haveData from the
            // user thread on the way to us. We need to ignore it since it is the data for the wrong(later) chunk.
            return;
        }
        wantDataPending = false;

        m_dataArray = dataArray;
        m_data = const_cast<char*>(m_dataArray.constData());
        m_amount = dataArray.size();

        m_atEnd = dataAtEnd;
        m_size = dataSize;

        // This will tell the HTTP code (QHttpNetworkConnectionChannel) that we have data available now
        emit readyRead();
    }

signals:
    // void readyRead(); in parent class
    // void readProgress(qint64 current, qint64 total); happens in the main thread with the real bytedevice

    // to main thread:
    void wantData(qint64);
    void processedData(qint64 pos, qint64 amount);
    void resetData(bool *b);
};

QT_END_NAMESPACE

#endif // QHTTPTHREADDELEGATE_H
