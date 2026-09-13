#pragma once

#include <QtCore/QMetaType>

enum class GPSConnectionError
{
    None,
    OpenFailed,
    ConfigFailed,
    DeviceError,
};
Q_DECLARE_METATYPE(GPSConnectionError)

/// Terminal intent errors wait for a new request or changed receiver; transient failures back off.
enum class GPSRetryDisposition
{
    Retry,
    AwaitChange,
    Cancel,
};
