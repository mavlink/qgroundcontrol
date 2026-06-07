// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKREQUEST_P_H
#define QNETWORKREQUEST_P_H

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
#include <QtNetwork/qhttpheaders.h>
#include "qnetworkrequest.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qlist.h"
#include "QtCore/qhash.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qsharedpointer.h"
#include "QtCore/qpointer.h"

#include <utility>

QT_BEGIN_NAMESPACE

class QNetworkCookie;

// this is the common part between QNetworkRequestPrivate, QNetworkReplyPrivate and QHttpPartPrivate
class QNetworkHeadersPrivate
{
public:
    typedef std::pair<QByteArray, QByteArray> RawHeaderPair;
    typedef QList<RawHeaderPair> RawHeadersList;
    typedef QHash<QNetworkRequest::KnownHeaders, QVariant> CookedHeadersMap;
    typedef QHash<QNetworkRequest::Attribute, QVariant> AttributesMap;

    mutable struct {
        RawHeadersList headersList;
        bool isCached = false;
    } rawHeaderCache;

    QHttpHeaders httpHeaders;
    CookedHeadersMap cookedHeaders;
    AttributesMap attributes;
    QPointer<QObject> originatingObject;

    const RawHeadersList &allRawHeaders() const;
    QList<QByteArray> rawHeadersKeys() const;
    QByteArray rawHeader(QAnyStringView headerName) const;
    void setRawHeader(const QByteArray &key, const QByteArray &value);
    void setCookedHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);

    QHttpHeaders headers() const;
    void setHeaders(const QHttpHeaders &newHeaders);
    void setHeaders(QHttpHeaders &&newHeaders);
    void setHeader(QHttpHeaders::WellKnownHeader name, QByteArrayView value);

    void clearHeaders();

    static QDateTime fromHttpDate(QByteArrayView value);
    static QByteArray toHttpDate(const QDateTime &dt);

    static std::optional<qint64> toInt(QByteArrayView value);

    typedef QList<QNetworkCookie> NetworkCookieList;
    static QByteArray fromCookieList(const NetworkCookieList &cookies);
    static std::optional<NetworkCookieList> toSetCookieList(const QList<QByteArray> &values);
    static std::optional<NetworkCookieList> toCookieList(const QList<QByteArray> &values);

    static RawHeadersList fromHttpToRaw(const QHttpHeaders &headers);
    static QHttpHeaders fromRawToHttp(const RawHeadersList &raw);

private:
    void invalidateHeaderCache();

    void setCookedFromHttp(const QHttpHeaders &newHeaders);
    void parseAndSetHeader(QByteArrayView key, QByteArrayView value);
    void parseAndSetHeader(QNetworkRequest::KnownHeaders key, QByteArrayView value);

};

Q_DECLARE_TYPEINFO(QNetworkHeadersPrivate::RawHeaderPair, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE


#endif
