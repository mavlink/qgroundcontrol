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
