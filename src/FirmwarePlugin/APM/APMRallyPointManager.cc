/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMRallyPointManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "ParameterManager.h"

const char* APMRallyPointManager::_rallyTotalParam = "RALLY_TOTAL";

APMRallyPointManager::APMRallyPointManager(Vehicle* vehicle)
    : RallyPointManager(vehicle)
    , _readTransactionInProgress(false)
    , _writeTransactionInProgress(false)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &APMRallyPointManager::_mavlinkMessageReceived);
}

APMRallyPointManager::~APMRallyPointManager()
{

}

void APMRallyPointManager::sendToVehicle(const QList<QGeoCoordinate>& rgPoints)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (_readTransactionInProgress) {
        _sendError(InternalError, QStringLiteral("Rally Point write attempted while read in progress."));
        return;
    }

    _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _rallyTotalParam)->setRawValue(rgPoints.count());

    // FIXME: No validation of correct point received
    _rgPoints = rgPoints;
    for (uint8_t index=0; index<_rgPoints.count(); index++) {
        _sendRallyPoint(index);
    }

    emit loadComplete(_rgPoints);
}

void APMRallyPointManager::loadFromVehicle(void)
{
    if (_vehicle->isOfflineEditingVehicle() || _readTransactionInProgress) {
        return;
    }

    _rgPoints.clear();

    _cReadRallyPoints = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _rallyTotalParam)->rawValue().toInt();
    qCDebug(GeoFenceManagerLog) << "APMGeoFenceManager::loadFromVehicle" << _cReadRallyPoints;
    if (_cReadRallyPoints == 0) {
        emit loadComplete(_rgPoints);
        return;
    }

    _currentRallyPoint = 0;
    _readTransactionInProgress = true;
    _requestRallyPoint(_currentRallyPoint);
}

/// Called when a new mavlink message for out vehicle is received
void APMRallyPointManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_RALLY_POINT) {
        mavlink_rally_point_t rallyPoint;

        mavlink_msg_rally_point_decode(&message, &rallyPoint);
        qCDebug(RallyPointManagerLog) << "From vehicle rally_point: idx:lat:lng:alt" << rallyPoint.idx << rallyPoint.lat << rallyPoint.lng << rallyPoint.alt;

        if (rallyPoint.idx != _currentRallyPoint) {
            // FIXME: Protocol out of whack
            _readTransactionInProgress = false;
            emit inProgressChanged(inProgress());
            qCWarning(RallyPointManagerLog) << "Indices out of sync" << rallyPoint.idx << _currentRallyPoint;
            return;
        }

        QGeoCoordinate point((float)rallyPoint.lat / 1e7, (float)rallyPoint.lng / 1e7, rallyPoint.alt);
        _rgPoints.append(point);
        if (rallyPoint.idx < _cReadRallyPoints - 2) {
            // Still more points to request
            _requestRallyPoint(++_currentRallyPoint);
        } else {
            // We've finished collecting rally points
            qCDebug(RallyPointManagerLog) << "Rally point load complete";
            _readTransactionInProgress = false;
            emit loadComplete(_rgPoints);
            emit inProgressChanged(inProgress());
        }
    }
}

void APMRallyPointManager::_requestRallyPoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    qCDebug(RallyPointManagerLog) << "APMRallyPointManager::_requestRallyPoint" << pointIndex;
    mavlink_msg_rally_fetch_point_pack_chan(mavlink->getSystemId(),
                                            mavlink->getComponentId(),
                                            _vehicle->priorityLink()->mavlinkChannel(),
                                            &msg,
                                            _vehicle->id(),
                                            _vehicle->defaultComponentId(),
                                            pointIndex);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void APMRallyPointManager::_sendRallyPoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    QGeoCoordinate point = _rgPoints[pointIndex];
    mavlink_msg_rally_point_pack_chan(mavlink->getSystemId(),
                                      mavlink->getComponentId(),
                                      _vehicle->priorityLink()->mavlinkChannel(),
                                      &msg,
                                      _vehicle->id(),
                                      _vehicle->defaultComponentId(),
                                      pointIndex,
                                      _rgPoints.count(),
                                      point.latitude() * 1e7,
                                      point.longitude() * 1e7,
                                      point.altitude(),
                                      0, 0, 0);          //  break_alt, land_dir, flags
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

bool APMRallyPointManager::inProgress(void) const
{
    return _readTransactionInProgress || _writeTransactionInProgress;
}
