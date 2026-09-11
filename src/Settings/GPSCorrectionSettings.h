#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class GPSCorrectionSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum CorrectionSource
    {
        Automatic = 0,
        LocalReceiver = 1,
        Ntrip = 2,
        Udp = 3,
        All = 4,
    };
    Q_ENUM(CorrectionSource)

    explicit GPSCorrectionSettings(QObject* parent = nullptr);
    ~GPSCorrectionSettings() override;

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(rtcmUdpInputEnabled)
    DEFINE_SETTINGFACT(rtcmUdpInputPort)
    DEFINE_SETTINGFACT(rtcmUdpValidate)
    DEFINE_SETTINGFACT(correctionSource)
    DEFINE_SETTINGFACT(correctionSourceInstance)
    DEFINE_SETTINGFACT(injectLocalReceiver)
};
