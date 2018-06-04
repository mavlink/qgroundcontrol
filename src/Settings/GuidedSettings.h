/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef GuidedSettings_H
#define GuidedSettings_H

#include "SettingsGroup.h"
#include "QGCMAVLink.h"

class GuidedSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    GuidedSettings(QObject* parent = NULL);

    // These min/max altitudes are used by the guided altitude slider
    Q_PROPERTY(Fact* fixedWingMinimumAltitude   READ fixedWingMinimumAltitude   CONSTANT)
    Q_PROPERTY(Fact* fixedWingMaximumAltitude   READ fixedWingMaximumAltitude   CONSTANT)
    Q_PROPERTY(Fact* vehicleMinimumAltitude     READ vehicleMinimumAltitude     CONSTANT)
    Q_PROPERTY(Fact* vehicleMaximumAltitude     READ vehicleMaximumAltitude     CONSTANT)

    Fact* fixedWingMinimumAltitude  (void);
    Fact* fixedWingMaximumAltitude  (void);
    Fact* vehicleMinimumAltitude    (void);
    Fact* vehicleMaximumAltitude    (void);

    static const char* name;
    static const char* settingsGroup;

    static const char* fixedWingMinimumAltitudeName;
    static const char* fixedWingMaximumAltitudeName;
    static const char* vehicleMinimumAltitudeName;
    static const char* vehicleMaximumAltitudeName;

private:
    SettingsFact* _fixedWingMinimumAltitudeFact;
    SettingsFact* _fixedWingMaximumAltitudeFact;
    SettingsFact* _vehicleMinimumAltitudeFact;
    SettingsFact* _vehicleMaximumAltitudeFact;
};

#endif
