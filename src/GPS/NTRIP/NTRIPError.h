#pragma once

#include <QtCore/QMetaType>

namespace NTRIPErrors {
Q_NAMESPACE

enum class NTRIPError {
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

} // namespace NTRIPErrors

using NTRIPError = NTRIPErrors::NTRIPError;
