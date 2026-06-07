// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKCOOKIE_P_H
#define QNETWORKCOOKIE_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtCore/qdatetime.h"
#include "QtNetwork/qnetworkcookie.h"

QT_BEGIN_NAMESPACE

class QNetworkCookiePrivate: public QSharedData
{
public:
    QNetworkCookiePrivate() = default;
    static QList<QNetworkCookie> parseSetCookieHeaderLine(QByteArrayView cookieString);

    QDateTime expirationDate;
    QString domain;
    QString path;
    QString comment;
    QByteArray name;
    QByteArray value;
    QNetworkCookie::SameSite sameSite = QNetworkCookie::SameSite::Default;
    bool secure = false;
    bool httpOnly = false;
};

static inline bool isLWS(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int nextNonWhitespace(QByteArrayView text, int from)
{
    // RFC 2616 defines linear whitespace as:
    //  LWS = [CRLF] 1*( SP | HT )
    // We ignore the fact that CRLF must come as a pair at this point
    // It's an invalid HTTP header if that happens.
    while (from < text.size()) {
        if (isLWS(text.at(from)))
            ++from;
        else
            return from;        // non-whitespace
    }

    // reached the end
    return text.size();
}

QT_END_NAMESPACE

#endif
