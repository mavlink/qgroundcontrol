/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>

const char* AppSettings::appSettingsGroupName =                         "App";
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
const char* AppSettings::virtualJoystickName =                          "VirtualTabletJoystick";
const char* AppSettings::appFontPointSizeName =                         "BaseDeviceFontPointSize";
const char* AppSettings::indoorPaletteName =                            "StyleIsDark";
const char* AppSettings::showLargeCompassName =                         "ShowLargeCompass";

AppSettings::AppSettings(QObject* parent)
    : SettingsGroup(appSettingsGroupName, QString() /* root settings group */, parent)
    , _offlineEditingFirmwareTypeFact(NULL)
    , _offlineEditingVehicleTypeFact(NULL)
    , _offlineEditingCruiseSpeedFact(NULL)
    , _offlineEditingHoverSpeedFact(NULL)
    , _batteryPercentRemainingAnnounceFact(NULL)
    , _defaultMissionItemAltitudeFact(NULL)
    , _missionAutoLoadDirFact(NULL)
    , _promptFlightTelemetrySaveFact(NULL)
    , _promptFlightTelemetrySaveNotArmedFact(NULL)
    , _audioMutedFact(NULL)
    , _virtualJoystickFact(NULL)
    , _appFontPointSizeFact(NULL)
    , _indoorPaletteFact(NULL)
    , _showLargeCompassFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AppSettings>("QGroundControl.SettingsManager", 1, 0, "AppSettings", "Reference only");        
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);
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
    if (!_promptFlightTelemetrySaveFact) {
        _promptFlightTelemetrySaveFact = _createSettingsFact(promptFlightTelemetrySaveName);
    }

    return _promptFlightTelemetrySaveFact;
}

Fact* AppSettings::promptFlightTelemetrySaveNotArmed(void)
{
    if (!_promptFlightTelemetrySaveNotArmedFact) {
        _promptFlightTelemetrySaveNotArmedFact = _createSettingsFact(promptFlightTelemetrySaveNotArmedName);
    }

    return _promptFlightTelemetrySaveNotArmedFact;
}

Fact* AppSettings::audioMuted(void)
{
    if (!_audioMutedFact) {
        _audioMutedFact = _createSettingsFact(audioMutedName);
    }

    return _audioMutedFact;
}

Fact* AppSettings::appFontPointSize(void)
{
    if (!_appFontPointSizeFact) {
        _appFontPointSizeFact = _createSettingsFact(appFontPointSizeName);
    }

    return _appFontPointSizeFact;
}

Fact* AppSettings::virtualJoystick(void)
{
    if (!_virtualJoystickFact) {
        _virtualJoystickFact = _createSettingsFact(virtualJoystickName);
    }

    return _virtualJoystickFact;
}

Fact* AppSettings::indoorPalette(void)
{
    if (!_indoorPaletteFact) {
        _indoorPaletteFact = _createSettingsFact(indoorPaletteName);
        connect(_indoorPaletteFact, &Fact::rawValueChanged, this, &AppSettings::_indoorPaletteChanged);
    }

    return _indoorPaletteFact;
}

void AppSettings::_indoorPaletteChanged(void)
{
    qgcApp()->_loadCurrentStyleSheet();
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);
}

Fact* AppSettings::showLargeCompass(void)
{
    if (!_showLargeCompassFact) {
        _showLargeCompassFact = _createSettingsFact(showLargeCompassName);
    }

    return _showLargeCompassFact;
}

