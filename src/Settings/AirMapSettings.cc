/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(AirMap)
{
    INIT_SETTINGFACT(apiKey);
    INIT_SETTINGFACT(clientID);
    INIT_SETTINGFACT(userName);
    INIT_SETTINGFACT(password);
    INIT_SETTINGFACT(sitaUavReg);
    INIT_SETTINGFACT(sitaUserReg);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AirMapSettings>("QGroundControl.SettingsManager", 1, 0, "AirMapSettings", "Reference only");
}

DECLARE_SETTINGSFACT(AirMapSettings, apiKey)
DECLARE_SETTINGSFACT(AirMapSettings, clientID)
DECLARE_SETTINGSFACT(AirMapSettings, userName)
DECLARE_SETTINGSFACT(AirMapSettings, password)
DECLARE_SETTINGSFACT(AirMapSettings, sitaUavReg)
DECLARE_SETTINGSFACT(AirMapSettings, sitaUserReg)
