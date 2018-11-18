/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class FlightMapSettings : public SettingsGroup
{
    Q_OBJECT

public:
    FlightMapSettings(QObject* parent = nullptr);

    // This enum must match the json meta data
    typedef enum {
        mapProviderBing,
        mapProviderGoogle,
        mapProviderStarkart,
        mapProviderMapbox,
        mapProviderEsri,
        mapProviderEniro,
        mapProviderVWorld
    } MapProvider_t;

    // This enum must match the json meta data
    typedef enum {
        mapTypeStreet,
        mapTypeSatellite,
        mapTypeHybrid,
        mapTypeTerrain
    } MapType_t;

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(mapProvider)
    DEFINE_SETTINGFACT(mapType)

signals:
    void mapTypeChanged(void);

private slots:
    void _newMapProvider(QVariant value);

private:
    void _removeEnumValue(int value, QStringList& enumStrings, QVariantList& enumValues);
    void _excludeProvider(MapProvider_t provider);

    QStringList     _savedMapTypeStrings;
    QVariantList    _savedMapTypeValues;
};
