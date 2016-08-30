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

#include "GeoFenceController.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "ParameterLoader.h"

QGC_LOGGING_CATEGORY(GeoFenceControllerLog, "GeoFenceControllerLog")

GeoFenceController::GeoFenceController(QObject* parent)
    : PlanElementController(parent)
    , _dirty(false)
{
    _clearGeoFence();
}

GeoFenceController::~GeoFenceController()
{

}

void GeoFenceController::start(bool editMode)
{
    qCDebug(GeoFenceControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);

    connect(_multiVehicleMgr,   &MultiVehicleManager::parameterReadyVehicleAvailableChanged,    this, &GeoFenceController::_parameterReadyVehicleAvailableChanged);
    connect(&_geoFence.polygon, &QGCMapPolygon::dirtyChanged,                                   this, &GeoFenceController::_polygonDirtyChanged);
}

void GeoFenceController::setFenceType(GeoFenceTypeEnum fenceType)
{
    if (_geoFence.fenceType != (GeoFenceManager::GeoFenceType_t)fenceType) {
        _geoFence.fenceType = (GeoFenceManager::GeoFenceType_t)fenceType;
        emit fenceTypeChanged(fenceType);
    }
}

void GeoFenceController::setCircleRadius(float circleRadius)
{
    if (qFuzzyCompare(_geoFence.circleRadius, circleRadius)) {
        _geoFence.circleRadius = circleRadius;
        emit circleRadiusChanged(circleRadius);
    }
}

void GeoFenceController::setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint)
{
    if (_geoFence.breachReturnPoint != breachReturnPoint) {
        _geoFence.breachReturnPoint = breachReturnPoint;
        emit breachReturnPointChanged(breachReturnPoint);
    }
}

void GeoFenceController::_setParams(void)
{
    if (_params.count() == 0 && _activeVehicle && _multiVehicleMgr->parameterReadyVehicleAvailable()) {
        QStringList skipList;
        skipList << QStringLiteral("FENCE_TOTAL") << QStringLiteral("FENCE_ENABLE");

        QStringList allNames = _activeVehicle->autopilotPlugin()->parameterNames(-1);
        foreach (const QString& paramName, allNames) {
            if (paramName.startsWith(QStringLiteral("FENCE_")) && !skipList.contains(paramName)) {
                _params << QVariant::fromValue(_activeVehicle->autopilotPlugin()->getParameterFact(-1, paramName));
            }
        }
        emit paramsChanged();
    }
}

void GeoFenceController::_activeVehicleBeingRemoved(void)
{
    _clearGeoFence();
    _params.clear();
    emit paramsChanged();
    _activeVehicle->geoFenceManager()->disconnect(this);
}

void GeoFenceController::_activeVehicleSet(void)
{
    connect(_activeVehicle->geoFenceManager(), &GeoFenceManager::newGeoFenceAvailable, this, &GeoFenceController::_newGeoFenceAvailable);

    _setParams();

    if (_activeVehicle->getParameterLoader()->parametersAreReady() && !syncInProgress()) {
        // We are switching between two previously existing vehicles. We have to manually ask for the items from the Vehicle.
        // We don't request mission items for new vehicles since that will happen autamatically.
        loadFromVehicle();
    }
}

void GeoFenceController::_parameterReadyVehicleAvailableChanged(bool parameterReadyVehicleAvailable)
{
    Q_UNUSED(parameterReadyVehicleAvailable);
    _setParams();
}

void GeoFenceController::_newGeoFenceAvailable(void)
{
    _setGeoFence(_activeVehicle->geoFenceManager()->geoFence());
    setDirty(false);
}

void GeoFenceController::loadFromFilePicker(void)
{

}

void GeoFenceController::loadFromFile(const QString& filename)
{
    Q_UNUSED(filename);
}

void GeoFenceController::saveToFilePicker(void)
{

}

void GeoFenceController::saveToFile(const QString& filename)
{
    Q_UNUSED(filename);
}

void GeoFenceController::removeAll(void)
{
    _clearGeoFence();
}

void GeoFenceController::loadFromVehicle(void)
{
    if (_activeVehicle && _activeVehicle->getParameterLoader()->parametersAreReady() && !syncInProgress()) {
        _activeVehicle->geoFenceManager()->requestGeoFence();
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call at wrong time" << _activeVehicle << _activeVehicle->getParameterLoader()->parametersAreReady() << syncInProgress();
    }
}

void GeoFenceController::sendToVehicle(void)
{
    if (_activeVehicle && _activeVehicle->getParameterLoader()->parametersAreReady() && !syncInProgress()) {
        // FIXME: Hack
        setFenceType(GeoFencePolygon);
        setDirty(false);
        _geoFence.polygon.setDirty(false);
        _activeVehicle->geoFenceManager()->setGeoFence(_geoFence);
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call at wrong time" << _activeVehicle << _activeVehicle->getParameterLoader()->parametersAreReady() << syncInProgress();
    }
}

void GeoFenceController::_clearGeoFence(void)
{
    setFenceType(GeoFenceNone);
    setCircleRadius(0.0);
    setBreachReturnPoint(QGeoCoordinate());
    _geoFence.polygon.clear();
}

void GeoFenceController::_setGeoFence(const GeoFenceManager::GeoFence_t& geoFence)
{
    _clearGeoFence();
    setFenceType(static_cast<GeoFenceTypeEnum>(geoFence.fenceType));
    setCircleRadius(geoFence.circleRadius);
    setBreachReturnPoint(geoFence.breachReturnPoint);
    _geoFence.polygon = geoFence.polygon;
}

bool GeoFenceController::syncInProgress(void) const
{
    if (_activeVehicle) {
        return _activeVehicle->geoFenceManager()->inProgress();
    } else {
        return false;
    }
}

bool GeoFenceController::dirty(void) const
{
    return _dirty | _geoFence.polygon.dirty();
}


void GeoFenceController::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        if (!dirty) {
            _geoFence.polygon.setDirty(dirty);
        }
        emit dirtyChanged(dirty);
    }
}

void GeoFenceController::_polygonDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}
