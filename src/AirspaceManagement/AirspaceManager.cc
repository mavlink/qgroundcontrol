/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "AirspaceAdvisoryProvider.h"
#include "AirspaceFlightPlanProvider.h"
#include "AirspaceManager.h"
#include "AirspaceRestriction.h"
#include "AirspaceRestrictionProvider.h"
#include "AirspaceRulesetsProvider.h"
#include "AirspaceVehicleManager.h"
#include "AirspaceWeatherInfoProvider.h"

#include "Vehicle.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(AirspaceManagementLog, "AirspaceManagementLog")

AirspaceManager::AirspaceManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _airspaceVisible(false)
{
    _roiUpdateTimer.setInterval(2000);
    _roiUpdateTimer.setSingleShot(true);
    connect(&_roiUpdateTimer, &QTimer::timeout, this, &AirspaceManager::_updateToROI);
    qmlRegisterUncreatableType<AirspaceAdvisoryProvider>    ("QGroundControl.Airspace",      1, 0, "AirspaceAdvisoryProvider",       "Reference only");
    qmlRegisterUncreatableType<AirspaceFlightPlanProvider>  ("QGroundControl.Airspace",      1, 0, "AirspaceFlightPlanProvider",     "Reference only");
    qmlRegisterUncreatableType<AirspaceManager>             ("QGroundControl.Airspace",      1, 0, "AirspaceManager",                "Reference only");
    qmlRegisterUncreatableType<AirspaceRestrictionProvider> ("QGroundControl.Airspace",      1, 0, "AirspaceRestrictionProvider",    "Reference only");
    qmlRegisterUncreatableType<AirspaceRule>                ("QGroundControl.Airspace",      1, 0, "AirspaceRule",                   "Reference only");
    qmlRegisterUncreatableType<AirspaceRuleFeature>         ("QGroundControl.Airspace",      1, 0, "AirspaceRuleFeature",            "Reference only");
    qmlRegisterUncreatableType<AirspaceRuleSet>             ("QGroundControl.Airspace",      1, 0, "AirspaceRuleSet",                "Reference only");
    qmlRegisterUncreatableType<AirspaceRulesetsProvider>    ("QGroundControl.Airspace",      1, 0, "AirspaceRulesetsProvider",       "Reference only");
    qmlRegisterUncreatableType<AirspaceWeatherInfoProvider> ("QGroundControl.Airspace",      1, 0, "AirspaceWeatherInfoProvider",    "Reference only");
}

AirspaceManager::~AirspaceManager()
{
    if(_advisories) {
        delete _advisories;
    }
    if(_weatherProvider) {
        delete _weatherProvider;
    }
    if(_ruleSetsProvider) {
        delete _ruleSetsProvider;
    }
    if(_airspaces) {
        delete _airspaces;
    }
    if(_flightPlan) {
        delete _flightPlan;
    }
}

void AirspaceManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    // We should not call virtual methods in the constructor, so we instantiate the restriction provider here
    _ruleSetsProvider   = _instantiateRulesetsProvider();
    _weatherProvider    = _instatiateAirspaceWeatherInfoProvider();
    _advisories         = _instatiateAirspaceAdvisoryProvider();
    _airspaces          = _instantiateAirspaceRestrictionProvider();
    _flightPlan         = _instantiateAirspaceFlightPlanProvider();
}

void AirspaceManager::setROI(const QGeoCoordinate& pointNW, const QGeoCoordinate& pointSE)
{
    _setROI(QGCGeoBoundingCube(pointNW, pointSE));
}

void AirspaceManager::_setROI(const QGCGeoBoundingCube& roi)
{
    _roi = roi;
    _roiUpdateTimer.start();
}

void AirspaceManager::_updateToROI()
{
    if(_airspaces) {
        _airspaces->setROI(_roi);
    }
    if(_ruleSetsProvider) {
        _ruleSetsProvider->setROI(_roi);
    }
    if(_weatherProvider) {
        _weatherProvider->setROI(_roi);
    }
    if (_advisories) {
        _advisories->setROI(_roi);
    }
}
