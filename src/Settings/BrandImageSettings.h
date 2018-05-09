/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef BrandImageSettings_H
#define BrandImageSettings_H

#include "SettingsGroup.h"

class BrandImageSettings : public SettingsGroup
{
    Q_OBJECT

public:
    BrandImageSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* userBrandImageIndoor       READ userBrandImageIndoor       CONSTANT)
    Q_PROPERTY(Fact* userBrandImageOutdoor      READ userBrandImageOutdoor      CONSTANT)

    Fact* userBrandImageIndoor      (void);
    Fact* userBrandImageOutdoor     (void);

    static const char* brandImageSettingsGroupName;

    static const char* userBrandImageIndoorName;
    static const char* userBrandImageOutdoorName;

private:
    SettingsFact* _userBrandImageIndoorFact;
    SettingsFact* _userBrandImageOutdoorFact;
};

#endif
