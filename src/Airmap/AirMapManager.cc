/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapAdvisoryManager.h"
#include "AirMapFlightPlanManager.h"
#include "AirMapManager.h"
#include "AirMapRestrictionManager.h"
#include "AirMapRulesetsManager.h"
#include "AirMapSettings.h"
#include "AirMapTelemetry.h"
#include "AirMapTrafficMonitor.h"
#include "AirMapVehicleManager.h"
#include "AirMapWeatherInfoManager.h"

#include "QmlObjectListModel.h"
#include "JsonHelper.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"
#include "QGCApplication.h"

#include <airmap/authenticator.h>

//-- Hardwired API key
#if defined(QGC_AIRMAP_KEY_AVAILABLE)
#include "Airmap_api_key.h"
#endif

using namespace airmap;

QGC_LOGGING_CATEGORY(AirMapManagerLog, "AirMapManagerLog")

//-----------------------------------------------------------------------------
AirMapManager::AirMapManager(QGCApplication* app, QGCToolbox* toolbox)
    : AirspaceManager(app, toolbox)
{
    _logger = std::make_shared<qt::Logger>();
    qt::register_types(); // TODO: still needed?
    _logger->logging_category().setEnabled(QtDebugMsg,   true);
    _logger->logging_category().setEnabled(QtInfoMsg,    true);
    _logger->logging_category().setEnabled(QtWarningMsg, false);
    _dispatchingLogger = std::make_shared<qt::DispatchingLogger>(_logger);
    connect(&_shared, &AirMapSharedState::error, this, &AirMapManager::_error);
}

//-----------------------------------------------------------------------------
AirMapManager::~AirMapManager()
{
    if (_shared.client()) {
        delete _shared.client();
    }
}

//-----------------------------------------------------------------------------
void
AirMapManager::setToolbox(QGCToolbox* toolbox)
{
    AirspaceManager::setToolbox(toolbox);
    AirMapSettings* ap = toolbox->settingsManager()->airMapSettings();
    connect(ap->apiKey(),    &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->clientID(),  &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->userName(),  &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->password(),  &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    _settingsChanged();
}

//-----------------------------------------------------------------------------
void
AirMapManager::_error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails)
{
    qCDebug(AirMapManagerLog) << "Error: "<<what<<", msg: "<<airmapdMessage<<", details: "<<airmapdDetails;
    qgcApp()->showMessage(QString("AirMap Error: %1. %2").arg(what).arg(airmapdMessage));
}

//-----------------------------------------------------------------------------
void
AirMapManager::_settingsChanged()
{
    qCDebug(AirMapManagerLog) << "AirMap settings changed";
    AirMapSettings* ap = _toolbox->settingsManager()->airMapSettings();
    AirMapSharedState::Settings settings;
    settings.apiKey = ap->apiKey()->rawValueString();
#if defined(QGC_AIRMAP_KEY_AVAILABLE)
    bool apiKeyChanged = settings.apiKey != _shared.settings().apiKey || settings.apiKey.isEmpty();
#else
    bool apiKeyChanged = settings.apiKey != _shared.settings().apiKey;
#endif
    settings.clientID = ap->clientID()->rawValueString();
    settings.userName = ap->userName()->rawValueString();
    settings.password = ap->password()->rawValueString();
    //-- If we have a hardwired key (and no custom key), set it.
#if defined(QGC_AIRMAP_KEY_AVAILABLE)
    if(settings.apiKey.isEmpty() || settings.clientID.isEmpty()) {
        settings.apiKey     = kAirmapAPIKey;
        settings.clientID   = kAirmapClientID;
    }
#endif
    _shared.setSettings(settings);
    //-- Need to re-create the client if the API key changed
    if (_shared.client() && apiKeyChanged) {
        delete _shared.client();
        _shared.setClient(nullptr);
    }
    if (!_shared.client() && settings.apiKey != "") {
        qCDebug(AirMapManagerLog) << "Creating AirMap client";
        auto credentials    = Credentials{};
        credentials.api_key = _shared.settings().apiKey.toStdString();
        auto configuration  = Client::default_production_configuration(credentials);
        qt::Client::create(configuration, _dispatchingLogger, this, [this, ap](const qt::Client::CreateResult& result) {
            if (result) {
                qCDebug(AirMapManagerLog) << "Successfully created airmap::qt::Client instance";
                _shared.setClient(result.value());
            } else {
                qWarning("Failed to create airmap::qt::Client instance");
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                _error("Failed to create airmap::qt::Client instance",
                        QString::fromStdString(result.error().message()), description);
            }
        });
    }
}

//-----------------------------------------------------------------------------
AirspaceVehicleManager*
AirMapManager::instantiateVehicle(const Vehicle& vehicle)
{
    AirMapVehicleManager* manager = new AirMapVehicleManager(_shared, vehicle);
    connect(manager, &AirMapVehicleManager::error, this, &AirMapManager::_error);
    return manager;
}

//-----------------------------------------------------------------------------
AirspaceRulesetsProvider*
AirMapManager::_instantiateRulesetsProvider()
{
    AirMapRulesetsManager* rulesetsManager = new AirMapRulesetsManager(_shared);
    connect(rulesetsManager, &AirMapRulesetsManager::error, this, &AirMapManager::_error);
    return rulesetsManager;
}

//-----------------------------------------------------------------------------
AirspaceWeatherInfoProvider*
AirMapManager::_instatiateAirspaceWeatherInfoProvider()
{
    AirMapWeatherInfoManager* weatherInfo = new AirMapWeatherInfoManager(_shared);
    connect(weatherInfo, &AirMapWeatherInfoManager::error, this, &AirMapManager::_error);
    return weatherInfo;
}

//-----------------------------------------------------------------------------
AirspaceAdvisoryProvider*
AirMapManager::_instatiateAirspaceAdvisoryProvider()
{
    AirMapAdvisoryManager* advisories = new AirMapAdvisoryManager(_shared);
    connect(advisories, &AirMapAdvisoryManager::error, this, &AirMapManager::_error);
    return advisories;
}

//-----------------------------------------------------------------------------
AirspaceRestrictionProvider*
AirMapManager::_instantiateAirspaceRestrictionProvider()
{
    AirMapRestrictionManager* airspaces = new AirMapRestrictionManager(_shared);
    connect(airspaces, &AirMapRestrictionManager::error, this, &AirMapManager::_error);
    return airspaces;
}

//-----------------------------------------------------------------------------
AirspaceFlightPlanProvider*
AirMapManager::_instantiateAirspaceFlightPlanProvider()
{
    AirMapFlightPlanManager* flightPlan = new AirMapFlightPlanManager(_shared);
    connect(flightPlan, &AirMapFlightPlanManager::error, this, &AirMapManager::_error);
    return flightPlan;
}
