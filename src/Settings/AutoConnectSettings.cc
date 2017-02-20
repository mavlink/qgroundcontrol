/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AutoConnectSettings.h"

#include <QQmlEngine>

const char* AutoConnectSettings::autoConnectSettingsGroupName = "autoConnect";

AutoConnectSettings::AutoConnectSettings(QObject* parent)
    : SettingsGroup(autoConnectSettingsGroupName, QString() /* root settings group */, parent)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AutoConnectSettings>("QGroundControl.SettingsManager", 1, 0, "AutoConnectSettings", "Reference only");
}
