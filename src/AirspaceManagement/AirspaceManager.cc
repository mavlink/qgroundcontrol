/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "AirspaceManager.h"
#include "AirspaceWeatherInfoProvider.h"
#include "AirspaceAdvisoryProvider.h"
#include "AirspaceRestriction.h"
#include "AirspaceRulesetsProvider.h"
#include "AirspaceRestrictionProvider.h"
#include "AirspaceVehicleManager.h"
#include "AirspaceController.h"

#include "Vehicle.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(AirspaceManagementLog, "AirspaceManagementLog")

AirspaceManager::AirspaceManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _roiUpdateTimer.setInterval(2000);
    _roiUpdateTimer.setSingleShot(true);
    connect(&_roiUpdateTimer, &QTimer::timeout, this, &AirspaceManager::_updateToROI);
    //-- TODO: Move these away from QGroundControl and into their own group (Airspace)
    qmlRegisterUncreatableType<AirspaceAuthorization>       ("QGroundControl",              1, 0, "AirspaceAuthorization",          "Reference only");
    qmlRegisterUncreatableType<AirspaceController>          ("QGroundControl.Vehicle",      1, 0, "AirspaceController",             "Reference only");
    qmlRegisterUncreatableType<AirspaceWeatherInfoProvider> ("QGroundControl.Vehicle",      1, 0, "AirspaceWeatherInfoProvider",    "Reference only");
    qmlRegisterUncreatableType<AirspaceAdvisoryProvider>    ("QGroundControl.Vehicle",      1, 0, "AirspaceAdvisoryProvider",       "Reference only");
    qmlRegisterUncreatableType<AirspaceRuleFeature>         ("QGroundControl.Vehicle",      1, 0, "AirspaceRuleFeature",            "Reference only");
    qmlRegisterUncreatableType<AirspaceRule>                ("QGroundControl.Vehicle",      1, 0, "AirspaceRule",                   "Reference only");
    qmlRegisterUncreatableType<AirspaceRuleSet>             ("QGroundControl.Vehicle",      1, 0, "AirspaceRuleSet",                "Reference only");
    qmlRegisterUncreatableType<AirspaceRulesetsProvider>    ("QGroundControl.Vehicle",      1, 0, "AirspaceRulesetsProvider",       "Reference only");
    qmlRegisterUncreatableType<AirspaceRestrictionProvider> ("QGroundControl.Vehicle",      1, 0, "AirspaceRestrictionProvider",    "Reference only");
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
}

void AirspaceManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    // We should not call virtual methods in the constructor, so we instantiate the restriction provider here
    _ruleSetsProvider   = instantiateRulesetsProvider();
    _weatherProvider    = instatiateAirspaceWeatherInfoProvider();
    _advisories         = instatiateAirspaceAdvisoryProvider();
    _airspaces          = instantiateAirspaceRestrictionProvider();
}

void AirspaceManager::setROI(const QGeoCoordinate& center, double radiusMeters)
{
    _roiCenter = center;
    _roiRadius = radiusMeters;
    _roiUpdateTimer.start();
}

void AirspaceManager::_updateToROI()
{
    if(_airspaces) {
        _airspaces->setROI(_roiCenter, _roiRadius);
    }
    if(_ruleSetsProvider) {
        _ruleSetsProvider->setROI(_roiCenter);
    }
    if(_weatherProvider) {
        _weatherProvider->setROI(_roiCenter);
    }
    if (_advisories) {
        _advisories->setROI(_roiCenter, _roiRadius);
    }
}
