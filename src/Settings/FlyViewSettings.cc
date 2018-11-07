/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FlyViewSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>
#include <QStandardPaths>

const char* FlyViewSettings::name =                          "FlyView";
const char* FlyViewSettings::settingsGroup =                 "FlyView";

const char* FlyViewSettings::guidedMinimumAltitudeName =  "GuidedMinimumAltitude";
const char* FlyViewSettings::guidedMaximumAltitudeName =  "GuidedMaximumAltitude";

FlyViewSettings::FlyViewSettings(QObject* parent)
    : SettingsGroup             (name, settingsGroup, parent)
    , _guidedMinimumAltitudeFact(NULL)
    , _guidedMaximumAltitudeFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<FlyViewSettings>("QGroundControl.SettingsManager", 1, 0, "FlyViewSettings", "Reference only");
}

Fact* FlyViewSettings::guidedMinimumAltitude(void)
{
    if (!_guidedMinimumAltitudeFact) {
        _guidedMinimumAltitudeFact = _createSettingsFact(guidedMinimumAltitudeName);
    }
    return _guidedMinimumAltitudeFact;
}

Fact* FlyViewSettings::guidedMaximumAltitude(void)
{
    if (!_guidedMaximumAltitudeFact) {
        _guidedMaximumAltitudeFact = _createSettingsFact(guidedMaximumAltitudeName);
    }
    return _guidedMaximumAltitudeFact;
}
