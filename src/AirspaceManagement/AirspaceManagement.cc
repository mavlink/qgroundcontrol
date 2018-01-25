/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "AirspaceManagement.h"
#include <Vehicle.h>

QGC_LOGGING_CATEGORY(AirspaceManagementLog, "AirspaceManagementLog")

AirspaceManager::AirspaceManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _roiUpdateTimer.setInterval(2000);
    _roiUpdateTimer.setSingleShot(true);
    connect(&_roiUpdateTimer, &QTimer::timeout, this, &AirspaceManager::_updateToROI);
    qmlRegisterUncreatableType<AirspaceAuthorization>("QGroundControl", 1, 0, "AirspaceAuthorization", "Reference only");
}

AirspaceManager::~AirspaceManager()
{
    if (_restrictionsProvider) {
        delete _restrictionsProvider;
    }
    if(_rulesetsProvider) {
        delete _rulesetsProvider;
    }
    _polygonRestrictions.clearAndDeleteContents();
    _circleRestrictions.clearAndDeleteContents();
}

void AirspaceManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    // We should not call virtual methods in the constructor, so we instantiate the restriction provider here
    _restrictionsProvider = instantiateRestrictionProvider();
    if (_restrictionsProvider) {
        connect(_restrictionsProvider, &AirspaceRestrictionProvider::requestDone, this,   &AirspaceManager::_restrictionsUpdated);
    }
    _rulesetsProvider = instantiateRulesetsProvider();
    if (_rulesetsProvider) {
        connect(_rulesetsProvider, &AirspaceRulesetsProvider::requestDone, this,  &AirspaceManager::_rulessetsUpdated);
    }
    _weatherProvider = instatiateAirspaceWeatherInfoProvider();
}

void AirspaceManager::setROI(const QGeoCoordinate& center, double radiusMeters)
{
    _roiCenter = center;
    _roiRadius = radiusMeters;
    _roiUpdateTimer.start();
}

void AirspaceManager::_updateToROI()
{
    if (_restrictionsProvider) {
        _restrictionsProvider->setROI(_roiCenter, _roiRadius);
    }
    if(_rulesetsProvider) {
        _rulesetsProvider->setROI(_roiCenter);
    }
    if(_weatherProvider) {
        _weatherProvider->setROI(_roiCenter);
    }
}

void AirspaceManager::_restrictionsUpdated(bool success)
{
    _polygonRestrictions.clearAndDeleteContents();
    _circleRestrictions.clearAndDeleteContents();
    if (success) {
        for (const auto& circle : _restrictionsProvider->circles()) {
            _circleRestrictions.append(circle);
        }
        for (const auto& polygon : _restrictionsProvider->polygons()) {
            _polygonRestrictions.append(polygon);
        }
    } else {
        // TODO: show error?
    }
}

void AirspaceManager::_rulessetsUpdated(bool)
{

}

AirspaceVehicleManager::AirspaceVehicleManager(const Vehicle& vehicle)
    : _vehicle(vehicle)
{
    connect(&_vehicle, &Vehicle::armedChanged, this, &AirspaceVehicleManager::_vehicleArmedChanged);
    connect(&_vehicle, &Vehicle::mavlinkMessageReceived, this, &AirspaceVehicleManager::vehicleMavlinkMessageReceived);
}

void AirspaceVehicleManager::_vehicleArmedChanged(bool armed)
{
    if (armed) {
        startTelemetryStream();
        _vehicleWasInMissionMode = _vehicle.flightMode() == _vehicle.missionFlightMode();
    } else {
        stopTelemetryStream();
        // end the flight if we were in mission mode during arming or disarming
        // TODO: needs to be improved. for instance if we do RTL and then want to continue the mission...
        if (_vehicleWasInMissionMode || _vehicle.flightMode() == _vehicle.missionFlightMode()) {
            endFlight();
        }
    }
}

