#pragma once

#include <QtCore/QMetaType>

enum class GPSConnectionError
{
    None = 0,
    OpenFailed = 1,
    ConfigFailed = 2,
    DeviceError = 3,
};
Q_DECLARE_METATYPE(GPSConnectionError)
