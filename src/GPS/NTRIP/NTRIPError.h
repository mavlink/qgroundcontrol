#pragma once

#include <QtCore/QMetaType>

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
Q_DECLARE_METATYPE(NTRIPError)
