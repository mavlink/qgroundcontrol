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

class FlyViewSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    FlyViewSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* guidedMinimumAltitude  READ guidedMinimumAltitude  CONSTANT)
    Q_PROPERTY(Fact* guidedMaximumAltitude  READ guidedMaximumAltitude  CONSTANT)

    Fact* guidedMinimumAltitude(void);
    Fact* guidedMaximumAltitude(void);

    static const char* name;
    static const char* settingsGroup;

    static const char* guidedMinimumAltitudeName;
    static const char* guidedMaximumAltitudeName;

private:
    SettingsFact* _guidedMinimumAltitudeFact;
    SettingsFact* _guidedMaximumAltitudeFact;
};
