// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREQUESTFACTORY_H
#define QNETWORKREQUESTFACTORY_H

#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qhttpheaders.h>

#include <QtCore/qcompare.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QDebug;
#if QT_CONFIG(ssl)
class QSslConfiguration;
#endif

class QNetworkRequestFactoryPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QNetworkRequestFactoryPrivate, Q_NETWORK_EXPORT)

class QNetworkRequestFactory
{
public:
    Q_NETWORK_EXPORT QNetworkRequestFactory();
    Q_NETWORK_EXPORT explicit QNetworkRequestFactory(const QUrl &baseUrl);
    Q_NETWORK_EXPORT ~QNetworkRequestFactory();

    Q_NETWORK_EXPORT QNetworkRequestFactory(const QNetworkRequestFactory &other);
    QNetworkRequestFactory(QNetworkRequestFactory &&other) noexcept = default;
    Q_NETWORK_EXPORT QNetworkRequestFactory &operator=(const QNetworkRequestFactory &other);

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QNetworkRequestFactory)
    void swap(QNetworkRequestFactory &other) noexcept { d.swap(other.d); }

    Q_NETWORK_EXPORT QUrl baseUrl() const;
    Q_NETWORK_EXPORT void setBaseUrl(const QUrl &url);

#if QT_CONFIG(ssl)
    Q_NETWORK_EXPORT QSslConfiguration sslConfiguration() const;
    Q_NETWORK_EXPORT void setSslConfiguration(const QSslConfiguration &configuration);
#endif

    Q_NETWORK_EXPORT QNetworkRequest createRequest() const;
    Q_NETWORK_EXPORT QNetworkRequest createRequest(const QUrlQuery &query) const;
    Q_NETWORK_EXPORT QNetworkRequest createRequest(const QString &path) const;
    Q_NETWORK_EXPORT QNetworkRequest createRequest(const QString &path, const QUrlQuery &query) const;

    Q_NETWORK_EXPORT void setCommonHeaders(const QHttpHeaders &headers);
    Q_NETWORK_EXPORT QHttpHeaders commonHeaders() const;
    Q_NETWORK_EXPORT void clearCommonHeaders();

    Q_NETWORK_EXPORT QByteArray bearerToken() const;
    Q_NETWORK_EXPORT void setBearerToken(const QByteArray &token);
    Q_NETWORK_EXPORT void clearBearerToken();

    Q_NETWORK_EXPORT QString userName() const;
    Q_NETWORK_EXPORT void setUserName(const QString &userName);
    Q_NETWORK_EXPORT void clearUserName();

    Q_NETWORK_EXPORT QString password() const;
    Q_NETWORK_EXPORT void setPassword(const QString &password);
    Q_NETWORK_EXPORT void clearPassword();

    Q_NETWORK_EXPORT void setTransferTimeout(std::chrono::milliseconds timeout);
    Q_NETWORK_EXPORT std::chrono::milliseconds transferTimeout() const;

    Q_NETWORK_EXPORT QUrlQuery queryParameters() const;
    Q_NETWORK_EXPORT void setQueryParameters(const QUrlQuery &query);
    Q_NETWORK_EXPORT void clearQueryParameters();

    Q_NETWORK_EXPORT void setPriority(QNetworkRequest::Priority priority);
    Q_NETWORK_EXPORT QNetworkRequest::Priority priority() const;

    Q_NETWORK_EXPORT QVariant attribute(QNetworkRequest::Attribute attribute) const;
    Q_NETWORK_EXPORT QVariant attribute(QNetworkRequest::Attribute attribute,
                                        const QVariant &defaultValue) const;
    Q_NETWORK_EXPORT void setAttribute(QNetworkRequest::Attribute attribute, const QVariant &value);
    Q_NETWORK_EXPORT void clearAttribute(QNetworkRequest::Attribute attribute);
    Q_NETWORK_EXPORT void clearAttributes();

private:
#ifndef QT_NO_DEBUG_STREAM
    friend Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QNetworkRequestFactory &reply);
#endif

    QExplicitlySharedDataPointer<QNetworkRequestFactoryPrivate> d;
};

Q_DECLARE_SHARED(QNetworkRequestFactory)

QT_END_NAMESPACE

#endif // QNETWORKREQUESTFACTORY_H
