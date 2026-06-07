// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDNSLOOKUP_P_H
#define QDNSLOOKUP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QDnsLookup class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtCore/qmutex.h"
#include "QtCore/qrunnable.h"
#if QT_CONFIG(thread)
#include "QtCore/qthreadpool.h"
#endif
#include "QtNetwork/qdnslookup.h"
#include "QtNetwork/qhostaddress.h"
#include "private/qobject_p.h"
#include "private/qurl_p.h"

#if QT_CONFIG(ssl)
#  include "qsslconfiguration.h"
#endif

QT_REQUIRE_CONFIG(dnslookup);

QT_BEGIN_NAMESPACE

//#define QDNSLOOKUP_DEBUG

constexpr qsizetype MaxDomainNameLength = 255;
constexpr quint16 DnsPort = 53;
constexpr quint16 DnsOverTlsPort = 853;

class QDnsLookupRunnable;
QDebug operator<<(QDebug &, QDnsLookupRunnable *);

class QDnsLookupReply
{
public:
    QDnsLookup::Error error = QDnsLookup::NoError;
    bool authenticData = false;
    QString errorString;

    QList<QDnsDomainNameRecord> canonicalNameRecords;
    QList<QDnsHostAddressRecord> hostAddressRecords;
    QList<QDnsMailExchangeRecord> mailExchangeRecords;
    QList<QDnsDomainNameRecord> nameServerRecords;
    QList<QDnsDomainNameRecord> pointerRecords;
    QList<QDnsServiceRecord> serviceRecords;
    QList<QDnsTlsAssociationRecord> tlsAssociationRecords;
    QList<QDnsTextRecord> textRecords;

#if QT_CONFIG(ssl)
    std::optional<QSslConfiguration> sslConfiguration;
#endif

    // helper methods
    void setError(QDnsLookup::Error err, QString &&msg)
    {
        error = err;
        errorString = std::move(msg);
    }

    void makeResolverSystemError(int code = -1)
    {
        Q_ASSERT(allAreEmpty());
        setError(QDnsLookup::ResolverError, qt_error_string(code));
    }

    void makeTimeoutError()
    {
        Q_ASSERT(allAreEmpty());
        setError(QDnsLookup::TimeoutError, QDnsLookup::tr("Request timed out"));
    }

    void makeDnsRcodeError(quint8 rcode)
    {
        Q_ASSERT(allAreEmpty());
        switch (rcode) {
        case 1:     // FORMERR
            error = QDnsLookup::InvalidRequestError;
            errorString = QDnsLookup::tr("Server could not process query");
            return;
        case 2:     // SERVFAIL
        case 4:     // NOTIMP
            error = QDnsLookup::ServerFailureError;
            errorString = QDnsLookup::tr("Server failure");
            return;
        case 3:     // NXDOMAIN
            error = QDnsLookup::NotFoundError;
            errorString = QDnsLookup::tr("Non existent domain");
            return;
        case 5:     // REFUSED
            error = QDnsLookup::ServerRefusedError;
            errorString = QDnsLookup::tr("Server refused to answer");
            return;
        default:
            error = QDnsLookup::InvalidReplyError;
            errorString = QDnsLookup::tr("Invalid reply received (rcode %1)")
                    .arg(rcode);
            return;
        }
    }

    void makeInvalidReplyError(QString &&msg = QString())
    {
        if (msg.isEmpty())
            msg = QDnsLookup::tr("Invalid reply received");
        else
            msg = QDnsLookup::tr("Invalid reply received (%1)").arg(std::move(msg));
        *this = QDnsLookupReply();  // empty our lists
        setError(QDnsLookup::InvalidReplyError, std::move(msg));
    }

private:
    bool allAreEmpty() const
    {
        return canonicalNameRecords.isEmpty()
                && hostAddressRecords.isEmpty()
                && mailExchangeRecords.isEmpty()
                && nameServerRecords.isEmpty()
                && pointerRecords.isEmpty()
                && serviceRecords.isEmpty()
                && tlsAssociationRecords.isEmpty()
                && textRecords.isEmpty();
    }
};

class QDnsLookupPrivate : public QObjectPrivate
{
public:
    QDnsLookupPrivate()
        : type(QDnsLookup::A)
        , port(0)
        , protocol(QDnsLookup::Standard)
    { }

    void nameChanged()
    {
        emit q_func()->nameChanged(name);
    }
    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QString, name,
                               &QDnsLookupPrivate::nameChanged);

    void nameserverChanged()
    {
        emit q_func()->nameserverChanged(nameserver);
    }
    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QHostAddress, nameserver,
                               &QDnsLookupPrivate::nameserverChanged);

    void typeChanged()
    {
        emit q_func()->typeChanged(type);
    }

    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QDnsLookup::Type,
                               type, &QDnsLookupPrivate::typeChanged);

    void nameserverPortChanged()
    {
        emit q_func()->nameserverPortChanged(port);
    }

    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, quint16,
                               port, &QDnsLookupPrivate::nameserverPortChanged);

    void nameserverProtocolChanged()
    {
        emit q_func()->nameserverProtocolChanged(protocol);
    }

    Q_OBJECT_BINDABLE_PROPERTY(QDnsLookupPrivate, QDnsLookup::Protocol,
                               protocol, &QDnsLookupPrivate::nameserverProtocolChanged);

    QDnsLookupReply reply;
    QDnsLookupRunnable *runnable = nullptr;
    bool isFinished = false;

#if QT_CONFIG(ssl)
    std::optional<QSslConfiguration> sslConfiguration;
#endif

    Q_DECLARE_PUBLIC(QDnsLookup)
};

class QDnsLookupRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
#ifdef Q_OS_WIN
    using EncodedLabel = QString;
#else
    using EncodedLabel = QByteArray;
#endif
    // minimum IPv6 MTU (1280) minus the IPv6 (40) and UDP headers (8)
    static constexpr qsizetype ReplyBufferSize = 1280 - 40 - 8;
    using ReplyBuffer = QVarLengthArray<unsigned char, ReplyBufferSize>;

    QDnsLookupRunnable(const QDnsLookupPrivate *d);
    void run() override;
    bool sendDnsOverTls(QDnsLookupReply *reply, QSpan<unsigned char> query, ReplyBuffer &response);

signals:
    void finished(const QDnsLookupReply &reply);

private:
    template <typename T> static QString decodeLabel(T encodedLabel)
    {
        return qt_ACE_do(encodedLabel.toString(), NormalizeAce, ForbidLeadingDot);
    }
    void query(QDnsLookupReply *reply);

    EncodedLabel requestName;
    QHostAddress nameserver;
    QDnsLookup::Type requestType;
    quint16 port;
    QDnsLookup::Protocol protocol;

#if QT_CONFIG(ssl)
    std::optional<QSslConfiguration> sslConfiguration;
#endif
    friend QDebug operator<<(QDebug &, QDnsLookupRunnable *);
};

class QDnsRecordPrivate : public QSharedData
{
public:
    QDnsRecordPrivate()
        : timeToLive(0)
    { }

    QString name;
    quint32 timeToLive;
};

class QDnsDomainNameRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsDomainNameRecordPrivate()
    { }

    QString value;
};

class QDnsHostAddressRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsHostAddressRecordPrivate()
    { }

    QHostAddress value;
};

class QDnsMailExchangeRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsMailExchangeRecordPrivate()
        : preference(0)
    { }

    QString exchange;
    quint16 preference;
};

class QDnsServiceRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsServiceRecordPrivate()
        : port(0),
          priority(0),
          weight(0)
    { }

    QString target;
    quint16 port;
    quint16 priority;
    quint16 weight;
};

class QDnsTextRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsTextRecordPrivate()
    { }

    QList<QByteArray> values;
};

class QDnsTlsAssociationRecordPrivate : public QDnsRecordPrivate
{
public:
    QDnsTlsAssociationRecord::CertificateUsage usage;
    QDnsTlsAssociationRecord::Selector selector;
    QDnsTlsAssociationRecord::MatchingType matchType;
    QByteArray value;
};

QT_END_NAMESPACE

#endif // QDNSLOOKUP_P_H
