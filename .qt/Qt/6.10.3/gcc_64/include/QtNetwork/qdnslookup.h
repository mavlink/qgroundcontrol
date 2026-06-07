// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDNSLOOKUP_H
#define QDNSLOOKUP_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qproperty.h>

QT_REQUIRE_CONFIG(dnslookup);

QT_BEGIN_NAMESPACE

class QHostAddress;
class QDnsLookupPrivate;
class QDnsDomainNameRecordPrivate;
class QDnsHostAddressRecordPrivate;
class QDnsMailExchangeRecordPrivate;
class QDnsServiceRecordPrivate;
class QDnsTextRecordPrivate;
class QDnsTlsAssociationRecordPrivate;
class QSslConfiguration;

QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QDnsTlsAssociationRecordPrivate)

class Q_NETWORK_EXPORT QDnsDomainNameRecord
{
public:
    QDnsDomainNameRecord();
    QDnsDomainNameRecord(const QDnsDomainNameRecord &other);
    QDnsDomainNameRecord &operator=(QDnsDomainNameRecord &&other) noexcept { swap(other); return *this; }
    QDnsDomainNameRecord &operator=(const QDnsDomainNameRecord &other);
    ~QDnsDomainNameRecord();

    void swap(QDnsDomainNameRecord &other) noexcept { d.swap(other.d); }

    QString name() const;
    quint32 timeToLive() const;
    QString value() const;

private:
    QSharedDataPointer<QDnsDomainNameRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsDomainNameRecord)

class Q_NETWORK_EXPORT QDnsHostAddressRecord
{
public:
    QDnsHostAddressRecord();
    QDnsHostAddressRecord(const QDnsHostAddressRecord &other);
    QDnsHostAddressRecord &operator=(QDnsHostAddressRecord &&other) noexcept { swap(other); return *this; }
    QDnsHostAddressRecord &operator=(const QDnsHostAddressRecord &other);
    ~QDnsHostAddressRecord();

    void swap(QDnsHostAddressRecord &other) noexcept { d.swap(other.d); }

    QString name() const;
    quint32 timeToLive() const;
    QHostAddress value() const;

private:
    QSharedDataPointer<QDnsHostAddressRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsHostAddressRecord)

class Q_NETWORK_EXPORT QDnsMailExchangeRecord
{
public:
    QDnsMailExchangeRecord();
    QDnsMailExchangeRecord(const QDnsMailExchangeRecord &other);
    QDnsMailExchangeRecord &operator=(QDnsMailExchangeRecord &&other) noexcept { swap(other); return *this; }
    QDnsMailExchangeRecord &operator=(const QDnsMailExchangeRecord &other);
    ~QDnsMailExchangeRecord();

    void swap(QDnsMailExchangeRecord &other) noexcept { d.swap(other.d); }

    QString exchange() const;
    QString name() const;
    quint16 preference() const;
    quint32 timeToLive() const;

private:
    QSharedDataPointer<QDnsMailExchangeRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsMailExchangeRecord)

class Q_NETWORK_EXPORT QDnsServiceRecord
{
public:
    QDnsServiceRecord();
    QDnsServiceRecord(const QDnsServiceRecord &other);
    QDnsServiceRecord &operator=(QDnsServiceRecord &&other) noexcept { swap(other); return *this; }
    QDnsServiceRecord &operator=(const QDnsServiceRecord &other);
    ~QDnsServiceRecord();

    void swap(QDnsServiceRecord &other) noexcept { d.swap(other.d); }

    QString name() const;
    quint16 port() const;
    quint16 priority() const;
    QString target() const;
    quint32 timeToLive() const;
    quint16 weight() const;

private:
    QSharedDataPointer<QDnsServiceRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsServiceRecord)

class Q_NETWORK_EXPORT QDnsTextRecord
{
public:
    QDnsTextRecord();
    QDnsTextRecord(const QDnsTextRecord &other);
    QDnsTextRecord &operator=(QDnsTextRecord &&other) noexcept { swap(other); return *this; }
    QDnsTextRecord &operator=(const QDnsTextRecord &other);
    ~QDnsTextRecord();

    void swap(QDnsTextRecord &other) noexcept { d.swap(other.d); }

    QString name() const;
    quint32 timeToLive() const;
    QList<QByteArray> values() const;

private:
    QSharedDataPointer<QDnsTextRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsTextRecord)

class QDnsTlsAssociationRecord
{
    Q_GADGET_EXPORT(Q_NETWORK_EXPORT)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
public:
    enum class CertificateUsage : quint8 {
        // https://www.iana.org/assignments/dane-parameters/dane-parameters.xhtml#certificate-usages
        // RFC 6698
        CertificateAuthorityConstrait = 0,
        ServiceCertificateConstraint = 1,
        TrustAnchorAssertion = 2,
        DomainIssuedCertificate = 3,
        PrivateUse = 255,

        // Aliases by RFC 7218
        PKIX_TA = 0,
        PKIX_EE = 1,
        DANE_TA = 2,
        DANE_EE = 3,
        PrivCert = 255,
    };
    Q_ENUM(CertificateUsage)

    enum class Selector : quint8 {
        // https://www.iana.org/assignments/dane-parameters/dane-parameters.xhtml#selectors
        // RFC 6698
        FullCertificate = 0,
        SubjectPublicKeyInfo = 1,
        PrivateUse = 255,

        // Aliases by RFC 7218
        Cert = FullCertificate,
        SPKI = SubjectPublicKeyInfo,
        PrivSel = PrivateUse,
    };
    Q_ENUM(Selector)

    enum class MatchingType : quint8 {
        // https://www.iana.org/assignments/dane-parameters/dane-parameters.xhtml#matching-types
        // RFC 6698
        Exact = 0,
        Sha256 = 1,
        Sha512 = 2,
        PrivateUse = 255,
        PrivMatch = PrivateUse,
    };
    Q_ENUM(MatchingType)

    Q_NETWORK_EXPORT QDnsTlsAssociationRecord();
    Q_NETWORK_EXPORT QDnsTlsAssociationRecord(const QDnsTlsAssociationRecord &other);
    QDnsTlsAssociationRecord(QDnsTlsAssociationRecord &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QDnsTlsAssociationRecord)
    Q_NETWORK_EXPORT QDnsTlsAssociationRecord &operator=(const QDnsTlsAssociationRecord &other);
    Q_NETWORK_EXPORT ~QDnsTlsAssociationRecord();

    void swap(QDnsTlsAssociationRecord &other) noexcept { d.swap(other.d); }

    Q_NETWORK_EXPORT QString name() const;
    Q_NETWORK_EXPORT quint32 timeToLive() const;
    Q_NETWORK_EXPORT CertificateUsage usage() const;
    Q_NETWORK_EXPORT Selector selector() const;
    Q_NETWORK_EXPORT MatchingType matchType() const;
    Q_NETWORK_EXPORT QByteArray value() const;

private:
    QExplicitlySharedDataPointer<QDnsTlsAssociationRecordPrivate> d;
    friend class QDnsLookupRunnable;
};

Q_DECLARE_SHARED(QDnsTlsAssociationRecord)

class Q_NETWORK_EXPORT QDnsLookup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Error error READ error NOTIFY finished)
    Q_PROPERTY(bool authenticData READ isAuthenticData NOTIFY finished)
    Q_PROPERTY(QString errorString READ errorString NOTIFY finished)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged BINDABLE bindableName)
    Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged BINDABLE bindableType)
    Q_PROPERTY(QHostAddress nameserver READ nameserver WRITE setNameserver NOTIFY nameserverChanged
               BINDABLE bindableNameserver)
    Q_PROPERTY(quint16 nameserverPort READ nameserverPort WRITE setNameserverPort
               NOTIFY nameserverPortChanged BINDABLE bindableNameserverPort)
    Q_PROPERTY(Protocol nameserverProtocol READ nameserverProtocol WRITE setNameserverProtocol
               NOTIFY nameserverProtocolChanged BINDABLE bindableNameserverProtocol)

public:
    enum Error
    {
        NoError = 0,
        ResolverError,
        OperationCancelledError,
        InvalidRequestError,
        InvalidReplyError,
        ServerFailureError,
        ServerRefusedError,
        NotFoundError,
        TimeoutError,
    };
    Q_ENUM(Error)

    enum Type
    {
        A = 1,
        AAAA = 28,
        ANY = 255,
        CNAME = 5,
        MX = 15,
        NS = 2,
        PTR = 12,
        SRV = 33,
        TLSA = 52,
        TXT = 16
    };
    Q_ENUM(Type)

    enum Protocol : quint8 {
        Standard = 0,
        DnsOverTls,
    };
    Q_ENUM(Protocol)

    explicit QDnsLookup(QObject *parent = nullptr);
    QDnsLookup(Type type, const QString &name, QObject *parent = nullptr);
    QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, QObject *parent = nullptr);
    QDnsLookup(Type type, const QString &name, const QHostAddress &nameserver, quint16 port,
               QObject *parent = nullptr);
    QDnsLookup(Type type, const QString &name, Protocol protocol, const QHostAddress &nameserver,
               quint16 port = 0, QObject *parent = nullptr);
    ~QDnsLookup();

    bool isAuthenticData() const;
    Error error() const;
    QString errorString() const;
    bool isFinished() const;

    QString name() const;
    void setName(const QString &name);
    QBindable<QString> bindableName();

    Type type() const;
    void setType(QDnsLookup::Type);
    QBindable<Type> bindableType();

    QHostAddress nameserver() const;
    void setNameserver(const QHostAddress &nameserver);
    QBindable<QHostAddress> bindableNameserver();
    quint16 nameserverPort() const;
    void setNameserverPort(quint16 port);
    QBindable<quint16> bindableNameserverPort();
    Protocol nameserverProtocol() const;
    void setNameserverProtocol(Protocol protocol);
    QBindable<Protocol> bindableNameserverProtocol();
    void setNameserver(Protocol protocol, const QHostAddress &nameserver, quint16 port = 0);
    QT_NETWORK_INLINE_SINCE(6, 8)
    void setNameserver(const QHostAddress &nameserver, quint16 port);

    QList<QDnsDomainNameRecord> canonicalNameRecords() const;
    QList<QDnsHostAddressRecord> hostAddressRecords() const;
    QList<QDnsMailExchangeRecord> mailExchangeRecords() const;
    QList<QDnsDomainNameRecord> nameServerRecords() const;
    QList<QDnsDomainNameRecord> pointerRecords() const;
    QList<QDnsServiceRecord> serviceRecords() const;
    QList<QDnsTextRecord> textRecords() const;
    QList<QDnsTlsAssociationRecord> tlsAssociationRecords() const;

#if QT_CONFIG(ssl)
    void setSslConfiguration(const QSslConfiguration &sslConfiguration);
    QSslConfiguration sslConfiguration() const;
#endif

    static bool isProtocolSupported(Protocol protocol);
    static quint16 defaultPortForProtocol(Protocol protocol) noexcept Q_DECL_CONST_FUNCTION;

public Q_SLOTS:
    void abort();
    void lookup();

Q_SIGNALS:
    void finished();
    void nameChanged(const QString &name);
    void typeChanged(QDnsLookup::Type type);
    void nameserverChanged(const QHostAddress &nameserver);
    void nameserverPortChanged(quint16 port);
    void nameserverProtocolChanged(QDnsLookup::Protocol protocol);

private:
    Q_DECLARE_PRIVATE(QDnsLookup)
};

#if QT_NETWORK_INLINE_IMPL_SINCE(6, 8)
void QDnsLookup::setNameserver(const QHostAddress &nameserver, quint16 port)
{
    setNameserver(Standard, nameserver, port);
}
#endif

QT_END_NAMESPACE

#endif // QDNSLOOKUP_H
