// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREQUEST_H
#define QNETWORKREQUEST_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtNetwork/qhttpheaders.h>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

#include <QtCore/q26numeric.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QSslConfiguration;
class QHttp2Configuration;
class QHttp1Configuration;

class QNetworkRequestPrivate;
class Q_NETWORK_EXPORT QNetworkRequest
{
    Q_GADGET
public:
    enum KnownHeaders {
        ContentTypeHeader,
        ContentLengthHeader,
        LocationHeader,
        LastModifiedHeader,
        CookieHeader,
        SetCookieHeader,
        ContentDispositionHeader,  // added for QMultipartMessage
        UserAgentHeader,
        ServerHeader,
        IfModifiedSinceHeader,
        ETagHeader,
        IfMatchHeader,
        IfNoneMatchHeader,
        NumKnownHeaders
    };
    Q_ENUM(KnownHeaders)

    enum Attribute {
        HttpStatusCodeAttribute,
        HttpReasonPhraseAttribute,
        RedirectionTargetAttribute,
        ConnectionEncryptedAttribute,
        CacheLoadControlAttribute,
        CacheSaveControlAttribute,
        SourceIsFromCacheAttribute,
        DoNotBufferUploadDataAttribute,
        HttpPipeliningAllowedAttribute,
        HttpPipeliningWasUsedAttribute,
        CustomVerbAttribute,
        CookieLoadControlAttribute,
        AuthenticationReuseAttribute,
        CookieSaveControlAttribute,
        MaximumDownloadBufferSizeAttribute, // internal
        DownloadBufferAttribute, // internal
        SynchronousRequestAttribute, // internal
        BackgroundRequestAttribute,
        EmitAllUploadProgressSignalsAttribute,
        Http2AllowedAttribute,
        Http2WasUsedAttribute,
        OriginalContentLengthAttribute,
        RedirectPolicyAttribute,
        Http2DirectAttribute,
        ResourceTypeAttribute, // internal
        AutoDeleteReplyOnFinishAttribute,
        ConnectionCacheExpiryTimeoutSecondsAttribute,
        Http2CleartextAllowedAttribute,
        UseCredentialsAttribute,
        FullLocalServerNameAttribute,

        User = 1000,
        UserMax = 32767
    };
    enum CacheLoadControl {
        AlwaysNetwork,
        PreferNetwork,
        PreferCache,
        AlwaysCache
    };
    enum LoadControl {
        Automatic = 0,
        Manual
    };

    enum Priority {
        HighPriority = 1,
        NormalPriority = 3,
        LowPriority = 5
    };

    enum RedirectPolicy {
        ManualRedirectPolicy,
        NoLessSafeRedirectPolicy,
        SameOriginRedirectPolicy,
        UserVerifiedRedirectPolicy
    };

    enum TransferTimeoutConstant {
        DefaultTransferTimeoutConstant = 30000
    };

    static constexpr auto DefaultTransferTimeout =
            std::chrono::milliseconds(DefaultTransferTimeoutConstant);

    QNetworkRequest();
    explicit QNetworkRequest(const QUrl &url);
    QNetworkRequest(const QNetworkRequest &other);
    ~QNetworkRequest();
    QNetworkRequest &operator=(QNetworkRequest &&other) noexcept { swap(other); return *this; }
    QNetworkRequest &operator=(const QNetworkRequest &other);

    void swap(QNetworkRequest &other) noexcept { d.swap(other.d); }

    bool operator==(const QNetworkRequest &other) const;
    inline bool operator!=(const QNetworkRequest &other) const
    { return !operator==(other); }

    QUrl url() const;
    void setUrl(const QUrl &url);

    QHttpHeaders headers() const;
    void setHeaders(const QHttpHeaders &newHeaders);
    void setHeaders(QHttpHeaders &&newHeaders);

    // "cooked" headers
    QVariant header(KnownHeaders header) const;
    void setHeader(KnownHeaders header, const QVariant &value);

    // raw headers:
#if QT_NETWORK_REMOVED_SINCE(6, 7)
    bool hasRawHeader(const QByteArray &headerName) const;
#endif
    bool hasRawHeader(QAnyStringView headerName) const;
    QList<QByteArray> rawHeaderList() const;
#if QT_NETWORK_REMOVED_SINCE(6, 7)
    QByteArray rawHeader(const QByteArray &headerName) const;
#endif
    QByteArray rawHeader(QAnyStringView headerName) const;
    void setRawHeader(const QByteArray &headerName, const QByteArray &value);

    // attributes
    QVariant attribute(Attribute code, const QVariant &defaultValue = QVariant()) const;
    void setAttribute(Attribute code, const QVariant &value);

#ifndef QT_NO_SSL
    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &configuration);
#endif

    void setOriginatingObject(QObject *object);
    QObject *originatingObject() const;

    Priority priority() const;
    void setPriority(Priority priority);

    // HTTP redirect related
    int maximumRedirectsAllowed() const;
    void setMaximumRedirectsAllowed(int maximumRedirectsAllowed);

    QString peerVerifyName() const;
    void setPeerVerifyName(const QString &peerName);
#if QT_CONFIG(http)
    QHttp1Configuration http1Configuration() const;
    void setHttp1Configuration(const QHttp1Configuration &configuration);

    QHttp2Configuration http2Configuration() const;
    void setHttp2Configuration(const QHttp2Configuration &configuration);

    qint64 decompressedSafetyCheckThreshold() const;
    void setDecompressedSafetyCheckThreshold(qint64 threshold);
#endif // QT_CONFIG(http)

#if QT_CONFIG(http) || defined (Q_OS_WASM)
    QT_NETWORK_INLINE_SINCE(6, 8)
    int transferTimeout() const;
    QT_NETWORK_INLINE_SINCE(6, 8)
    void setTransferTimeout(int timeout);

    std::chrono::milliseconds transferTimeoutAsDuration() const;
    void setTransferTimeout(std::chrono::milliseconds duration = DefaultTransferTimeout);
#endif // QT_CONFIG(http) || defined (Q_OS_WASM)
private:
    QSharedDataPointer<QNetworkRequestPrivate> d;
    friend class QNetworkRequestPrivate;
};

Q_DECLARE_SHARED(QNetworkRequest)

#if QT_NETWORK_INLINE_IMPL_SINCE(6, 8)
#if QT_CONFIG(http) || defined (Q_OS_WASM)
int QNetworkRequest::transferTimeout() const
{
    return q26::saturate_cast<int>(transferTimeoutAsDuration().count());
}

void QNetworkRequest::setTransferTimeout(int timeout)
{
    setTransferTimeout(std::chrono::milliseconds(timeout));
}
#endif // QT_CONFIG(http) || defined (Q_OS_WASM)
#endif // INLINE_SINCE 6.8

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QNetworkRequest, Q_NETWORK_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QNetworkRequest::RedirectPolicy,
                               QNetworkRequest__RedirectPolicy, Q_NETWORK_EXPORT)

#endif
