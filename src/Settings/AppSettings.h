/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AppSettings_H
#define AppSettings_H

#include "SettingsGroup.h"

class AppSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    AppSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* offlineEditingFirmwareType         READ offlineEditingFirmwareType         CONSTANT)
    Q_PROPERTY(Fact* offlineEditingVehicleType          READ offlineEditingVehicleType          CONSTANT)
    Q_PROPERTY(Fact* offlineEditingCruiseSpeed          READ offlineEditingCruiseSpeed          CONSTANT)
    Q_PROPERTY(Fact* offlineEditingHoverSpeed           READ offlineEditingHoverSpeed           CONSTANT)
    Q_PROPERTY(Fact* batteryPercentRemainingAnnounce    READ batteryPercentRemainingAnnounce    CONSTANT)
    Q_PROPERTY(Fact* defaultMissionItemAltitude         READ defaultMissionItemAltitude         CONSTANT)
    Q_PROPERTY(Fact* missionAutoLoadDir                 READ missionAutoLoadDir                 CONSTANT)
    Q_PROPERTY(Fact* promptFlightTelemetrySave          READ promptFlightTelemetrySave          CONSTANT)
    Q_PROPERTY(Fact* promptFlightTelemetrySaveNotArmed  READ promptFlightTelemetrySaveNotArmed  CONSTANT)
    Q_PROPERTY(Fact* audioMuted                         READ audioMuted                         CONSTANT)
    Q_PROPERTY(Fact* virtualJoystick                    READ virtualJoystick                    CONSTANT)
    Q_PROPERTY(Fact* appFontPointSize                   READ appFontPointSize                   CONSTANT)
    Q_PROPERTY(Fact* indoorPalette                      READ indoorPalette                      CONSTANT)

    Fact* offlineEditingFirmwareType        (void);
    Fact* offlineEditingVehicleType         (void);
    Fact* offlineEditingCruiseSpeed         (void);
    Fact* offlineEditingHoverSpeed          (void);
    Fact* batteryPercentRemainingAnnounce   (void);
    Fact* defaultMissionItemAltitude        (void);
    Fact* missionAutoLoadDir                (void);
    Fact* promptFlightTelemetrySave         (void);
    Fact* promptFlightTelemetrySaveNotArmed (void);
    Fact* audioMuted                        (void);
    Fact* virtualJoystick                   (void);
    Fact* appFontPointSize                  (void);
    Fact* indoorPalette                     (void);

    static const char* appSettingsGroupName;

    static const char* offlineEditingFirmwareTypeSettingsName;
    static const char* offlineEditingVehicleTypeSettingsName;
    static const char* offlineEditingCruiseSpeedSettingsName;
    static const char* offlineEditingHoverSpeedSettingsName;
    static const char* batteryPercentRemainingAnnounceSettingsName;
    static const char* defaultMissionItemAltitudeSettingsName;
    static const char* missionAutoLoadDirSettingsName;
    static const char* promptFlightTelemetrySaveName;
    static const char* promptFlightTelemetrySaveNotArmedName;
    static const char* audioMutedName;
    static const char* virtualJoystickName;
    static const char* appFontPointSizeName;
    static const char* indoorPaletteName;

private slots:
    void _indoorPaletteChanged(void);

private:
    SettingsFact* _offlineEditingFirmwareTypeFact;
    SettingsFact* _offlineEditingVehicleTypeFact;
    SettingsFact* _offlineEditingCruiseSpeedFact;
    SettingsFact* _offlineEditingHoverSpeedFact;
    SettingsFact* _batteryPercentRemainingAnnounceFact;
    SettingsFact* _defaultMissionItemAltitudeFact;
    SettingsFact* _missionAutoLoadDirFact;
    SettingsFact* _promptFlightTelemetrySaveFact;
    SettingsFact* _promptFlightTelemetrySaveNotArmedFact;
    SettingsFact* _audioMutedFact;
    SettingsFact* _virtualJoystickFact;
    SettingsFact* _appFontPointSizeFact;
    SettingsFact* _indoorPaletteFact;
};

#endif
