// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKACCESSMANAGER_P_H
#define QNETWORKACCESSMANAGER_P_H

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
#include "qnetworkaccessmanager.h"
#include "qnetworkaccesscache_p.h"
#include "qnetworkaccessbackend_p.h"
#include "private/qnetconmonitor_p.h"
#include "qnetworkrequest.h"
#include "qhsts_p.h"
#include "private/qobject_p.h"
#include "QtNetwork/qnetworkproxy.h"
#include "qnetworkaccessauthenticationmanager_p.h"

#if QT_CONFIG(settings)
#include "qhstsstore_p.h"
#endif // QT_CONFIG(settings)

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QAbstractNetworkCache;
class QNetworkAuthenticationCredential;
class QNetworkCookieJar;

class QNetworkAccessManagerPrivate: public QObjectPrivate
{
public:
    QNetworkAccessManagerPrivate()
        : networkCache(nullptr),
          cookieJar(nullptr),
          thread(nullptr),
#ifndef QT_NO_NETWORKPROXY
          proxyFactory(nullptr),
#endif
          cookieJarCreated(false),
          defaultAccessControl(true),
          redirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy),
          authenticationManager(std::make_shared<QNetworkAccessAuthenticationManager>())
    {
    }
    ~QNetworkAccessManagerPrivate();

    QThread * createThread();
    void destroyThread();

    void _q_replyFinished(QNetworkReply *reply);
    void _q_replyEncrypted(QNetworkReply *reply);
    void _q_replySslErrors(const QList<QSslError> &errors);
    void _q_replyPreSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator);
    QNetworkReply *postProcess(QNetworkReply *reply);
    void createCookieJar() const;

    void authenticationRequired(QAuthenticator *authenticator,
                                QNetworkReply *reply,
                                bool synchronous,
                                QUrl &url,
                                QUrl *urlForLastAuthentication,
                                bool allowAuthenticationReuse = true);
    void cacheCredentials(const QUrl &url, const QAuthenticator *auth);
    QNetworkAuthenticationCredential *fetchCachedCredentials(const QUrl &url,
                                                             const QAuthenticator *auth = nullptr);

#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QUrl &url,
                                const QNetworkProxy &proxy,
                                bool synchronous,
                                QAuthenticator *authenticator,
                                QNetworkProxy *lastProxyAuthentication);
    void cacheProxyCredentials(const QNetworkProxy &proxy, const QAuthenticator *auth);
    QNetworkAuthenticationCredential *fetchCachedProxyCredentials(const QNetworkProxy &proxy,
                                                             const QAuthenticator *auth = nullptr);
    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query);
#endif

    QNetworkAccessBackend *findBackend(QNetworkAccessManager::Operation op, const QNetworkRequest &request);
    QStringList backendSupportedSchemes() const;

#if QT_CONFIG(http) || defined(Q_OS_WASM)
    QNetworkRequest prepareMultipart(const QNetworkRequest &request, QHttpMultiPart *multiPart);
#endif

    void ensureBackendPluginsLoaded();

    // this is the cache for storing downloaded files
    QAbstractNetworkCache *networkCache;

    QNetworkCookieJar *cookieJar;

    QThread *thread;


#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    QNetworkProxyFactory *proxyFactory;
#endif

    bool cookieJarCreated;
    bool defaultAccessControl;
    QNetworkRequest::RedirectPolicy redirectPolicy = QNetworkRequest::NoLessSafeRedirectPolicy;

    // The cache with authorization data:
    std::shared_ptr<QNetworkAccessAuthenticationManager> authenticationManager;

    Q_AUTOTEST_EXPORT static void clearAuthenticationCache(QNetworkAccessManager *manager);
    Q_AUTOTEST_EXPORT static void clearConnectionCache(QNetworkAccessManager *manager);

    QHstsCache stsCache;
#if QT_CONFIG(settings)
    QScopedPointer<QHstsStore> stsStore;
#endif // QT_CONFIG(settings)
    bool stsEnabled = false;

    bool autoDeleteReplies = false;

    std::chrono::milliseconds transferTimeout{0};

    Q_DECLARE_PUBLIC(QNetworkAccessManager)
};

QT_END_NAMESPACE

#endif
