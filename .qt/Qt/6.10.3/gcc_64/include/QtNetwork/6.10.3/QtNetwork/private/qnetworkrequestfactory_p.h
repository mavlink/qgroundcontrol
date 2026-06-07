// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREQUESTFACTORY_P_H
#define QNETWORKREQUESTFACTORY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access framework.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/qhttpheaders.h>
#include <QtNetwork/qnetworkrequest.h>
#if QT_CONFIG(ssl)
#include <QtNetwork/qsslconfiguration.h>
#endif
#include <QtCore/qhash.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qurl.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QNetworkRequestFactoryPrivate : public QSharedData
{
public:
    QNetworkRequestFactoryPrivate();
    explicit QNetworkRequestFactoryPrivate(const QUrl &baseUrl);
    QNetworkRequest newRequest(const QUrl &url) const;
    QUrl requestUrl(const QString *path = nullptr, const QUrlQuery *query = nullptr) const;

#if QT_CONFIG(ssl)
    QSslConfiguration sslConfig;
#endif
    QUrl baseUrl;
    QHttpHeaders headers;
    QByteArray bearerToken;
    QString userName;
    QString password;
    QUrlQuery queryParameters;
    QNetworkRequest::Priority priority = QNetworkRequest::NormalPriority;
    std::chrono::milliseconds transferTimeout{0};
    QHash<QNetworkRequest::Attribute, QVariant> attributes;
};

QT_END_NAMESPACE

#endif // QNETWORKREQUESTFACTORY_P_H
