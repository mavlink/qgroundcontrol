/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef FlightMapSettings_H
#define FlightMapSettings_H

#include "SettingsGroup.h"

class FlightMapSettings : public SettingsGroup
{
    Q_OBJECT

public:
    FlightMapSettings(QObject* parent = NULL);

    // This enum must match the json meta data
    typedef enum {
        mapProviderBing,
        mapProviderGoogle,
        mapProviderStarkart,
        mapProviderMapbox,
        mapProviderEsri,
        mapProviderEniro
    } MapProvider_t;

    // This enum must match the json meta data
    typedef enum {
        mapTypeStreet,
        mapTypeSatellite,
        mapTypeHybrid,
        mapTypeTerrain
    } MapType_t;

    Q_PROPERTY(Fact* mapProvider     READ mapProvider   CONSTANT)               ///< Currently selected map provider
    Q_PROPERTY(Fact* mapType         READ mapType       NOTIFY mapTypeChanged)  ///< Current selected map type

    Fact* mapProvider   (void);
    Fact* mapType       (void);

    static const char* name;
    static const char* settingsGroup;

    static const char* mapProviderSettingsName;
    static const char* mapTypeSettingsName;

signals:
    void mapTypeChanged(void);

private slots:
    void _newMapProvider(QVariant value);

private:
    void _removeEnumValue(int value, QStringList& enumStrings, QVariantList& enumValues);
    void _excludeProvider(MapProvider_t provider);

    SettingsFact*   _mapProviderFact;
    SettingsFact*   _mapTypeFact;
    QStringList     _savedMapTypeStrings;
    QVariantList    _savedMapTypeValues;
};

#endif
