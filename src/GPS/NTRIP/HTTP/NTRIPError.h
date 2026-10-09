#pragma once

#include <chrono>

#include <QtCore/QMetaType>
#include <QtCore/QString>

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
    /// HTTP 400 or 403: the caster refuses the request as sent, so repeating it cannot succeed.
    RequestRejected,
    HeaderTooLarge,
    InvalidMountpoint,
    InvalidConfig,
    Unknown
};
Q_ENUM_NS(NTRIPError)

}  // namespace NTRIPErrors

using NTRIPError = NTRIPErrors::NTRIPError;

struct [[nodiscard]] NTRIPFailure
{
    /// Longest caster Retry-After hint that may delay a reconnect.
    static constexpr std::chrono::milliseconds MAX_RETRY_AFTER{std::chrono::minutes(5)};

    NTRIPError code = NTRIPError::Unknown;
    QString detail;
    std::chrono::milliseconds retryAfter{0};
};
