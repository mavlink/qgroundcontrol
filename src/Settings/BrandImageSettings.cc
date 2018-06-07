/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BrandImageSettings.h"

#include <QQmlEngine>
#include <QtQml>

const char* BrandImageSettings::name =                      "BrandImage";
const char* BrandImageSettings::settingsGroup =             ""; // settings are in root group

const char* BrandImageSettings::userBrandImageIndoorName =  "UserBrandImageIndoor";
const char* BrandImageSettings::userBrandImageOutdoorName = "UserBrandImageOutdoor";

BrandImageSettings::BrandImageSettings(QObject* parent)
    : SettingsGroup(name, settingsGroup, parent)
    , _userBrandImageIndoorFact(NULL)
    , _userBrandImageOutdoorFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<BrandImageSettings>("QGroundControl.SettingsManager", 1, 0, "BrandImageSettings", "Reference only");
}

Fact* BrandImageSettings::userBrandImageIndoor(void)
{
    if (!_userBrandImageIndoorFact) {
        _userBrandImageIndoorFact = _createSettingsFact(userBrandImageIndoorName);
    }

    return _userBrandImageIndoorFact;
}

Fact* BrandImageSettings::userBrandImageOutdoor(void)
{
    if (!_userBrandImageOutdoorFact) {
        _userBrandImageOutdoorFact = _createSettingsFact(userBrandImageOutdoorName);
    }

    return _userBrandImageOutdoorFact;
}
