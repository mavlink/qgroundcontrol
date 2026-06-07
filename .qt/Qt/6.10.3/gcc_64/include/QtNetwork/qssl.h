// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSL_H
#define QSSL_H

#if 0
#pragma qt_class(QSsl)
#endif

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/QFlags>

QT_BEGIN_NAMESPACE


namespace QSsl {
    Q_NAMESPACE_EXPORT(Q_NETWORK_EXPORT)

    enum KeyType {
        PrivateKey,
        PublicKey
    };
    Q_ENUM_NS(KeyType)

    enum EncodingFormat {
        Pem,
        Der
    };
    Q_ENUM_NS(EncodingFormat)

    enum KeyAlgorithm {
        Opaque,
        Rsa,
        Dsa,
        Ec,
        Dh,
    };
    Q_ENUM_NS(KeyAlgorithm)

    enum AlternativeNameEntryType {
        EmailEntry,
        DnsEntry,
        IpAddressEntry
    };
    Q_ENUM_NS(AlternativeNameEntryType)

    enum SslProtocol {
        TlsV1_0 QT_DEPRECATED_VERSION_X_6_3("Use TlsV1_2OrLater instead."),
        TlsV1_1 QT_DEPRECATED_VERSION_X_6_3("Use TlsV1_2OrLater instead."),
        TlsV1_2,
        AnyProtocol,
        SecureProtocols,

        TlsV1_0OrLater QT_DEPRECATED_VERSION_X_6_3("Use TlsV1_2OrLater instead."),
        TlsV1_1OrLater QT_DEPRECATED_VERSION_X_6_3("Use TlsV1_2OrLater instead."),
        TlsV1_2OrLater,

        DtlsV1_0 QT_DEPRECATED_VERSION_X_6_3("Use DtlsV1_2OrLater instead."),
        DtlsV1_0OrLater QT_DEPRECATED_VERSION_X_6_3("Use DtlsV1_2OrLater instead."),
        DtlsV1_2,
        DtlsV1_2OrLater,

        TlsV1_3,
        TlsV1_3OrLater,

        UnknownProtocol = -1
    };
    Q_ENUM_NS(SslProtocol)

    enum SslOption {
        SslOptionDisableEmptyFragments = 0x01,
        SslOptionDisableSessionTickets = 0x02,
        SslOptionDisableCompression = 0x04,
        SslOptionDisableServerNameIndication = 0x08,
        SslOptionDisableLegacyRenegotiation = 0x10,
        SslOptionDisableSessionSharing = 0x20,
        SslOptionDisableSessionPersistence = 0x40,
        SslOptionDisableServerCipherPreference = 0x80
    };
    Q_ENUM_NS(SslOption)
    Q_DECLARE_FLAGS(SslOptions, SslOption)

    enum class AlertLevel {
        Warning,
        Fatal,
        Unknown
    };
    Q_ENUM_NS(AlertLevel)

    enum class AlertType {
        CloseNotify,
        UnexpectedMessage = 10,
        BadRecordMac = 20,
        RecordOverflow = 22,
        DecompressionFailure = 30, // reserved
        HandshakeFailure = 40,
        NoCertificate = 41, // reserved
        BadCertificate = 42,
        UnsupportedCertificate = 43,
        CertificateRevoked = 44,
        CertificateExpired = 45,
        CertificateUnknown = 46,
        IllegalParameter = 47,
        UnknownCa = 48,
        AccessDenied = 49,
        DecodeError = 50,
        DecryptError = 51,
        ExportRestriction = 60, // reserved
        ProtocolVersion = 70,
        InsufficientSecurity = 71,
        InternalError = 80,
        InappropriateFallback = 86,
        UserCancelled = 90,
        NoRenegotiation = 100,
        MissingExtension = 109,
        UnsupportedExtension = 110,
        CertificateUnobtainable = 111, // reserved
        UnrecognizedName = 112,
        BadCertificateStatusResponse = 113,
        BadCertificateHashValue = 114, // reserved
        UnknownPskIdentity = 115,
        CertificateRequired = 116,
        NoApplicationProtocol = 120,
        UnknownAlertMessage = 255
    };
    Q_ENUM_NS(AlertType)

    enum class ImplementedClass
    {
        Key,
        Certificate,
        Socket,
        DiffieHellman,
        EllipticCurve,
        Dtls,
        DtlsCookie
    };
    Q_ENUM_NS(ImplementedClass)

    enum class SupportedFeature
    {
        CertificateVerification,
        ClientSideAlpn,
        ServerSideAlpn,
        Ocsp,
        Psk,
        SessionTicket,
        Alerts
    };
    Q_ENUM_NS(SupportedFeature)
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QSsl::SslOptions)

QT_END_NAMESPACE

#endif // QSSL_H
