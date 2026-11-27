#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class GPSCorrectionSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    /// Persisted values of correctionSource. The storage type keeps any saved value a valid enum value.
    enum CorrectionSource : quint8
    {
        HighestPriority = 0,
        LocalReceiver = 1,
        Ntrip = 2,
        Udp = 3,
    };
    Q_ENUM(CorrectionSource)

    /// Persisted values of correctionVehicles.
    enum CorrectionVehicles : quint8
    {
        AllVehicles = 0,
        ActiveVehicle = 1,
    };

    explicit GPSCorrectionSettings(QObject* parent = nullptr);
    ~GPSCorrectionSettings() override;

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(rtcmUdpInputEnabled)
    DEFINE_SETTINGFACT(rtcmUdpInputPort)
    DEFINE_SETTINGFACT(rtcmUdpOutputEnabled)
    DEFINE_SETTINGFACT(rtcmUdpOutputAddress)
    DEFINE_SETTINGFACT(rtcmUdpOutputPort)
    DEFINE_SETTINGFACT(correctionSource)
    DEFINE_SETTINGFACT(correctionVehicles)
    /// The RTCM message filter of every correction source. It was an NTRIP setting and keeps that name, and so its
    /// stored key, so a saved filter still applies.
    DEFINE_SETTINGFACT(ntripWhitelist)
};
