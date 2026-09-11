#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// \brief Auto connect settings
///
/// Defines which links should be automatically created and started at runtime
///
class AutoConnectSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    AutoConnectSettings(QObject* parent = nullptr);

    enum NmeaSource {
        NmeaSourceDisabled = 0,
        NmeaSourceUdp,
        NmeaSourceSerial,
        NmeaSourceTcp,
    };
    Q_ENUM(NmeaSource)

    enum NmeaReceiverMode
    {
        NmeaReceiverPassive = 0,
        NmeaReceiverUblox,
    };
    Q_ENUM(NmeaReceiverMode)

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(autoConnectUDP)
    DEFINE_SETTINGFACT(autoConnectPixhawk)
    DEFINE_SETTINGFACT(autoConnectSiKRadio)
    DEFINE_SETTINGFACT(autoConnectRTKGPS)
    DEFINE_SETTINGFACT(autoConnectNetworkRTKGPS)
    DEFINE_SETTINGFACT(autoConnectLibrePilot)
    DEFINE_SETTINGFACT(nmeaSource)
    DEFINE_SETTINGFACT(nmeaAutoConnect)
    DEFINE_SETTINGFACT(nmeaReceiverMode)
    DEFINE_SETTINGFACT(nmeaTcpHost)
    DEFINE_SETTINGFACT(nmeaTcpPort)
    DEFINE_SETTINGFACT(autoConnectNmeaPort)
    DEFINE_SETTINGFACT(autoConnectNmeaBaud)
    DEFINE_SETTINGFACT(udpListenPort)
    DEFINE_SETTINGFACT(udpTargetHostIP)
    DEFINE_SETTINGFACT(udpTargetHostPort)
    DEFINE_SETTINGFACT(nmeaUdpPort)
};
