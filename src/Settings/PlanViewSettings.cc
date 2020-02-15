/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PlanViewSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(PlanView, "PlanView")
{
    qmlRegisterUncreatableType<PlanViewSettings>("QGroundControl.SettingsManager", 1, 0, "PlanViewSettings", "Reference only"); \
}

DECLARE_SETTINGSFACT(PlanViewSettings, displayPresetsTabFirst)
DECLARE_SETTINGSFACT(PlanViewSettings, aboveTerrainWarning)
DECLARE_SETTINGSFACT(PlanViewSettings, showMissionItemStatus)
