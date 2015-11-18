/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGroundControlQmlGlobal.h"
#include "QGCApplication.h"

#include <QSettings>

static const char* kQmlGlobalKeyName = "QGCQml";

QGroundControlQmlGlobal::QGroundControlQmlGlobal(QGCToolbox* toolbox, QObject* parent)
    : QObject(parent)
    , _multiVehicleManager(toolbox->multiVehicleManager())
    , _linkManager(toolbox->linkManager())
    , _homePositionManager(toolbox->homePositionManager())
    , _flightMapSettings(toolbox->flightMapSettings())
{

}

void QGroundControlQmlGlobal::saveGlobalSetting (const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    settings.setValue(key, value);
}

QString QGroundControlQmlGlobal::loadGlobalSetting (const QString& key, const QString& defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    return settings.value(key, defaultValue).toString();
}

void QGroundControlQmlGlobal::saveBoolGlobalSetting (const QString& key, bool value)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    settings.setValue(key, value);
}

bool QGroundControlQmlGlobal::loadBoolGlobalSetting (const QString& key, bool defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    return settings.value(key, defaultValue).toBool();
}

#ifdef QT_DEBUG
void QGroundControlQmlGlobal::_startMockLink(MockConfiguration* mockConfig)
{
    LinkManager* linkManager = qgcApp()->toolbox()->linkManager();

    mockConfig->setDynamic(true);
    linkManager->linkConfigurations()->append(mockConfig);

    linkManager->createConnectedLink(mockConfig, false /* autoconnectLink */);
}
#endif

void QGroundControlQmlGlobal::startPX4MockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockConfiguration* mockConfig = new MockConfiguration("PX4 MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setSendStatusText(sendStatusText);

    _startMockLink(mockConfig);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startGenericMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockConfiguration* mockConfig = new MockConfiguration("Generic MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_GENERIC);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setSendStatusText(sendStatusText);

    _startMockLink(mockConfig);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduCopterMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockConfiguration* mockConfig = new MockConfiguration("APM ArduCopter MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setSendStatusText(sendStatusText);

    _startMockLink(mockConfig);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduPlaneMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockConfiguration* mockConfig = new MockConfiguration("APM ArduPlane MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    mockConfig->setVehicleType(MAV_TYPE_FIXED_WING);
    mockConfig->setSendStatusText(sendStatusText);

    _startMockLink(mockConfig);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::stopAllMockLinks(void)
{
#ifdef QT_DEBUG
    LinkManager* linkManager = qgcApp()->toolbox()->linkManager();

    for (int i=0; i<linkManager->links()->count(); i++) {
        LinkInterface* link = linkManager->links()->value<LinkInterface*>(i);
        MockLink* mockLink = qobject_cast<MockLink*>(link);

        if (mockLink) {
            linkManager->disconnectLink(mockLink, false /* disconnectAutoconnectLink */);
        }
    }
#endif
}
