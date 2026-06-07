// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPNETWORKHEADER_H
#define QHTTPNETWORKHEADER_H

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
#include <QtNetwork/private/qhttpheaderparser_p.h>
#include <QtNetwork/qhttpheaders.h>

#include <qshareddata.h>
#include <qurl.h>

#ifndef Q_OS_WASM
QT_REQUIRE_CONFIG(http);
#endif

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QHttpNetworkHeader
{
public:
    virtual ~QHttpNetworkHeader();
    virtual QUrl url() const = 0;
    virtual void setUrl(const QUrl &url) = 0;

    virtual int majorVersion() const = 0;
    virtual int minorVersion() const = 0;

    virtual qint64 contentLength() const = 0;
    virtual void setContentLength(qint64 length) = 0;

    virtual QHttpHeaders header() const = 0;
    virtual QByteArray headerField(QByteArrayView name, const QByteArray &defaultValue = QByteArray()) const = 0;
    virtual void setHeaderField(const QByteArray &name, const QByteArray &data) = 0;
};

class Q_AUTOTEST_EXPORT QHttpNetworkHeaderPrivate : public QSharedData
{
public:
    QUrl url;
    QHttpHeaderParser parser;

    QHttpNetworkHeaderPrivate(const QUrl &newUrl = QUrl());
    QHttpNetworkHeaderPrivate(const QHttpNetworkHeaderPrivate &other) = default;
    qint64 contentLength() const;
    void setContentLength(qint64 length);

    QByteArray headerField(QByteArrayView name, const QByteArray &defaultValue = QByteArray()) const;
    QList<QByteArray> headerFieldValues(QByteArrayView name) const;
    void setHeaderField(const QByteArray &name, const QByteArray &data);
    void prependHeaderField(const QByteArray &name, const QByteArray &data);
    void clearHeaders();
    QHttpHeaders headers() const;
    bool operator==(const QHttpNetworkHeaderPrivate &other) const;

};


QT_END_NAMESPACE

#endif // QHTTPNETWORKHEADER_H






