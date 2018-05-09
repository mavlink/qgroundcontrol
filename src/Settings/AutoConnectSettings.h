/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AutoConnectSettings_H
#define AutoConnectSettings_H

#include "SettingsGroup.h"

class AutoConnectSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    AutoConnectSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* autoConnectUDP         READ autoConnectUDP         CONSTANT)
    Q_PROPERTY(Fact* autoConnectPixhawk     READ autoConnectPixhawk     CONSTANT)
    Q_PROPERTY(Fact* autoConnectSiKRadio    READ autoConnectSiKRadio    CONSTANT)
    Q_PROPERTY(Fact* autoConnectPX4Flow     READ autoConnectPX4Flow     CONSTANT)
    Q_PROPERTY(Fact* autoConnectRTKGPS      READ autoConnectRTKGPS      CONSTANT)
    Q_PROPERTY(Fact* autoConnectLibrePilot  READ autoConnectLibrePilot  CONSTANT)
    Q_PROPERTY(Fact* autoConnectNmeaPort    READ autoConnectNmeaPort    CONSTANT)
    Q_PROPERTY(Fact* autoConnectNmeaBaud    READ autoConnectNmeaBaud    CONSTANT)
    Q_PROPERTY(Fact* udpListenPort          READ udpListenPort          CONSTANT)   ///< Port to listen on for UDP autoconnect
    Q_PROPERTY(Fact* udpTargetHostIP        READ udpTargetHostIP        CONSTANT)   ///< Target host IP for UDP autoconnect, empty string for none
    Q_PROPERTY(Fact* udpTargetHostPort      READ udpTargetHostPort      CONSTANT)   ///< Target host post for UDP autoconnect

    Fact* autoConnectUDP        (void);
    Fact* autoConnectPixhawk    (void);
    Fact* autoConnectSiKRadio   (void);
    Fact* autoConnectPX4Flow    (void);
    Fact* autoConnectRTKGPS     (void);
    Fact* autoConnectLibrePilot (void);
    Fact* autoConnectNmeaPort   (void);
    Fact* autoConnectNmeaBaud   (void);
    Fact* udpListenPort         (void);
    Fact* udpTargetHostIP       (void);
    Fact* udpTargetHostPort     (void);

    static const char* autoConnectSettingsGroupName;

    static const char* autoConnectUDPSettingsName;
    static const char* autoConnectPixhawkSettingsName;
    static const char* autoConnectSiKRadioSettingsName;
    static const char* autoConnectPX4FlowSettingsName;
    static const char* autoConnectRTKGPSSettingsName;
    static const char* autoConnectLibrePilotSettingsName;
    static const char* autoConnectNmeaPortName;
    static const char* autoConnectNmeaBaudName;
    static const char* udpListenPortName;
    static const char* udpTargetHostIPName;
    static const char* udpTargetHostPortName;

private:
    SettingsFact* _autoConnectUDPFact;
    SettingsFact* _autoConnectPixhawkFact;
    SettingsFact* _autoConnectSiKRadioFact;
    SettingsFact* _autoConnectPX4FlowFact;
    SettingsFact* _autoConnectRTKGPSFact;
    SettingsFact* _autoConnectLibrePilotFact;
    SettingsFact* _autoConnectNmeaPortFact;
    SettingsFact* _autoConnectNmeaBaudFact;
    SettingsFact* _udpListenPortFact;
    SettingsFact* _udpTargetHostIPFact;
    SettingsFact* _udpTargetHostPortFact;

    static const char* _settingsGroup;
};

#endif
