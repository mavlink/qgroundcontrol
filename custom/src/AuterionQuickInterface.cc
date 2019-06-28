/*!
 * @file
 *   @brief Auterion QtQuick Interface
 *   @author Gus Grubba <gus@grubba.com>
 */

#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkLogManager.h"
#include "QGCMapEngine.h"
#include "QGCApplication.h"
#include "PositionManager.h"

#include "AuterionPlugin.h"
#include "AuterionQuickInterface.h"

#include <QDirIterator>
#include <QtAlgorithms>

//-----------------------------------------------------------------------------
AuterionQuickInterface::AuterionQuickInterface(QObject* parent)
    : QObject(parent)
{
    qCDebug(AuterionLog) << "AuterionQuickInterface Created";
}

//-----------------------------------------------------------------------------
AuterionQuickInterface::~AuterionQuickInterface()
{
    qCDebug(AuterionLog) << "AuterionQuickInterface Destroyed";
}

//-----------------------------------------------------------------------------
void
AuterionQuickInterface::init()
{
    QGCToolbox* toolbox = qgcApp()->toolbox();
    connect(toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &AuterionQuickInterface::_activeVehicleChanged);
}

//-----------------------------------------------------------------------------
void
AuterionQuickInterface::setTestFlight(bool b)
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
AuterionQuickInterface::_activeVehicleChanged(Vehicle* vehicle)
{
    if(_vehicle) {
        disconnect(_vehicle, &Vehicle::armedChanged, this, &AuterionQuickInterface::_armedChanged);
        _vehicle = nullptr;
    }
    if(vehicle) {
        _vehicle = vehicle;
        connect(_vehicle, &Vehicle::armedChanged, this, &AuterionQuickInterface::_armedChanged);
    }
}

//-----------------------------------------------------------------------------
void
AuterionQuickInterface::_armedChanged(bool armed)
{
    if(_vehicle) {
        if(armed) {
            _sendLogMessage();
        }
    }
}

//-----------------------------------------------------------------------------
void
AuterionQuickInterface::_sendLogMessage()
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
        qCDebug(AuterionLog) << "Log Message Sent:" << paylod;
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
AuterionQuickInterface::addBatteryScan(QString batteryID)
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
AuterionQuickInterface::resetBatteries()
{
    _batteries.clear();
    emit batteriesChanged();
}
