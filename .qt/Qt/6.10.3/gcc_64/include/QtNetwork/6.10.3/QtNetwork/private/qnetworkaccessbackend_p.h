// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKACCESSBACKEND_P_H
#define QNETWORKACCESSBACKEND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Access API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/qtnetworkglobal.h>

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <QtCore/qobject.h>
#include <QtCore/qflags.h>
#include <QtCore/qbytearrayview.h>
#if QT_CONFIG(ssl)
#include <QtNetwork/qsslconfiguration.h>
#endif

QT_BEGIN_NAMESPACE

class QNetworkReplyImplPrivate;
class QNetworkAccessManagerPrivate;
class QNetworkAccessBackendPrivate;
class Q_NETWORK_EXPORT QNetworkAccessBackend : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QNetworkAccessBackend);

public:
    enum class TargetType {
        Networked = 0x1, // We need to query for proxy in case it is needed
        Local = 0x2, // Local file, generated data or local device
    };
    Q_ENUM(TargetType)
    Q_DECLARE_FLAGS(TargetTypes, TargetType)

    enum class SecurityFeature {
        None = 0x0,
        TLS = 0x1, // We need to set QSslConfiguration
    };
    Q_ENUM(SecurityFeature)
    Q_DECLARE_FLAGS(SecurityFeatures, SecurityFeature)

    enum class IOFeature {
        None = 0x0,
        ZeroCopy = 0x1, // readPointer and advanceReadPointer() is available!
        NeedResetableUpload = 0x2, // Need to buffer upload data
        SupportsSynchronousMode = 0x4, // Used for XMLHttpRequest
    };
    Q_ENUM(IOFeature)
    Q_DECLARE_FLAGS(IOFeatures, IOFeature)

    QNetworkAccessBackend(TargetTypes targetTypes, SecurityFeatures securityFeatures,
                          IOFeatures ioFeatures);
    QNetworkAccessBackend(TargetTypes targetTypes);
    QNetworkAccessBackend(TargetTypes targetTypes, SecurityFeatures securityFeatures);
    QNetworkAccessBackend(TargetTypes targetTypes, IOFeatures ioFeatures);
    virtual ~QNetworkAccessBackend();

    SecurityFeatures securityFeatures() const noexcept;
    TargetTypes targetTypes() const noexcept;
    IOFeatures ioFeatures() const noexcept;

    inline bool needsResetableUploadData() const noexcept
    {
        return ioFeatures() & IOFeature::NeedResetableUpload;
    }

    virtual bool start();
    virtual void open() = 0;
    virtual void close() = 0;
#if QT_CONFIG(ssl)
    virtual void setSslConfiguration(const QSslConfiguration &configuration);
    virtual QSslConfiguration sslConfiguration() const;
#endif
    virtual void ignoreSslErrors();
    virtual void ignoreSslErrors(const QList<QSslError> &errors);
    virtual qint64 bytesAvailable() const = 0;
    virtual QByteArrayView readPointer();
    virtual void advanceReadPointer(qint64 distance);
    virtual qint64 read(char *data, qint64 maxlen);
    virtual bool wantToRead();

#if QT_CONFIG(networkproxy)
    QList<QNetworkProxy> proxyList() const;
#endif
    QUrl url() const;
    void setUrl(const QUrl &url);
    QVariant header(QNetworkRequest::KnownHeaders header) const;
    void setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);
    QByteArray rawHeader(const QByteArray &header) const;
    void setRawHeader(const QByteArray &header, const QByteArray &value);
    QHttpHeaders headers() const;
    void setHeaders(const QHttpHeaders &newHeaders);
    void setHeaders(QHttpHeaders &&newHeaders);
    QNetworkAccessManager::Operation operation() const;

    bool isCachingEnabled() const;
    void setCachingEnabled(bool canCache);

    void setAttribute(QNetworkRequest::Attribute attribute, const QVariant &value);

    QIODevice *createUploadByteDevice();
    QIODevice *uploadByteDevice();

    QAbstractNetworkCache *networkCache() const;

public slots:
    void readyRead();
protected slots:
    void finished();
    void error(QNetworkReply::NetworkError code, const QString &errorString);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);
#endif
    void authenticationRequired(QAuthenticator *auth);
    void metaDataChanged();
    void redirectionRequested(const QUrl &destination);

private:
    void setReplyPrivate(QNetworkReplyImplPrivate *reply);
    void setManagerPrivate(QNetworkAccessManagerPrivate *manager);
    bool isSynchronous() const;
    void setSynchronous(bool synchronous);

    friend class QNetworkAccessManager; // for setReplyPrivate
    friend class QNetworkAccessManagerPrivate; // for setManagerPrivate
    friend class QNetworkReplyImplPrivate; // for {set,is}Synchronous()
};

class Q_NETWORK_EXPORT QNetworkAccessBackendFactory : public QObject
{
    Q_OBJECT
public:
    QNetworkAccessBackendFactory();
    virtual ~QNetworkAccessBackendFactory();
    virtual QStringList supportedSchemes() const = 0;
    virtual QNetworkAccessBackend *create(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request) const = 0;
};

#define QNetworkAccessBackendFactory_iid "org.qt-project.Qt.NetworkAccessBackendFactory"
Q_DECLARE_INTERFACE(QNetworkAccessBackendFactory, QNetworkAccessBackendFactory_iid);

QT_END_NAMESPACE
#endif
