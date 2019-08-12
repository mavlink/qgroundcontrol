/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom QtQuick Interface
 *   @author Gus Grubba <gus@auterion.com>
 */

#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkLogManager.h"
#include "QGCMapEngine.h"
#include "QGCApplication.h"
#include "PositionManager.h"

#include "CustomPlugin.h"
#include "CustomQuickInterface.h"

#include <QSettings>

static const char* kGroupName       = "CustomSettings";
static const char* kShowGimbalCtl   = "ShowGimbalCtl";

//-----------------------------------------------------------------------------
CustomQuickInterface::CustomQuickInterface(QObject* parent)
    : QObject(parent)
{
    qCDebug(CustomLog) << "CustomQuickInterface Created";
}

//-----------------------------------------------------------------------------
CustomQuickInterface::~CustomQuickInterface()
{
    qCDebug(CustomLog) << "CustomQuickInterface Destroyed";
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::init()
{
    QSettings settings;
    settings.beginGroup(kGroupName);
    _showGimbalControl = settings.value(kShowGimbalCtl, false).toBool();
    QGCToolbox* toolbox = qgcApp()->toolbox();
    connect(toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &CustomQuickInterface::_activeVehicleChanged);
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setShowGimbalControl(bool set)
{
    if(_showGimbalControl != set) {
        _showGimbalControl = set;
        QSettings settings;
        settings.beginGroup(kGroupName);
        settings.setValue(kShowGimbalCtl,set);
        emit showGimbalControlChanged();
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::setTestFlight(bool b)
{
#if defined(QT_DEBUG)
    //-- Debug builds are always test mode
    b = true;
#endif
    _testFlight = b;
    emit testFlightChanged();
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::_activeVehicleChanged(Vehicle* vehicle)
{
    if(_vehicle) {
        disconnect(_vehicle, &Vehicle::armedChanged, this, &CustomQuickInterface::_armedChanged);
        _vehicle = nullptr;
    }
    if(vehicle) {
        _vehicle = vehicle;
        connect(_vehicle, &Vehicle::armedChanged, this, &CustomQuickInterface::_armedChanged);
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::_armedChanged(bool armed)
{
    if(_vehicle) {
        if(armed) {
            _sendLogMessage();
        }
    }
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::_sendLogMessage()
{
    static int LOG_VERSION = 1;
    if(_vehicle) {
        QString paylod;
        paylod.sprintf("#0v:%d\np:%s\nt:%d\nc:%c",
            LOG_VERSION,
            _pilotID.toLatin1().data(),
            _testFlight ? 1 : 0,
            _checkListState == NotSetup ? 'n' : (_checkListState == Passed ? 'p' : 'f'));
        //-- Handle batteries
        foreach(const QString battery, _batteries) {
            paylod.append("\nb:");
            paylod.append(battery);
        }
        mavlink_message_t msg;
        qCDebug(CustomLog) << "Log Message Sent:" << paylod;
        mavlink_msg_statustext_pack_chan(
            static_cast<uint8_t>(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId()),
            static_cast<uint8_t>(qgcApp()->toolbox()->mavlinkProtocol()->getComponentId()),
            _vehicle->priorityLink()->mavlinkChannel(),
            &msg,
            MAV_SEVERITY_NOTICE,
            paylod.toLatin1().data());
        _vehicle->sendMessageMultiple(msg);
    }
}

//-----------------------------------------------------------------------------
bool
CustomQuickInterface::addBatteryScan(QString batteryID)
{
    //-- Arbitrary limit of 4 batteries
    if(_batteries.length() < 4 && !_batteries.contains(batteryID)) {
        _batteries.append(batteryID);
        emit batteriesChanged();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::resetBatteries()
{
    _batteries.clear();
    emit batteriesChanged();
}
