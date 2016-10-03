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
#include "ParameterManager.h"

const char* APMGeoFenceManager::_fenceTotalParam =     "FENCE_TOTAL";
const char* APMGeoFenceManager::_fenceActionParam =    "FENCE_ACTION";
const char* APMGeoFenceManager::_fenceEnableParam =    "FENCE_ENABLE";

APMGeoFenceManager::APMGeoFenceManager(Vehicle* vehicle)
    : GeoFenceManager(vehicle)
    , _fenceSupported(false)
    , _breachReturnSupported(vehicle->fixedWing())
    , _firstParamLoadComplete(false)
    , _circleRadiusFact(NULL)
    , _readTransactionInProgress(false)
    , _writeTransactionInProgress(false)
    , _fenceEnableFact(NULL)
    , _fenceTypeFact(NULL)
{
    connect(_vehicle,                       &Vehicle::mavlinkMessageReceived,           this, &APMGeoFenceManager::_mavlinkMessageReceived);
    connect(_vehicle->parameterManager(),   &ParameterManager::parametersReadyChanged,  this, &APMGeoFenceManager::_parametersReady);

    if (_vehicle->parameterManager()->parametersReady()) {
        _parametersReady();
    }
}

APMGeoFenceManager::~APMGeoFenceManager()
{

}

void APMGeoFenceManager::sendToVehicle(const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (_readTransactionInProgress) {
        _sendError(InternalError, QStringLiteral("Geo-Fence write attempted while read in progress."));
        return;
    }

    if (!_geoFenceSupported()) {
        return;
    }

    // Validate
    int validatedPolygonCount = polygon.count();
    if (polygonSupported()) {
        if (polygon.count() < 3) {
            validatedPolygonCount = 0;
        }
        if (polygon.count() > std::numeric_limits<uint8_t>::max()) {
            _sendError(TooManyPoints, QStringLiteral("Geo-Fence polygon has too many points: %1.").arg(_polygon.count()));
            validatedPolygonCount = 0;
        }
    }

    _breachReturnPoint = breachReturn;
    _polygon = polygon;

    // First thing is to turn off geo fence while we are updating. This prevents the vehicle from going haywire it is in the air.
    // Unfortunately the param to do this with differs between plane and copter.
    const char* enableParam = _vehicle->fixedWing() ? _fenceActionParam : _fenceEnableParam;
    Fact* fenceEnableFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, enableParam);
    QVariant savedEnableState = fenceEnableFact->rawValue();
    fenceEnableFact->setRawValue(0);

    // Total point count, +1 polygon close in last index, +1 for breach in index 0
    _cWriteFencePoints = (validatedPolygonCount ? (validatedPolygonCount + 1) : 0) + 1 ;
    _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _fenceTotalParam)->setRawValue(_cWriteFencePoints);

    // FIXME: No validation of correct fence received
    for (uint8_t index=0; index<_cWriteFencePoints; index++) {
        _sendFencePoint(index);
    }

    fenceEnableFact->setRawValue(savedEnableState);

    emit loadComplete(_breachReturnPoint, _polygon);
}

void APMGeoFenceManager::loadFromVehicle(void)
{
    if (_vehicle->isOfflineEditingVehicle() || _readTransactionInProgress) {
        return;
    }

    _breachReturnPoint = QGeoCoordinate();
    _polygon.clear();

    if (!_geoFenceSupported()) {
        return;
    }

    // Point 0: Breach return point (ArduPlane only)
    // Point [1,N]: Polygon points
    // Point N+1: Close polygon point (same as point 1)
    int cFencePoints = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _fenceTotalParam)->rawValue().toInt();
    int minFencePoints = 6;
    qCDebug(GeoFenceManagerLog) << "APMGeoFenceManager::loadFromVehicle" << cFencePoints;
    if (cFencePoints == 0) {
        // No fence
        emit loadComplete(_breachReturnPoint, _polygon);
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
        qCDebug(GeoFenceManagerLog) << "From vehicle fence_point: idx:lat:lng" << fencePoint.idx << fencePoint.lat << fencePoint.lng;

        if (fencePoint.idx != _currentFencePoint) {
            // FIXME: Protocol out of whack
            _readTransactionInProgress = false;
            emit inProgressChanged(inProgress());
            qCWarning(GeoFenceManagerLog) << "Indices out of sync" << fencePoint.idx << _currentFencePoint;
            return;
        }

        if (fencePoint.idx == 0) {
            _breachReturnPoint = QGeoCoordinate(fencePoint.lat, fencePoint.lng);
            qCDebug(GeoFenceManagerLog) << "From vehicle: breach return point" << _breachReturnPoint;
            _requestFencePoint(++_currentFencePoint);
        } else if (fencePoint.idx < _cReadFencePoints - 1) {
            QGeoCoordinate polyCoord(fencePoint.lat, fencePoint.lng);
            _polygon.append(polyCoord);
            qCDebug(GeoFenceManagerLog) << "From vehicle: polygon point" << fencePoint.idx << polyCoord;
            if (fencePoint.idx < _cReadFencePoints - 2) {
                // Still more points to request
                _requestFencePoint(++_currentFencePoint);
            } else {
                // We've finished collecting fence points
                qCDebug(GeoFenceManagerLog) << "Fence point load complete";
                _readTransactionInProgress = false;
                emit loadComplete(_breachReturnPoint, _polygon);
                emit inProgressChanged(inProgress());
            }
        }
    }
}

void APMGeoFenceManager::_requestFencePoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    qCDebug(GeoFenceManagerLog) << "APMGeoFenceManager::_requestFencePoint" << pointIndex;
    mavlink_msg_fence_fetch_point_pack_chan(mavlink->getSystemId(),
                                            mavlink->getComponentId(),
                                            _vehicle->priorityLink()->mavlinkChannel(),
                                            &msg,
                                            _vehicle->id(),
                                            _vehicle->defaultComponentId(),
                                            pointIndex);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void APMGeoFenceManager::_sendFencePoint(uint8_t pointIndex)
{
    mavlink_message_t   msg;
    MAVLinkProtocol*    mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    QGeoCoordinate fenceCoord;
    if (pointIndex == 0) {
        fenceCoord = breachReturnPoint();
    } else if (pointIndex == _cWriteFencePoints - 1) {
        // Polygon close point
        fenceCoord = _polygon[0];
    } else {
        // Polygon point
        fenceCoord = _polygon[pointIndex - 1];
    }

    // Total point count, +1 polygon close in last index, +1 for breach in index 0
    uint8_t totalPointCount = _polygon.count() + 1 + 1;

    mavlink_msg_fence_point_pack_chan(mavlink->getSystemId(),
                                      mavlink->getComponentId(),
                                      _vehicle->priorityLink()->mavlinkChannel(),
                                      &msg,
                                      _vehicle->id(),
                                      _vehicle->defaultComponentId(),
                                      pointIndex,                        // Index of point to set
                                      totalPointCount,
                                      fenceCoord.latitude(),
                                      fenceCoord.longitude());
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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

    if (!_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, _fenceTotalParam) ||
            !_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, _fenceActionParam)) {
        return false;
    } else {
        return true;
    }
}

void APMGeoFenceManager::_updateSupportedFlags(void)
{
    emit circleSupportedChanged(circleSupported());
    emit polygonSupportedChanged(polygonSupported());
}

void APMGeoFenceManager::_parametersReady(void)
{
    if (!_firstParamLoadComplete) {
        _firstParamLoadComplete = true;

        _fenceSupported = _vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, QStringLiteral("FENCE_ACTION"));

        if (_fenceSupported) {
            QStringList paramNames;
            QStringList paramLabels;

            if (_vehicle->multiRotor()) {
                _fenceEnableFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("FENCE_ENABLE"));
                _fenceTypeFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("FENCE_TYPE"));

                connect(_fenceEnableFact,   &Fact::rawValueChanged, this, &APMGeoFenceManager::_updateSupportedFlags);
                connect(_fenceTypeFact,     &Fact::rawValueChanged, this, &APMGeoFenceManager::_updateSupportedFlags);

                _circleRadiusFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("FENCE_RADIUS"));
                connect(_circleRadiusFact, &Fact::rawValueChanged, this, &APMGeoFenceManager::_circleRadiusRawValueChanged);
                emit circleRadiusChanged(circleRadius());

                paramNames << QStringLiteral("FENCE_ENABLE") << QStringLiteral("FENCE_TYPE") << QStringLiteral("FENCE_ACTION") << QStringLiteral("FENCE_ALT_MAX")
                           << QStringLiteral("FENCE_RADIUS") << QStringLiteral("FENCE_MARGIN");
                paramLabels << QStringLiteral("Enabled:") << QStringLiteral("Type:") << QStringLiteral("Breach Action:") << QStringLiteral("Max Altitude:")
                            << QStringLiteral("Radius:") << QStringLiteral("Margin:");
            } if (_vehicle->fixedWing()) {
                paramNames << QStringLiteral("FENCE_ACTION") << QStringLiteral("FENCE_MINALT") << QStringLiteral("FENCE_MAXALT") << QStringLiteral("FENCE_RETALT")
                           << QStringLiteral("FENCE_AUTOENABLE") << QStringLiteral("FENCE_RET_RALLY");
                paramLabels << QStringLiteral("Breach Action:") << QStringLiteral("Min Altitude:") << QStringLiteral("Max Altitude:") << QStringLiteral("Return Altitude:")
                            << QStringLiteral("Auto-Enable:") << QStringLiteral("Return to Rally:");
            }

            _params.clear();
            _paramLabels.clear();
            for (int i=0; i<paramNames.count(); i++) {
                QString paramName = paramNames[i];
                if (_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, paramName)) {
                    Fact* paramFact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, paramName);
                    _params << QVariant::fromValue(paramFact);
                    _paramLabels << paramLabels[i];
                }
            }
            emit paramsChanged(_params);
            emit paramLabelsChanged(_paramLabels);

            emit fenceSupportedChanged(_fenceSupported);
            emit circleSupportedChanged(circleSupported());
            emit polygonSupportedChanged(polygonSupported());
        }

        qCDebug(GeoFenceManagerLog) << "fenceSupported:circleSupported:polygonSupported:breachReturnSupported" <<
                                       _fenceSupported << circleSupported() << polygonSupported() << _breachReturnSupported;
    }
}

float APMGeoFenceManager::circleRadius(void) const
{
    if (_circleRadiusFact) {
        return _circleRadiusFact->rawValue().toFloat();
    } else {
        return 0.0;
    }
}

void APMGeoFenceManager::_circleRadiusRawValueChanged(QVariant value)
{
    emit circleRadiusChanged(value.toFloat());
}

bool APMGeoFenceManager::circleSupported(void) const
{
    if (_fenceSupported && _vehicle->multiRotor() && _fenceEnableFact && _fenceTypeFact) {
        return _fenceEnableFact->rawValue().toBool() && (_fenceTypeFact->rawValue().toInt() & 2);
    }

    return false;
}

bool APMGeoFenceManager::polygonSupported(void) const
{
    if (_fenceSupported) {
        if (_vehicle->multiRotor()) {
            if (_fenceEnableFact && _fenceTypeFact) {
                return _fenceEnableFact->rawValue().toBool() && (_fenceTypeFact->rawValue().toInt() & 4);
            }
        } else if (_vehicle->fixedWing()) {
            return true;
        }
    }

    return false;
}

QString APMGeoFenceManager::editorQml(void) const
{
    return _vehicle->multiRotor() ?
                QStringLiteral("qrc:/FirmwarePlugin/APM/CopterGeoFenceEditor.qml") :
                QStringLiteral("qrc:/FirmwarePlugin/APM/PlaneGeoFenceEditor.qml");
}
