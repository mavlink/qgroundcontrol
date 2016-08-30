/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "GeoFenceManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(GeoFenceManagerLog, "GeoFenceManagerLog")

const char* GeoFenceManager::_fenceTotalParam =     "FENCE_TOTAL";
const char* GeoFenceManager::_fenceActionParam =    "FENCE_ACTION";

GeoFenceManager::GeoFenceManager(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _readTransactionInProgress(false)
    , _writeTransactionInProgress(false)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &GeoFenceManager::_mavlinkMessageReceived);
}

GeoFenceManager::~GeoFenceManager()
{

}

void GeoFenceManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(GeoFenceManagerLog) << "Sending error" << errorCode << errorMsg;

    emit error(errorCode, errorMsg);
}

void GeoFenceManager::setGeoFence(const GeoFence_t& geoFence)
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

    // First thing is to turn off geo fence while we are updating. This prevents the vehicle from going haywire it is in the air
    Fact* fenceActionFact = _vehicle->getParameterFact(FactSystem::defaultComponentId, _fenceActionParam);
    _savedWriteFenceAction = fenceActionFact->rawValue();
    fenceActionFact->setRawValue(0);

    // Fence total param includes:
    //  index 0: breach return
    //  last index: polygon close (same as first polygon point)
    _vehicle->getParameterFact(FactSystem::defaultComponentId, _fenceTotalParam)->setRawValue(_geoFence.polygon.count() + 2);

    // FIXME: No validation of correct fence received
    // Send points:
    //  breach return
    //  polygon fence points
    //  polygon close
    // Put back previous fence action to start geofence again
    _sendFencePoint(0);
    for (uint8_t index=0; index<_geoFence.polygon.count(); index++) {
        _sendFencePoint(index + 1);
    }
    _sendFencePoint(_geoFence.polygon.count() + 1);
    fenceActionFact->setRawValue(_savedWriteFenceAction);
}

void GeoFenceManager::requestGeoFence(void)
{
    _clearGeoFence();

    if (!_geoFenceSupported()) {
        return;
    }

    // Point 0: Breach return point
    // Point [1,N]: Polygon points
    // Point N+1: Close polygon point (same as point 1)
    int cFencePoints = _vehicle->getParameterFact(FactSystem::defaultComponentId, _fenceTotalParam)->rawValue().toInt();
    qCDebug(GeoFenceManagerLog) << "GeoFenceManager::requestGeoFence" << cFencePoints;
    if (cFencePoints == 0) {
        // No fence, no more work to do, fence data has already been cleared
        return;
    }
    if (cFencePoints < 0 || (cFencePoints > 0 && cFencePoints < 6)) {
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
void GeoFenceManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_FENCE_POINT) {
        mavlink_fence_point_t fencePoint;

        mavlink_msg_fence_point_decode(&message, &fencePoint);
        if (fencePoint.idx != _currentFencePoint) {
            // FIXME: Protocol out of whack
            qCWarning(GeoFenceManagerLog) << "Indices out of sync" << fencePoint.idx << _currentFencePoint;
            return;
        }

        if (fencePoint.idx == 0) {
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

void GeoFenceManager::_clearGeoFence(void)
{
    _geoFence.fenceType = GeoFenceNone;
    _geoFence.circleRadius = 0.0;
    _geoFence.polygon.clear();
    _geoFence.breachReturnPoint = QGeoCoordinate();
    emit newGeoFenceAvailable();
}

void GeoFenceManager::_requestFencePoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    qCDebug(GeoFenceManagerLog) << "GeoFenceManager::_requestFencePoint" << pointIndex;
    mavlink_msg_fence_fetch_point_pack(mavlink->getSystemId(),
                                       mavlink->getComponentId(),
                                       &msg,
                                       _vehicle->id(),
                                       _vehicle->defaultComponentId(),
                                       pointIndex);
    _vehicle->sendMessageOnPriorityLink(msg);
}

void GeoFenceManager::_sendFencePoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    QGeoCoordinate fenceCoord;
    if (pointIndex == 0) {
        fenceCoord = _geoFence.breachReturnPoint;
    } else if (pointIndex - 1 < _geoFence.polygon.count()) {
        fenceCoord = _geoFence.polygon[pointIndex - 1];
    } else {
        // Polygon close point
        fenceCoord = _geoFence.polygon[0];
    }

    mavlink_msg_fence_point_pack(mavlink->getSystemId(),
                                 mavlink->getComponentId(),
                                 &msg,
                                 _vehicle->id(),
                                 _vehicle->defaultComponentId(),
                                 pointIndex,                        // Index of point to set
                                 _geoFence.polygon.count() + 2,     // Total point count, +1 for breach in index 0, +1 polygon close in last index
                                 fenceCoord.latitude(),
                                 fenceCoord.longitude());
    _vehicle->sendMessageOnPriorityLink(msg);
}

bool GeoFenceManager::inProgress(void) const
{
    return _readTransactionInProgress || _writeTransactionInProgress;
}

bool GeoFenceManager::_geoFenceSupported(void)
{
    // FIXME: MockLink doesn't support geo fence yet
    if (qgcApp()->runningUnitTests()) {
        return false;
    }

    // FIXME: Hack to get around lack of plugin-ized version of code
    if (_vehicle->apmFirmware()) {
        if (!_vehicle->parameterExists(FactSystem::defaultComponentId, _fenceTotalParam) ||
                !_vehicle->parameterExists(FactSystem::defaultComponentId, _fenceActionParam)) {
            _sendError(InternalError, QStringLiteral("Vehicle does not support Geo-Fence implementation."));
            return false;
        } else {
            return true;
        }
    } else {
        return false;
    }
}
