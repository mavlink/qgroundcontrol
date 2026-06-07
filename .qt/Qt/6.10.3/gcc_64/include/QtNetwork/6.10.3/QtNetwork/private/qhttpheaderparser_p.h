// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPHEADERPARSER_H
#define QHTTPHEADERPARSER_H

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

#include <QByteArray>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE

namespace HeaderConstants {

// We previously used 8K, which is common on server side, but it turned out to
// not be enough for various uses. Historically Firefox used 10K as the limit of
// a single field, but some Location headers and Authorization challenges can
// get even longer. Other browsers, such as Chrome, instead have a limit on the
// total size of all the headers (as well as extra limits on some of the
// individual fields). We'll use 100K as our default limit, which would be a ridiculously large
// header, with the possibility to override it where we need to.
static constexpr int MAX_HEADER_FIELD_SIZE = 100 * 1024;
// Taken from http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfields
static constexpr int MAX_HEADER_FIELDS = 100;
// Chromium has a limit on the total size of the header set to 256KB,
// which is a reasonable default for QNetworkAccessManager.
// https://stackoverflow.com/a/3436155
static constexpr int MAX_TOTAL_HEADER_SIZE = 256 * 1024;

}

class Q_NETWORK_EXPORT QHttpHeaderParser
{
public:
    QHttpHeaderParser();

    void clear();
    bool parseHeaders(QByteArrayView headers);
    bool parseStatus(QByteArrayView status);

    const QHttpHeaders& headers() const &;
    QHttpHeaders headers() &&;
    void setStatusCode(int code);
    int getStatusCode() const;
    int getMajorVersion() const;
    void setMajorVersion(int version);
    int getMinorVersion() const;
    void setMinorVersion(int version);
    QString getReasonPhrase() const;
    void setReasonPhrase(const QString &reason);

    QByteArray firstHeaderField(QByteArrayView name,
                                const QByteArray &defaultValue = QByteArray()) const;
    QByteArray combinedHeaderValue(QByteArrayView name,
                                   const QByteArray &defaultValue = QByteArray()) const;
    QList<QByteArray> headerFieldValues(QByteArrayView name) const;
    void setHeaderField(const QByteArray &name, const QByteArray &data);
    void prependHeaderField(const QByteArray &name, const QByteArray &data);
    void appendHeaderField(const QByteArray &name, const QByteArray &data);
    void removeHeaderField(QByteArrayView name);
    void clearHeaders();

    void setMaxHeaderFieldSize(qsizetype size) { maxFieldSize = size; }
    qsizetype maxHeaderFieldSize() const { return maxFieldSize; }

    void setMaxTotalHeaderSize(qsizetype size) { maxTotalSize = size; }
    qsizetype maxTotalHeaderSize() const { return maxTotalSize; }

    void setMaxHeaderFields(qsizetype count) { maxFieldCount = count; }
    qsizetype maxHeaderFields() const { return maxFieldCount; }

private:
    QHttpHeaders fields;
    QString reasonPhrase;
    int statusCode;
    int majorVersion;
    int minorVersion;

    qsizetype maxFieldSize = HeaderConstants::MAX_HEADER_FIELD_SIZE;
    qsizetype maxTotalSize = HeaderConstants::MAX_TOTAL_HEADER_SIZE;
    qsizetype maxFieldCount = HeaderConstants::MAX_HEADER_FIELDS;
};


QT_END_NAMESPACE

#endif // QHTTPHEADERPARSER_H
