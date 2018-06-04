/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GuidedSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>
#include <QStandardPaths>

const char* GuidedSettings::name =                          "Guided";
const char* GuidedSettings::settingsGroup =                 ""; // settings are in root group

const char* GuidedSettings::fixedWingMinimumAltitudeName =  "FixedWingMinimumAltitude";
const char* GuidedSettings::fixedWingMaximumAltitudeName =  "FixedWingMaximumAltitude";
const char* GuidedSettings::vehicleMinimumAltitudeName =    "VehicleMinimumAltitude";
const char* GuidedSettings::vehicleMaximumAltitudeName =    "VehicleMaximumAltitude";

GuidedSettings::GuidedSettings(QObject* parent)
    : SettingsGroup(name, settingsGroup, parent)
    , _fixedWingMinimumAltitudeFact (NULL)
    , _fixedWingMaximumAltitudeFact (NULL)
    , _vehicleMinimumAltitudeFact   (NULL)
    , _vehicleMaximumAltitudeFact   (NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<GuidedSettings>("QGroundControl.SettingsManager", 1, 0, "GuidedSettings", "Reference only");
}

Fact* GuidedSettings::fixedWingMinimumAltitude(void)
{
    if (!_fixedWingMinimumAltitudeFact) {
        _fixedWingMinimumAltitudeFact = _createSettingsFact(fixedWingMinimumAltitudeName);
    }

    return _fixedWingMinimumAltitudeFact;
}

Fact* GuidedSettings::fixedWingMaximumAltitude(void)
{
    if (!_fixedWingMaximumAltitudeFact) {
        _fixedWingMaximumAltitudeFact = _createSettingsFact(fixedWingMaximumAltitudeName);
    }

    return _fixedWingMaximumAltitudeFact;
}

Fact* GuidedSettings::vehicleMinimumAltitude(void)
{
    if (!_vehicleMinimumAltitudeFact) {
        _vehicleMinimumAltitudeFact = _createSettingsFact(vehicleMinimumAltitudeName);
    }
    return _vehicleMinimumAltitudeFact;
}

Fact* GuidedSettings::vehicleMaximumAltitude(void)
{
    if (!_vehicleMaximumAltitudeFact) {
        _vehicleMaximumAltitudeFact = _createSettingsFact(vehicleMaximumAltitudeName);
    }
    return _vehicleMaximumAltitudeFact;
}
