#pragma once

#include <QtCore/QString>
#include <QtCore/QtTypes>

/// Decodes the device IDs PX4 stores in the CAL_*_ID parameters, using the layout and device types from the
/// firmware's lib/drivers/device/Device.hpp and drivers/drv_sensor.h
class PX4SensorDevice
{
public:
    /// Sensor chip name, or an empty string when the device type is not known
    static QString name(quint32 deviceId);

    /// Bus the sensor is on ("SPI", "I2C", "DroneCAN" ...), or an empty string when unknown
    static QString busName(quint32 deviceId);

    /// Chip name and bus for display, such as "ICM42688P · SPI". Empty for the unset device ID 0.
    static QString description(quint32 deviceId);

private:
    PX4SensorDevice() = delete;
};
