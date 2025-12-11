/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIPSettings.h"

NTRIPSettings::NTRIPSettings(QObject* parent)
    : SettingsGroup("NTRIP", "NTRIP", parent)
{
    FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeBool, this);
    metaData->setName("ntripServerConnectEnabled");
    metaData->setShortDescription(tr("Enable NTRIP"));
    metaData->setRawDefaultValue(false);
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeString, this);
    metaData->setName("ntripServerHostAddress");
    metaData->setShortDescription(tr("Host Address"));
    metaData->setRawDefaultValue("");
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeUint16, this);
    metaData->setName("ntripServerPort");
    metaData->setShortDescription(tr("Port"));
    metaData->setRawDefaultValue(2101);
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeString, this);
    metaData->setName("ntripUsername");
    metaData->setShortDescription(tr("Username"));
    metaData->setRawDefaultValue("");
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeString, this);
    metaData->setName("ntripPassword");
    metaData->setShortDescription(tr("Password"));
    metaData->setRawDefaultValue("");
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeString, this);
    metaData->setName("ntripMountpoint");
    metaData->setShortDescription(tr("Mount Point"));
    metaData->setRawDefaultValue("");
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeString, this);
    metaData->setName("ntripWhitelist");
    metaData->setShortDescription(tr("RTCM Message Whitelist"));
    metaData->setRawDefaultValue("");
    _nameToMetaDataMap[metaData->name()] = metaData;

    metaData = new FactMetaData(FactMetaData::valueTypeBool, this);
    metaData->setName("ntripUseSpartn");
    metaData->setShortDescription(tr("Use SPARTN pipeline"));
    metaData->setRawDefaultValue(false);
    _nameToMetaDataMap[metaData->name()] = metaData;

    // Force ntripServerConnectEnabled to false at every startup, ignoring saved settings
    // This ensures NTRIP never auto-starts regardless of previous user state
    if (ntripServerConnectEnabled()) {
        ntripServerConnectEnabled()->setRawValue(false);
    }
}

DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerConnectEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerHostAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerPort)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUsername)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripPassword)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripMountpoint)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripWhitelist)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUseSpartn)
