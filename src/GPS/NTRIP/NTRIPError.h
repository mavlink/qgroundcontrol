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
    std::chrono::milliseconds retryAfter{0};
};
Q_DECLARE_METATYPE(NTRIPFailure)
