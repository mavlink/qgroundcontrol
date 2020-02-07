/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

//-----------------------------------------------------------------------------
AirspaceManager::AirspaceManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    _ruleUpdateTimer.setInterval(2000);
    _ruleUpdateTimer.setSingleShot(true);
    _updateTimer.setInterval(1000);
    _updateTimer.setSingleShot(true);
    connect(&_ruleUpdateTimer, &QTimer::timeout, this, &AirspaceManager::_updateRulesTimeout);
    connect(&_updateTimer,     &QTimer::timeout, this, &AirspaceManager::_updateTimeout);
    qmlRegisterUncreatableType<AirspaceAdvisoryProvider>    ("QGroundControl.Airspace",      1, 0, "AirspaceAdvisoryProvider",       "Reference only");
    qmlRegisterUncreatableType<AirspaceFlightPlanProvider>  ("QGroundControl.Airspace",      1, 0, "AirspaceFlightPlanProvider",     "Reference only");
    qmlRegisterUncreatableType<AirspaceManager>             ("QGroundControl.Airspace",      1, 0, "AirspaceManager",                "Reference only");
    qmlRegisterUncreatableType<AirspaceRestrictionProvider> ("QGroundControl.Airspace",      1, 0, "AirspaceRestrictionProvider",    "Reference only");
    qmlRegisterUncreatableType<AirspaceRule>                ("QGroundControl.Airspace",      1, 0, "AirspaceRule",                   "Reference only");
    qmlRegisterUncreatableType<AirspaceRuleFeature>         ("QGroundControl.Airspace",      1, 0, "AirspaceRuleFeature",            "Reference only");
    qmlRegisterUncreatableType<AirspaceRuleSet>             ("QGroundControl.Airspace",      1, 0, "AirspaceRuleSet",                "Reference only");
    qmlRegisterUncreatableType<AirspaceRulesetsProvider>    ("QGroundControl.Airspace",      1, 0, "AirspaceRulesetsProvider",       "Reference only");
    qmlRegisterUncreatableType<AirspaceWeatherInfoProvider> ("QGroundControl.Airspace",      1, 0, "AirspaceWeatherInfoProvider",    "Reference only");
    qmlRegisterUncreatableType<AirspaceFlightAuthorization> ("QGroundControl.Airspace",      1, 0, "AirspaceFlightAuthorization",    "Reference only");
    qmlRegisterUncreatableType<AirspaceFlightInfo>          ("QGroundControl.Airspace",      1, 0, "AirspaceFlightInfo",             "Reference only");
}

//-----------------------------------------------------------------------------
AirspaceManager::~AirspaceManager()
{
    delete _advisories;
    _advisories = nullptr;
    delete _weatherProvider;
    _weatherProvider = nullptr;
    delete _ruleSetsProvider;
    _ruleSetsProvider = nullptr;
    delete _airspaces;
    _airspaces = nullptr;
    delete _flightPlan;
    _flightPlan = nullptr;
}

//-----------------------------------------------------------------------------
void
AirspaceManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    // We should not call virtual methods in the constructor, so we instantiate the restriction provider here
    _ruleSetsProvider   = _instantiateRulesetsProvider();
    _weatherProvider    = _instatiateAirspaceWeatherInfoProvider();
    _advisories         = _instatiateAirspaceAdvisoryProvider();
    _airspaces          = _instantiateAirspaceRestrictionProvider();
    _flightPlan         = _instantiateAirspaceFlightPlanProvider();
    //-- Keep track of rule changes
    if(_ruleSetsProvider) {
        connect(_ruleSetsProvider, &AirspaceRulesetsProvider::selectedRuleSetsChanged, this, &AirspaceManager::_rulesChanged);
    }
}

//-----------------------------------------------------------------------------
void
AirspaceManager::setROI(const QGeoCoordinate& pointNW, const QGeoCoordinate& pointSE, bool planView, bool reset)
{
    if(planView) {
        //-- Is there a mission?
        if(_flightPlan->flightPermitStatus() != AirspaceFlightPlanProvider::PermitNone) {
            //-- Is there a polygon to work with?
            if(_flightPlan->missionArea()->isValid() && _flightPlan->missionArea()->area() > 0.0) {
                if(reset) {
                    _roi = *_flightPlan->missionArea();
                    _updateToROI(true);
                } else {
                    _setROI(*_flightPlan->missionArea());
                }
                return;
            }
        }
    }
    //-- Use screen coordinates (what you see is what you get)
    if(reset) {
        _roi = QGCGeoBoundingCube(pointNW, pointSE);
        _updateToROI(true);
    } else {
        _setROI(QGCGeoBoundingCube(pointNW, pointSE));
    }
}

//-----------------------------------------------------------------------------
void
AirspaceManager::_setROI(const QGCGeoBoundingCube& roi)
{
    if(_roi != roi) {
        _roi = roi;
        _updateTimer.start();
    }
}

//-----------------------------------------------------------------------------
void
AirspaceManager::_updateToROI(bool reset)
{
    if(_airspaces) {
        _airspaces->setROI(_roi, reset);
    }
    if(_ruleSetsProvider) {
        _ruleSetsProvider->setROI(_roi, reset);
    }
    if(_weatherProvider) {
        _weatherProvider->setROI(_roi, reset);
    }
}


//-----------------------------------------------------------------------------
void
AirspaceManager::_updateTimeout()
{
    _updateToROI(false);
}

//-----------------------------------------------------------------------------
void
AirspaceManager::_rulesChanged()
{
    _ruleUpdateTimer.start();
}

//-----------------------------------------------------------------------------
void
AirspaceManager::_updateRulesTimeout()
{
    if (_advisories) {
        _advisories->setROI(_roi, true);
    }
}
