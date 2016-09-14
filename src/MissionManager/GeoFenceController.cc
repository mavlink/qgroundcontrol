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

}

GeoFenceController::~GeoFenceController()
{

}

void GeoFenceController::start(bool editMode)
{
    qCDebug(GeoFenceControllerLog) << "start editMode" << editMode;

    PlanElementController::start(editMode);

    connect(&_polygon, &QGCMapPolygon::dirtyChanged, this, &GeoFenceController::_polygonDirtyChanged);
}

void GeoFenceController::setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint)
{
    if (_breachReturnPoint != breachReturnPoint) {
        _breachReturnPoint = breachReturnPoint;
        setDirty(true);
        emit breachReturnPointChanged(breachReturnPoint);
    }
}

void GeoFenceController::_signalAll(void)
{
    emit fenceSupportedChanged(fenceSupported());
    emit circleSupportedChanged(circleSupported());
    emit polygonSupportedChanged(polygonSupported());
    emit breachReturnSupportedChanged(breachReturnSupported());
    emit circleRadiusChanged(circleRadius());
    emit paramsChanged(params());
    emit paramLabelsChanged(paramLabels());
    emit editorQmlChanged(editorQml());
}

void GeoFenceController::_activeVehicleBeingRemoved(void)
{
    _clearGeoFence();
    _signalAll();
    _activeVehicle->geoFenceManager()->disconnect(this);
}

void GeoFenceController::_activeVehicleSet(void)
{
    GeoFenceManager* geoFenceManager = _activeVehicle->geoFenceManager();
    connect(geoFenceManager, &GeoFenceManager::circleSupportedChanged,          this, &GeoFenceController::_setDirty);
    connect(geoFenceManager, &GeoFenceManager::polygonSupportedChanged,         this, &GeoFenceController::_setDirty);
    connect(geoFenceManager, &GeoFenceManager::fenceSupportedChanged,           this, &GeoFenceController::fenceSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::circleSupportedChanged,          this, &GeoFenceController::circleSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::polygonSupportedChanged,         this, &GeoFenceController::polygonSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::breachReturnSupportedChanged,    this, &GeoFenceController::breachReturnSupportedChanged);
    connect(geoFenceManager, &GeoFenceManager::circleRadiusChanged,             this, &GeoFenceController::circleRadiusChanged);
    connect(geoFenceManager, &GeoFenceManager::polygonChanged,                  this, &GeoFenceController::_setPolygon);
    connect(geoFenceManager, &GeoFenceManager::breachReturnPointChanged,        this, &GeoFenceController::setBreachReturnPoint);
    connect(geoFenceManager, &GeoFenceManager::paramsChanged,                   this, &GeoFenceController::paramsChanged);
    connect(geoFenceManager, &GeoFenceManager::paramLabelsChanged,              this, &GeoFenceController::paramLabelsChanged);

    _signalAll();

    if (_activeVehicle->getParameterLoader()->parametersAreReady()) {
        if (!syncInProgress()) {
            // We are switching between two previously existing vehicles. We have to manually ask for the items from the Vehicle.
            // We don't request mission items for new vehicles since that will happen autamatically.
            loadFromVehicle();
        }
    }
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
    if (_activeVehicle->getParameterLoader()->parametersAreReady() && !syncInProgress()) {
        _activeVehicle->geoFenceManager()->loadFromVehicle();
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call at wrong time" << _activeVehicle->getParameterLoader()->parametersAreReady() << syncInProgress();
    }
}

void GeoFenceController::sendToVehicle(void)
{
    if (_activeVehicle->getParameterLoader()->parametersAreReady() && !syncInProgress()) {
        _activeVehicle->geoFenceManager()->setPolygon(polygon());
        _activeVehicle->geoFenceManager()->setBreachReturnPoint(breachReturnPoint());
        setDirty(false);
        _polygon.setDirty(false);
        _activeVehicle->geoFenceManager()->sendToVehicle();
    } else {
        qCWarning(GeoFenceControllerLog) << "GeoFenceController::loadFromVehicle call at wrong time" << _activeVehicle->getParameterLoader()->parametersAreReady() << syncInProgress();
    }
}

void GeoFenceController::_clearGeoFence(void)
{
    setBreachReturnPoint(QGeoCoordinate());
    _polygon.clear();
}

bool GeoFenceController::syncInProgress(void) const
{
    return _activeVehicle->geoFenceManager()->inProgress();
}

bool GeoFenceController::dirty(void) const
{
    return _dirty | _polygon.dirty();
}


void GeoFenceController::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        if (!dirty) {
            _polygon.setDirty(dirty);
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

bool GeoFenceController::fenceSupported(void) const
{
    return _activeVehicle->geoFenceManager()->fenceSupported();
}

bool GeoFenceController::circleSupported(void) const
{
    return _activeVehicle->geoFenceManager()->circleSupported();
}

bool GeoFenceController::polygonSupported(void) const
{
    return _activeVehicle->geoFenceManager()->polygonSupported();
}

bool GeoFenceController::breachReturnSupported(void) const
{
    return _activeVehicle->geoFenceManager()->breachReturnSupported();
}

void GeoFenceController::_setDirty(void)
{
    setDirty(true);
}

void GeoFenceController::_setPolygon(const QList<QGeoCoordinate>& polygon)
{
    _polygon.setPath(polygon);
    // This is coming from a GeoFenceManager::loadFromVehicle call
    setDirty(false);
}

float GeoFenceController::circleRadius(void) const
{
    return _activeVehicle->geoFenceManager()->circleRadius();
}

QVariantList GeoFenceController::params(void) const
{
    return _activeVehicle->geoFenceManager()->params();
}

QStringList GeoFenceController::paramLabels(void) const
{
    return _activeVehicle->geoFenceManager()->paramLabels();
}

QString GeoFenceController::editorQml(void) const
{
    return _activeVehicle->geoFenceManager()->editorQml();
}
