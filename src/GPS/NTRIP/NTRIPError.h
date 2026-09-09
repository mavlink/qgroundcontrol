#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>

#include <chrono>

namespace NTRIPErrors {
Q_NAMESPACE

enum class NTRIPError
{
    ConnectionTimeout,
    DataWatchdog,
    AuthFailed,
    SocketError,
    SslError,
    ServerDisconnected,
    InvalidHttpResponse,
    HttpError,
    HeaderTooLarge,
    InvalidMountpoint,
    NoLocation,
    InvalidConfig,
    Unknown
};
Q_ENUM_NS(NTRIPError)

}  // namespace NTRIPErrors

using NTRIPError = NTRIPErrors::NTRIPError;

struct NTRIPFailure
{
    NTRIPError code = NTRIPError::Unknown;
    QString detail;
    bool retryable = true;
    int httpStatus = 0;
    std::chrono::milliseconds retryAfter{0};

    static NTRIPFailure fromError(NTRIPError code, const QString& detail)
    {
        const bool permanent = code == NTRIPError::AuthFailed || code == NTRIPError::InvalidConfig ||
                               code == NTRIPError::InvalidMountpoint || code == NTRIPError::SslError ||
                               code == NTRIPError::InvalidHttpResponse || code == NTRIPError::HeaderTooLarge;
        return {code, detail, !permanent};
    }
};
Q_DECLARE_METATYPE(NTRIPFailure)
