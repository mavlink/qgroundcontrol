/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsManager.h"
#include "QGCLoggingCategory.h"
#include "ADSBVehicleManagerSettings.h"
#ifndef QGC_NO_ARDUPILOT_DIALECT
#include "APMMavlinkStreamRateSettings.h"
#endif
#include "AppSettings.h"
#include "AutoConnectSettings.h"
#include "BatteryIndicatorSettings.h"
#include "BrandImageSettings.h"
#include "MavlinkActionsSettings.h"
#include "FirmwareUpgradeSettings.h"
#include "FlightMapSettings.h"
#include "FlightModeSettings.h"
#include "FlyViewSettings.h"
#include "GimbalControllerSettings.h"
#include "MapsSettings.h"
#include "OfflineMapsSettings.h"
#include "PlanViewSettings.h"
#include "RemoteIDSettings.h"
#include "RTKSettings.h"
#include "UnitsSettings.h"
#include "VideoSettings.h"
#include "MavlinkSettings.h"
#ifdef QGC_VIEWER3D
#include "Viewer3DSettings.h"
#endif
#include "JsonHelper.h"
#include "QGCCorePlugin.h"
#include "QGCApplication.h"

#include <QtCore/QApplicationStatic>
#include <QtQml/qqml.h>

QGC_LOGGING_CATEGORY(SettingsManagerLog, "Utilities.SettingsManager")

Q_APPLICATION_STATIC(SettingsManager, _settingsManagerInstance);

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(SettingsManagerLog) << this;
}

SettingsManager::~SettingsManager()
{
    qCDebug(SettingsManagerLog) << this;
}

SettingsManager *SettingsManager::instance()
{
    return _settingsManagerInstance();
}

void SettingsManager::init()
{
    _unitsSettings = new UnitsSettings(this); // Must be first since AppSettings references it

    _appSettings = new AppSettings(this);
    _loadSettingsFiles();

    _autoConnectSettings = new AutoConnectSettings(this);
    _batteryIndicatorSettings = new BatteryIndicatorSettings(this);
    _brandImageSettings = new BrandImageSettings(this);
    _mavlinkActionsSettings = new MavlinkActionsSettings(this);
    _firmwareUpgradeSettings = new FirmwareUpgradeSettings(this);
    _flightMapSettings = new FlightMapSettings(this);
    _flightModeSettings = new FlightModeSettings(this);
    _flyViewSettings = new FlyViewSettings(this);
    _gimbalControllerSettings = new GimbalControllerSettings(this);
    _mapsSettings = new MapsSettings(this);
    _offlineMapsSettings = new OfflineMapsSettings(this);
    _planViewSettings = new PlanViewSettings(this);
    _remoteIDSettings = new RemoteIDSettings(this);
    _rtkSettings = new RTKSettings(this);
    _videoSettings = new VideoSettings(this);
    _mavlinkSettings = new MavlinkSettings(this);
#ifdef QGC_VIEWER3D
    _viewer3DSettings = new Viewer3DSettings(this);
#endif
    _adsbVehicleManagerSettings = new ADSBVehicleManagerSettings(this);
#ifndef QGC_NO_ARDUPILOT_DIALECT
    _apmMavlinkStreamRateSettings = new APMMavlinkStreamRateSettings(this);
#endif
}

ADSBVehicleManagerSettings *SettingsManager::adsbVehicleManagerSettings() const { return _adsbVehicleManagerSettings; }
#ifndef QGC_NO_ARDUPILOT_DIALECT
APMMavlinkStreamRateSettings *SettingsManager::apmMavlinkStreamRateSettings() const { return _apmMavlinkStreamRateSettings; }
#endif
AppSettings *SettingsManager::appSettings() const { return _appSettings; }
AutoConnectSettings *SettingsManager::autoConnectSettings() const { return _autoConnectSettings; }
BatteryIndicatorSettings *SettingsManager::batteryIndicatorSettings() const { return _batteryIndicatorSettings; }
BrandImageSettings *SettingsManager::brandImageSettings() const { return _brandImageSettings; }
MavlinkActionsSettings *SettingsManager::mavlinkActionsSettings() const { return _mavlinkActionsSettings; }
FirmwareUpgradeSettings *SettingsManager::firmwareUpgradeSettings() const { return _firmwareUpgradeSettings; }
FlightMapSettings *SettingsManager::flightMapSettings() const { return _flightMapSettings; }
FlightModeSettings *SettingsManager::flightModeSettings() const { return _flightModeSettings; }
FlyViewSettings *SettingsManager::flyViewSettings() const { return _flyViewSettings; }
GimbalControllerSettings *SettingsManager::gimbalControllerSettings() const { return _gimbalControllerSettings; }
MapsSettings *SettingsManager::mapsSettings() const { return _mapsSettings; }
OfflineMapsSettings *SettingsManager::offlineMapsSettings() const { return _offlineMapsSettings; }
PlanViewSettings *SettingsManager::planViewSettings() const { return _planViewSettings; }
RemoteIDSettings *SettingsManager::remoteIDSettings() const { return _remoteIDSettings; }
RTKSettings *SettingsManager::rtkSettings() const { return _rtkSettings; }
UnitsSettings *SettingsManager::unitsSettings() const { return _unitsSettings; }
VideoSettings *SettingsManager::videoSettings() const { return _videoSettings; }
MavlinkSettings *SettingsManager::mavlinkSettings() const { return _mavlinkSettings; }
#ifdef QGC_VIEWER3D
Viewer3DSettings *SettingsManager::viewer3DSettings() const { return _viewer3DSettings; }
#endif

void SettingsManager::_loadSettingsFiles()
{
    // Settings files can be found in the settingsSavePath() directory
    // Settings files are json files which end in the settingsFileExtension extension
    // The format for a settings file is:
    // {
    //      "version": 1,
    //      "fileType": "Settings",
    //      "groups": {
    //          "groupName": {
    //              "settingName": {
    //                  "forceRawValue": <value>, // Forces the rawValue for this setting to the specific value
    //                  any FactMetaData json keys except for name and type,
    //                  ...
    //          },
    //          "groupName": {
    //              ...
    //          }
    //      }
    // }

    QDir settingsDir(_appSettings->settingsSavePath());
    if (!settingsDir.exists()) {
        qCWarning(SettingsManagerLog) << "Settings directory does not exist:" << settingsDir.absolutePath();
        return;
    }

    QStringList settingsFiles = settingsDir.entryList(QStringList() << QString("*.%1").arg(_appSettings->settingsFileExtension), QDir::Files);
    for (const QString &fileName : settingsFiles) {
        QFileInfo fileInfo(settingsDir, fileName);
        if (!fileInfo.isFile()) continue;

        // Load the settings file
        qCDebug(SettingsManagerLog) << "Loading settings file:" << fileInfo.absoluteFilePath();

        QJsonDocument jsonDoc;
        QString errorString;
        if (!JsonHelper::isJsonFile(fileInfo.absoluteFilePath(), jsonDoc, errorString)) {
            qCWarning(SettingsManagerLog) << "Failed to load settings file:" << fileInfo.absoluteFilePath() << errorString;
            continue;
        }

        QJsonObject jsonObject = jsonDoc.object();

        // Validate the settings file
        int version;
        if (!JsonHelper::validateInternalQGCJsonFile(jsonObject, "Settings", 1, 1, version, errorString)) {
            qCWarning(SettingsManagerLog) << "Settings file failed validation:" << fileInfo.absoluteFilePath() << errorString;
            continue;
        }

        // Validate the remainder of the file

        // groups key is an object
        static const QList<JsonHelper::KeyValidateInfo> keyInfoList = {
            { kJsonGroupsObjectKey, QJsonValue::Object, true },
        };
        if (!JsonHelper::validateKeys(jsonObject, keyInfoList, errorString)) {
            qCWarning(SettingsManagerLog) << "Settings file incorrect format:" << fileInfo.absoluteFilePath() << errorString;
            continue;
        }

        auto groupsObject = jsonObject[kJsonGroupsObjectKey].toObject();
        for (const QString &groupName : groupsObject.keys()) {
            qCDebug(SettingsManagerLog) << "  Loading settings group:" << groupName;

            const QJsonValue &groupValue = groupsObject[groupName];
            if (!groupValue.isObject()) {
                qCWarning(SettingsManagerLog) << "Settings file incorrect format, group is not an object:" << fileInfo.absoluteFilePath()
                                            << groupName;
                continue;
            }

            auto groupObject = groupValue.toObject();
            for (const QString &settingName : groupObject.keys()) {
                qCDebug(SettingsManagerLog) << "  Loading settings:" << groupName << settingName;

                if (!groupObject[settingName].isObject()) {
                    qCWarning(SettingsManagerLog) << "Settings file incorrect format, setting is not an object:" << fileInfo.absoluteFilePath() 
                                                << groupName << settingName;
                    continue;
                }

                // Store the setting overrides. Note that last one wins if there are multiple settings files with the same setting.
                QJsonObject metaDataObject = groupObject[settingName].toObject();
                _settingsFileOverrides[groupName][settingName] = metaDataObject;
            }
        }
    }
}

void SettingsManager::adjustSettingMetaData(const QString &settingsGroup, FactMetaData &metaData, bool &visible)
{
    visible = true; // By default all settings are visible

    SettingsManager *settingsManager = SettingsManager::instance();
    if (!settingsManager) {
        qCWarning(SettingsManagerLog) << "SettingsManager instance not available";
        return;
    }

    if (!qgcApp()->runningUnitTests()) {
        // Apply settings file overrides
        const auto &groupOverrides = settingsManager->_settingsFileOverrides;
        if (groupOverrides.contains(settingsGroup) && groupOverrides[settingsGroup].contains(metaData.name())) {
            QJsonObject settingOverrideJsonObject = groupOverrides[settingsGroup][metaData.name()];

            // We need to stuff in name and type so settingOverrideJsonObject can parse properly
            settingOverrideJsonObject["name"] = metaData.name();
            settingOverrideJsonObject["type"] = FactMetaData::typeToString(metaData.type());

            qCDebug(SettingsManagerLog) << "Applying settings file override for" << settingsGroup << metaData.name();

            QScopedPointer<FactMetaData> overrideMetaData(FactMetaData::createFromJsonObject(settingOverrideJsonObject, {}, nullptr));

            // Apply overrides
            for (const QString &metaDataName : settingOverrideJsonObject.keys()) {
                if (metaDataName == kJsonVisibleKey) {
                    qCDebug(SettingsManagerLog) << "  Setting visibility to" << settingOverrideJsonObject[kJsonVisibleKey].toBool();
                    visible = settingOverrideJsonObject[kJsonVisibleKey].toBool();
                } else if (metaDataName == kJsonForceRawValueKey) {
                    qCDebug(SettingsManagerLog) << "  Setting forceRawValue to" << settingOverrideJsonObject[kJsonForceRawValueKey];
                    metaData.setRawDefaultValue(settingOverrideJsonObject[kJsonForceRawValueKey].toVariant());
                    visible = false;
                } else if (metaDataName == FactMetaData::_defaultValueJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting default to" << overrideMetaData->rawDefaultValue();
                    metaData.setRawDefaultValue(overrideMetaData->rawDefaultValue());
                } else if (metaDataName == FactMetaData::_minJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting min to" << overrideMetaData->rawMin();
                    metaData.setRawMin(overrideMetaData->rawMin());
                } else if (metaDataName == FactMetaData::_maxJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting max to" << overrideMetaData->rawMax();
                    metaData.setRawMax(overrideMetaData->rawMax());
                } else if (metaDataName == FactMetaData::_decimalPlacesJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting decimalPlaces to" << overrideMetaData->decimalPlaces();
                    metaData.setDecimalPlaces(overrideMetaData->decimalPlaces());
                } else if (metaDataName == FactMetaData::_enumValuesJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting enumInfo to" << overrideMetaData->enumValues() << overrideMetaData->enumStrings();
                    metaData.setEnumInfo(overrideMetaData->enumStrings(), overrideMetaData->enumValues());
                } else if (metaDataName == FactMetaData::_enumBitmaskArrayJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting bitmaskInfo to" << overrideMetaData->bitmaskValues() << overrideMetaData->bitmaskStrings();
                    metaData.setBitmaskInfo(overrideMetaData->bitmaskStrings(), overrideMetaData->bitmaskValues());
                } else if (metaDataName == FactMetaData::_longDescriptionJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting longDesc to" << overrideMetaData->longDescription();
                    metaData.setLongDescription(overrideMetaData->longDescription());
                } else if (metaDataName == FactMetaData::_shortDescriptionJsonKey) {
                    qCDebug(SettingsManagerLog) << "  Setting shortDesc to" << overrideMetaData->shortDescription();
                    metaData.setShortDescription(overrideMetaData->shortDescription());
                }
            }
        }
    }

    // Give QGCCorePlugin a whack at it too
    QGCCorePlugin::instance()->adjustSettingMetaData(settingsGroup, metaData, visible);
}