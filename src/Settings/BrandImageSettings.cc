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

DECLARE_SETTINGGROUP(BrandImage, "Branding")
{
    qmlRegisterUncreatableType<BrandImageSettings>("QGroundControl.SettingsManager", 1, 0, "BrandImageSettings", "Reference only"); \
}

DECLARE_SETTINGSFACT(BrandImageSettings, userBrandImageIndoor)
DECLARE_SETTINGSFACT(BrandImageSettings, userBrandImageOutdoor)
