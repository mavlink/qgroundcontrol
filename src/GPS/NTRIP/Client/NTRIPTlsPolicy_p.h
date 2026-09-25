#pragma once

#include <QtCore/QCryptographicHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslError>

namespace NTRIPTlsPolicy {
/// Binds the certificate's SHA-256 digest to one caster endpoint ("host:port").
[[nodiscard]] inline QString certificatePin(const QString& endpoint, const QSslCertificate& certificate)
{
    return endpoint + QLatin1Char('|') + QString::fromLatin1(certificate.digest(QCryptographicHash::Sha256).toHex());
}

/// Callers must also require opt-in and ignore only the supplied error objects.
[[nodiscard]] inline bool isSelfSignedOnly(const QList<QSslError>& errors)
{
    if (errors.isEmpty()) {
        return false;
    }
    for (const QSslError& error : errors) {
        switch (error.error()) {
            case QSslError::SelfSignedCertificate:
            case QSslError::SelfSignedCertificateInChain:
                break;
            case QSslError::UnableToGetLocalIssuerCertificate:
            case QSslError::UnableToVerifyFirstCertificate:
            case QSslError::CertificateUntrusted:
                if (error.certificate().isNull() || !error.certificate().isSelfSigned()) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    return true;
}
}  // namespace NTRIPTlsPolicy
