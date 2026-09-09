#pragma once

#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslError>

namespace NTRIPTlsPolicy {
/// The opt-in permits self-signed trust errors only; expiry and identity errors remain fatal.
inline bool canIgnore(const QList<QSslError>& errors, bool allowSelfSigned)
{
    if (!allowSelfSigned || errors.isEmpty()) {
        return false;
    }
    for (const auto& error : errors) {
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
