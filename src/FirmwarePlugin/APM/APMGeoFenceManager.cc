/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMGeoFenceManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"

const char* APMGeoFenceManager::_fenceTotalParam =     "FENCE_TOTAL";
const char* APMGeoFenceManager::_fenceActionParam =    "FENCE_ACTION";
const char* APMGeoFenceManager::_fenceEnableParam =    "FENCE_ENABLE";

APMGeoFenceManager::APMGeoFenceManager(Vehicle* vehicle)
    : GeoFenceManager(vehicle)
    , _readTransactionInProgress(false)
    , _writeTransactionInProgress(false)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &APMGeoFenceManager::_mavlinkMessageReceived);
}

APMGeoFenceManager::~APMGeoFenceManager()
{

}

void APMGeoFenceManager::setGeoFence(const GeoFence_t& geoFence)
{
    if (_readTransactionInProgress) {
        _sendError(InternalError, QStringLiteral("Geo-Fence write attempted while read in progress."));
        return;
    }

    if (!_geoFenceSupported()) {
        return;
    }

    // Validate
    switch (geoFence.fenceType) {
    case GeoFencePolygon:
        if (geoFence.polygon.count() < 3) {
            _sendError(TooFewPoints, QStringLiteral("Geo-Fence polygon must contain at least 3 points."));
            return;
        }
        if (geoFence.polygon.count() > std::numeric_limits<uint8_t>::max()) {
            _sendError(TooManyPoints, QStringLiteral("Geo-Fence polygon has too many points: %1.").arg(geoFence.polygon.count()));
            return;
        }
        break;
    case GeoFenceCircle:
        if (geoFence.circleRadius <= 0.0) {
            _sendError(InvalidCircleRadius, QStringLiteral("Geo-Fence circle radius must be greater than 0."));
            return;
        }
    default:
        break;
    }

    _geoFence.fenceType =           geoFence.fenceType;
    _geoFence.circleRadius =        geoFence.circleRadius;
    _geoFence.polygon =             geoFence.polygon;
    _geoFence.breachReturnPoint =   geoFence.breachReturnPoint;

    if (_geoFence.fenceType != GeoFencePolygon) {
        // Circle is just params, so no more work to do
        return;
    }

    emit newGeoFenceAvailable();

    // First thing is to turn off geo fence while we are updating. This prevents the vehicle from going haywire it is in the air.
    // Unfortunately the param to do this with differs between plane and copter.
    const char* enableParam = _vehicle->fixedWing() ? _fenceActionParam : _fenceEnableParam;
    Fact* fenceEnableFact = _vehicle->getParameterFact(FactSystem::defaultComponentId, enableParam);
    QVariant savedEnableState = fenceEnableFact->rawValue();
    fenceEnableFact->setRawValue(0);

    // Total point count, +1 polygon close in last index, +1 for breach in index 0 (ArduPlane only)
    _cWriteFencePoints = _geoFence.polygon.count() + 1 + (_vehicle->fixedWing() ? 1 : 0);
    _vehicle->getParameterFact(FactSystem::defaultComponentId, _fenceTotalParam)->setRawValue(_cWriteFencePoints);

    // FIXME: No validation of correct fence received

    for (uint8_t index=0; index<_cWriteFencePoints; index++) {
        _sendFencePoint(index);
    }

    fenceEnableFact->setRawValue(savedEnableState);
}

void APMGeoFenceManager::requestGeoFence(void)
{
    _clearGeoFence();

    if (!_geoFenceSupported()) {
        return;
    }

    // Point 0: Breach return point (ArduPlane only)
    // Point [1,N]: Polygon points
    // Point N+1: Close polygon point (same as point 1)
    int cFencePoints = _vehicle->getParameterFact(FactSystem::defaultComponentId, _fenceTotalParam)->rawValue().toInt();
    int minFencePoints = _vehicle->fixedWing() ? 6 : 5;
    qCDebug(GeoFenceManagerLog) << "APMGeoFenceManager::requestGeoFence" << cFencePoints;
    if (cFencePoints == 0) {
        // No fence, no more work to do, fence data has already been cleared
        return;
    }
    if (cFencePoints < 0 || (cFencePoints > 0 && cFencePoints < minFencePoints)) {
        _sendError(TooFewPoints, QStringLiteral("Geo-Fence information from Vehicle has too few points: %1").arg(cFencePoints));
        return;
    }
    if (cFencePoints > std::numeric_limits<uint8_t>::max()) {
        _sendError(TooManyPoints, QStringLiteral("Geo-Fence information from Vehicle has too many points: %1").arg(cFencePoints));
        return;
    }

    _readTransactionInProgress = true;
    _cReadFencePoints = cFencePoints;
    _currentFencePoint = 0;

    _requestFencePoint(_currentFencePoint);
}

/// Called when a new mavlink message for out vehicle is received
void APMGeoFenceManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_FENCE_POINT) {
        mavlink_fence_point_t fencePoint;

        mavlink_msg_fence_point_decode(&message, &fencePoint);
        if (fencePoint.idx != _currentFencePoint) {
            // FIXME: Protocol out of whack
            qCWarning(GeoFenceManagerLog) << "Indices out of sync" << fencePoint.idx << _currentFencePoint;
            return;
        }

        if (fencePoint.idx == 0 && _vehicle->fixedWing()) {
            _geoFence.breachReturnPoint = QGeoCoordinate(fencePoint.lat, fencePoint.lng);
            qCDebug(GeoFenceManagerLog) << "From vehicle: breach return point" << _geoFence.breachReturnPoint;
            _requestFencePoint(++_currentFencePoint);
        } else if (fencePoint.idx < _cReadFencePoints - 1) {
            QGeoCoordinate polyCoord(fencePoint.lat, fencePoint.lng);
            _geoFence.polygon.addCoordinate(polyCoord);
            qCDebug(GeoFenceManagerLog) << "From vehicle: polygon point" << fencePoint.idx << polyCoord;
            if (fencePoint.idx < _cReadFencePoints - 2) {
                // Still more points to request
                _requestFencePoint(++_currentFencePoint);
            } else {
                // We've finished collecting fence points
                _readTransactionInProgress = false;
                emit newGeoFenceAvailable();
            }
        }
    }
}

void APMGeoFenceManager::_requestFencePoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    qCDebug(GeoFenceManagerLog) << "APMGeoFenceManager::_requestFencePoint" << pointIndex;
    mavlink_msg_fence_fetch_point_pack(mavlink->getSystemId(),
                                       mavlink->getComponentId(),
                                       &msg,
                                       _vehicle->id(),
                                       _vehicle->defaultComponentId(),
                                       pointIndex);
    _vehicle->sendMessageOnPriorityLink(msg);
}

void APMGeoFenceManager::_sendFencePoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    QGeoCoordinate fenceCoord;
    if (pointIndex == 0 && _vehicle->fixedWing()) {
        fenceCoord = _geoFence.breachReturnPoint;
    } else if (pointIndex == _cWriteFencePoints - 1) {
        // Polygon close point
        fenceCoord = _geoFence.polygon[0];
    } else {
        // Polygon point
        fenceCoord = _geoFence.polygon[pointIndex - (_vehicle->fixedWing() ? 1 : 0)];
    }

    // Total point count, +1 polygon close in last index, +1 for breach in index 0 (ArduPlane only)
    uint8_t totalPointCount = _geoFence.polygon.count() + 1 + (_vehicle->fixedWing() ? 1 : 0);

    mavlink_msg_fence_point_pack(mavlink->getSystemId(),
                                 mavlink->getComponentId(),
                                 &msg,
                                 _vehicle->id(),
                                 _vehicle->defaultComponentId(),
                                 pointIndex,                        // Index of point to set
                                 totalPointCount,
                                 fenceCoord.latitude(),
                                 fenceCoord.longitude());
    _vehicle->sendMessageOnPriorityLink(msg);
}

bool APMGeoFenceManager::inProgress(void) const
{
    return _readTransactionInProgress || _writeTransactionInProgress;
}

bool APMGeoFenceManager::_geoFenceSupported(void)
{
    // FIXME: MockLink doesn't support geo fence yet
    if (qgcApp()->runningUnitTests()) {
        return false;
    }

    if (!_vehicle->parameterExists(FactSystem::defaultComponentId, _fenceTotalParam) ||
            !_vehicle->parameterExists(FactSystem::defaultComponentId, _fenceActionParam)) {
        return false;
    } else {
        return true;
    }
}
