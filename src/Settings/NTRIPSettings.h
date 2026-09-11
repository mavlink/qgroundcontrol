#pragma once

#include "GPSCorrectionSettings.h"
#include "SettingsGroup.h"

class NTRIPSettings : public SettingsGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* rtcmUdpInputEnabled READ rtcmUdpInputEnabled CONSTANT)
    Q_PROPERTY(Fact* rtcmUdpInputPort READ rtcmUdpInputPort CONSTANT)
    Q_PROPERTY(Fact* rtcmUdpValidate READ rtcmUdpValidate CONSTANT)
    Q_PROPERTY(Fact* correctionSource READ correctionSource CONSTANT)
    Q_PROPERTY(Fact* correctionSourceInstance READ correctionSourceInstance CONSTANT)
    Q_PROPERTY(Fact* injectLocalReceiver READ injectLocalReceiver CONSTANT)

public:
    enum CorrectionSource
    {
        Automatic = GPSCorrectionSettings::Automatic,
        LocalReceiver = GPSCorrectionSettings::LocalReceiver,
        Ntrip = GPSCorrectionSettings::Ntrip,
        Udp = GPSCorrectionSettings::Udp,
        All = GPSCorrectionSettings::All,
    };
    Q_ENUM(CorrectionSource)

    /// The shared correction settings must outlive this compatibility facade.
    explicit NTRIPSettings(GPSCorrectionSettings& correctionSettings, QObject* parent = nullptr);
    ~NTRIPSettings() override;

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(ntripServerConnectEnabled)
    DEFINE_SETTINGFACT(ntripServerHostAddress)
    DEFINE_SETTINGFACT(ntripServerPort)
    DEFINE_SETTINGFACT(ntripUsername)
    DEFINE_SETTINGFACT(ntripPassword)
    DEFINE_SETTINGFACT(ntripMountpoint)
    DEFINE_SETTINGFACT(ntripWhitelist)
    DEFINE_SETTINGFACT(ntripUseTls)
    DEFINE_SETTINGFACT(ntripAllowSelfSignedCerts)
    DEFINE_SETTINGFACT(ntripGgaPositionSource)
    DEFINE_SETTINGFACT(ntripGgaIntervalSec)
    DEFINE_SETTINGFACT(ntripUdpForwardEnabled)
    DEFINE_SETTINGFACT(ntripUdpTargetAddress)
    DEFINE_SETTINGFACT(ntripUdpTargetPort)

    Fact* rtcmUdpInputEnabled() { return _correctionSettings.rtcmUdpInputEnabled(); }

    static const char* rtcmUdpInputEnabledName;

    Fact* rtcmUdpInputPort() { return _correctionSettings.rtcmUdpInputPort(); }

    static const char* rtcmUdpInputPortName;

    Fact* rtcmUdpValidate() { return _correctionSettings.rtcmUdpValidate(); }

    static const char* rtcmUdpValidateName;

    Fact* correctionSource() { return _correctionSettings.correctionSource(); }

    static const char* correctionSourceName;

    Fact* correctionSourceInstance() { return _correctionSettings.correctionSourceInstance(); }

    static const char* correctionSourceInstanceName;

    Fact* injectLocalReceiver() { return _correctionSettings.injectLocalReceiver(); }

    static const char* injectLocalReceiverName;

private:
    GPSCorrectionSettings& _correctionSettings;
};
