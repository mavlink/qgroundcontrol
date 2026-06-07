// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPNETWORKREQUEST_H
#define QHTTPNETWORKREQUEST_H

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

#include <private/qhttpnetworkheader_p.h>
#include <QtNetwork/qnetworkrequest.h>
#include <qmetatype.h>

#ifndef Q_OS_WASM
QT_REQUIRE_CONFIG(http);
#endif

QT_BEGIN_NAMESPACE

class QNonContiguousByteDevice;

class QHttpNetworkRequestPrivate;
class Q_AUTOTEST_EXPORT QHttpNetworkRequest: public QHttpNetworkHeader
{
public:
    enum Operation {
        Options,
        Get,
        Head,
        Post,
        Put,
        Delete,
        Trace,
        Connect,
        Custom
    };

    enum Priority {
        HighPriority,
        NormalPriority,
        LowPriority
    };

    explicit QHttpNetworkRequest(const QUrl &url = QUrl(), Operation operation = Get, Priority priority = NormalPriority);
    QHttpNetworkRequest(const QHttpNetworkRequest &other);
    ~QHttpNetworkRequest() override;
    QHttpNetworkRequest &operator=(const QHttpNetworkRequest &other);
    bool operator==(const QHttpNetworkRequest &other) const;

    QUrl url() const override;
    void setUrl(const QUrl &url) override;

    int majorVersion() const override;
    int minorVersion() const override;

    qint64 contentLength() const override;
    void setContentLength(qint64 length) override;

    QHttpHeaders header() const override;
    QByteArray headerField(QByteArrayView name, const QByteArray &defaultValue = QByteArray()) const override;
    void setHeaderField(const QByteArray &name, const QByteArray &data) override;
    void prependHeaderField(const QByteArray &name, const QByteArray &data);
    void clearHeaders();

    Operation operation() const;
    void setOperation(Operation operation);

    QByteArray customVerb() const;
    void setCustomVerb(const QByteArray &customOperation);

    Priority priority() const;
    void setPriority(Priority priority);

    bool isPipeliningAllowed() const;
    void setPipeliningAllowed(bool b);

    bool isHTTP2Allowed() const;
    void setHTTP2Allowed(bool b);

    bool isHTTP2Direct() const;
    void setHTTP2Direct(bool b);

    bool isH2cAllowed() const;
    void setH2cAllowed(bool b);

    bool withCredentials() const;
    void setWithCredentials(bool b);

    bool isSsl() const;
    void setSsl(bool);

    bool isPreConnect() const;
    void setPreConnect(bool preConnect);

    bool isFollowRedirects() const;
    void setRedirectPolicy(QNetworkRequest::RedirectPolicy policy);
    QNetworkRequest::RedirectPolicy redirectPolicy() const;

    int redirectCount() const;
    void setRedirectCount(int count);

    void setUploadByteDevice(QNonContiguousByteDevice *bd);
    QNonContiguousByteDevice* uploadByteDevice() const;

    QByteArray methodName() const;
    QByteArray uri(bool throughProxy) const;

    QString peerVerifyName() const;
    void setPeerVerifyName(const QString &peerName);

    QString fullLocalServerName() const;
    void setFullLocalServerName(const QString &fullServerName);

    bool methodIsIdempotent() const;

private:
    QSharedDataPointer<QHttpNetworkRequestPrivate> d;
    friend class QHttpNetworkRequestPrivate;
    friend class QHttpNetworkConnectionPrivate;
    friend class QHttpNetworkConnectionChannel;
    friend class QHttpProtocolHandler;
    friend class QHttp2ProtocolHandler;
    friend class QSpdyProtocolHandler;
};

class QHttpNetworkRequestPrivate : public QHttpNetworkHeaderPrivate
{
public:
    QHttpNetworkRequestPrivate(QHttpNetworkRequest::Operation op,
        QHttpNetworkRequest::Priority pri, const QUrl &newUrl = QUrl());
    QHttpNetworkRequestPrivate(const QHttpNetworkRequestPrivate &other);
    ~QHttpNetworkRequestPrivate();
    bool operator==(const QHttpNetworkRequestPrivate &other) const;

    static QByteArray header(const QHttpNetworkRequest &request, bool throughProxy);

    QHttpNetworkRequest::Operation operation;
    QByteArray customVerb;
    QString fullLocalServerName; // for local sockets
    QHttpNetworkRequest::Priority priority;
    mutable QNonContiguousByteDevice* uploadByteDevice;
    bool autoDecompress;
    bool pipeliningAllowed;
    bool http2Allowed;
    bool http2Direct;
    bool h2cAllowed = false;
    bool withCredentials;
    bool ssl = false;
    bool preConnect;
    bool needResendWithCredentials = false;
    int redirectCount;
    QNetworkRequest::RedirectPolicy redirectPolicy;
    QString peerVerifyName;
};


QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QHttpNetworkRequest, Q_AUTOTEST_EXPORT)

#endif // QHTTPNETWORKREQUEST_H
