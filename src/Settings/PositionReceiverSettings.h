#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// The position receiver: a second GNSS receiver that is never configured and only provides the ground station
/// position, beside the receiver of RTKSettings. It has a connection and nothing else; connectionType holds an
/// RTKSettings::ConnectionType, and each connection setting means what the RTKSettings one of its name does.
class PositionReceiverSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit PositionReceiverSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    /// The position receiver is in use: it is kept connected, and it alone is the receiver that provides the ground
    /// station position.
    DEFINE_SETTINGFACT(enabled)
    DEFINE_SETTINGFACT(connectionType)
    DEFINE_SETTINGFACT(tcpHost)
    DEFINE_SETTINGFACT(tcpPort)
    DEFINE_SETTINGFACT(udpPort)
    DEFINE_SETTINGFACT(serialDevice)
    DEFINE_SETTINGFACT(serialBaudRate)
    DEFINE_SETTINGFACT(bluetoothDeviceAddress)
    DEFINE_SETTINGFACT(bluetoothDeviceName)
};
