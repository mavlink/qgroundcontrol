#pragma once

#include <QtCore/QList>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslError>

namespace NTRIPTlsPolicy {
/// Callers must also require opt-in and ignore only the supplied error objects.
inline bool isSelfSignedOnly(const QList<QSslError>& errors)
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
