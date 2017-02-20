/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppSettings.h"

#include <QQmlEngine>
#include <QtQml>

const char* AppSettings::appSettingsGroupName =                         "app";
const char* AppSettings::offlineEditingFirmwareTypeSettingsName =       "OfflineEditingFirmwareType";
const char* AppSettings::offlineEditingVehicleTypeSettingsName =        "OfflineEditingVehicleType";
const char* AppSettings::offlineEditingCruiseSpeedSettingsName =        "OfflineEditingCruiseSpeed";
const char* AppSettings::offlineEditingHoverSpeedSettingsName =         "OfflineEditingHoverSpeed";
const char* AppSettings::batteryPercentRemainingAnnounceSettingsName =  "batteryPercentRemainingAnnounce";
const char* AppSettings::defaultMissionItemAltitudeSettingsName =       "DefaultMissionItemAltitude";
const char* AppSettings::missionAutoLoadDirSettingsName =               "MissionAutoLoadDir";
const char* AppSettings::promptFlightTelemetrySaveName =                "PromptFLightDataSave";
const char* AppSettings::promptFlightTelemetrySaveNotArmedName =        "PromptFLightDataSaveNotArmed";
const char* AppSettings::audioMutedName =                               "AudioMuted";

AppSettings::AppSettings(QObject* parent)
    : SettingsGroup(appSettingsGroupName, QString() /* root settings group */, parent)
    , _offlineEditingFirmwareTypeFact(NULL)
    , _offlineEditingVehicleTypeFact(NULL)
    , _offlineEditingCruiseSpeedFact(NULL)
    , _offlineEditingHoverSpeedFact(NULL)
    , _batteryPercentRemainingAnnounceFact(NULL)
    , _defaultMissionItemAltitudeFact(NULL)
    , _missionAutoLoadDirFact(NULL)
    , _promptFlightTelemetrySave(NULL)
    , _promptFlightTelemetrySaveNotArmed(NULL)
    , _audioMuted(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AppSettings>("QGroundControl.SettingsManager", 1, 0, "AppSettings", "Reference only");
}

Fact* AppSettings::offlineEditingFirmwareType(void)
{
    if (!_offlineEditingFirmwareTypeFact) {
        _offlineEditingFirmwareTypeFact = _createSettingsFact(offlineEditingFirmwareTypeSettingsName);
    }

    return _offlineEditingFirmwareTypeFact;
}

Fact* AppSettings::offlineEditingVehicleType(void)
{
    if (!_offlineEditingVehicleTypeFact) {
        _offlineEditingVehicleTypeFact = _createSettingsFact(offlineEditingVehicleTypeSettingsName);
    }

    return _offlineEditingVehicleTypeFact;
}

Fact* AppSettings::offlineEditingCruiseSpeed(void)
{
    if (!_offlineEditingCruiseSpeedFact) {
        _offlineEditingCruiseSpeedFact = _createSettingsFact(offlineEditingCruiseSpeedSettingsName);
    }
    return _offlineEditingCruiseSpeedFact;
}

Fact* AppSettings::offlineEditingHoverSpeed(void)
{
    if (!_offlineEditingHoverSpeedFact) {
        _offlineEditingHoverSpeedFact = _createSettingsFact(offlineEditingHoverSpeedSettingsName);
    }
    return _offlineEditingHoverSpeedFact;
}

Fact* AppSettings::batteryPercentRemainingAnnounce(void)
{
    if (!_batteryPercentRemainingAnnounceFact) {
        _batteryPercentRemainingAnnounceFact = _createSettingsFact(batteryPercentRemainingAnnounceSettingsName);
    }

    return _batteryPercentRemainingAnnounceFact;
}

Fact* AppSettings::defaultMissionItemAltitude(void)
{
    if (!_defaultMissionItemAltitudeFact) {
        _defaultMissionItemAltitudeFact = _createSettingsFact(defaultMissionItemAltitudeSettingsName);
    }

    return _defaultMissionItemAltitudeFact;
}

Fact* AppSettings::missionAutoLoadDir(void)
{
    if (!_missionAutoLoadDirFact) {
        _missionAutoLoadDirFact = _createSettingsFact(missionAutoLoadDirSettingsName);
    }

    return _missionAutoLoadDirFact;
}

Fact* AppSettings::promptFlightTelemetrySave(void)
{
    if (!_promptFlightTelemetrySave) {
        _promptFlightTelemetrySave = _createSettingsFact(promptFlightTelemetrySaveName);
    }

    return _promptFlightTelemetrySave;
}

Fact* AppSettings::promptFlightTelemetrySaveNotArmed(void)
{
    if (!_promptFlightTelemetrySaveNotArmed) {
        _promptFlightTelemetrySaveNotArmed = _createSettingsFact(promptFlightTelemetrySaveNotArmedName);
    }

    return _promptFlightTelemetrySaveNotArmed;
}

Fact* AppSettings::audioMuted(void)
{
    if (!_audioMuted) {
        _audioMuted = _createSettingsFact(audioMutedName);
    }

    return _audioMuted;
}
