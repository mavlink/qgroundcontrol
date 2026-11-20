#pragma once

#include <chrono>

#include <QtCore/QMetaType>
#include <QtCore/QString>

namespace NTRIPErrors {
Q_NAMESPACE

enum class NTRIPError
{
    ConnectionTimeout,
    /// Nothing arrived for the stall timeout after the stream was due to deliver.
    DataWatchdog,
    /// Data arrived for the stall timeout without a valid RTCM 3 message: before any corrections, a mountpoint that
    /// sends another format.
    InvalidStream,
    AuthFailed,
    SocketError,
    SslError,
    ServerDisconnected,
    InvalidHttpResponse,
    HttpError,
    /// HTTP 3xx, 400 or 403: the caster does not serve the request as sent, so repeating it cannot succeed.
    RequestRejected,
    HeaderTooLarge,
    /// The caster does not have the mountpoint: it answered with its source table, or with HTTP 404 or 410.
    InvalidMountpoint,
    InvalidConfig,
    Unknown,
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
