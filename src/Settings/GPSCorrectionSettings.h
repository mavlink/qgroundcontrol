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

    explicit GPSCorrectionSettings(QObject* parent = nullptr);
    ~GPSCorrectionSettings() override;

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(rtcmUdpInputEnabled)
    DEFINE_SETTINGFACT(rtcmUdpInputPort)
    DEFINE_SETTINGFACT(rtcmUdpOutputEnabled)
    DEFINE_SETTINGFACT(rtcmUdpOutputAddress)
    DEFINE_SETTINGFACT(rtcmUdpOutputPort)
    DEFINE_SETTINGFACT(correctionSource)
};
