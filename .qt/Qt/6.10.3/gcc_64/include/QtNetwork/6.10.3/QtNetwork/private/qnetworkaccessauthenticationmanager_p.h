// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKACCESSAUTHENTICATIONMANAGER_P_H
#define QNETWORKACCESSAUTHENTICATIONMANAGER_P_H

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
#include "QtNetwork/qnetworkproxy.h"
#include "QtCore/QMutex"

QT_BEGIN_NAMESPACE

class QAuthenticator;
class QAbstractNetworkCache;
class QNetworkAuthenticationCredential;
class QNetworkCookieJar;

class QNetworkAuthenticationCredential
{
public:
    QString domain;
    QString user;
    QString password;
    bool isNull() const {
        return domain.isNull() && user.isNull() && password.isNull();
    }
};
Q_DECLARE_TYPEINFO(QNetworkAuthenticationCredential, Q_RELOCATABLE_TYPE);
inline bool operator<(const QNetworkAuthenticationCredential &t1, const QString &t2)
{ return t1.domain < t2; }
inline bool operator<(const QString &t1, const QNetworkAuthenticationCredential &t2)
{ return t1 < t2.domain; }
inline bool operator<(const QNetworkAuthenticationCredential &t1, const QNetworkAuthenticationCredential &t2)
{ return t1.domain < t2.domain; }

class QNetworkAccessAuthenticationManager
{
public:
    QNetworkAccessAuthenticationManager() {}

    void cacheCredentials(const QUrl &url, const QAuthenticator *auth);
    QNetworkAuthenticationCredential fetchCachedCredentials(const QUrl &url,
                                                             const QAuthenticator *auth = nullptr);

#ifndef QT_NO_NETWORKPROXY
    void cacheProxyCredentials(const QNetworkProxy &proxy, const QAuthenticator *auth);
    QNetworkAuthenticationCredential fetchCachedProxyCredentials(const QNetworkProxy &proxy,
                                                             const QAuthenticator *auth = nullptr);
#endif

    void clearCache();

protected:
    QNetworkAccessCache authenticationCache;
    QMutex mutex;
};

QT_END_NAMESPACE

#endif
