/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    , _authStatus(Unknown)
{
    _logger = std::make_shared<qt::Logger>();
    qt::register_types(); // TODO: still needed?
    _logger->logging_category().setEnabled(QtDebugMsg,   false);
    _logger->logging_category().setEnabled(QtInfoMsg,    false);
    _logger->logging_category().setEnabled(QtWarningMsg, false);
    _dispatchingLogger = std::make_shared<qt::DispatchingLogger>(_logger);
    connect(&_shared, &AirMapSharedState::error, this, &AirMapManager::_error);
    connect(&_shared, &AirMapSharedState::authStatus, this, &AirMapManager::_authStatusChanged);
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
    _settingsTimer.setSingleShot(true);
    AirspaceManager::setToolbox(toolbox);
    AirMapSettings* ap = toolbox->settingsManager()->airMapSettings();
    connect(ap->enableAirMap(),     &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->usePersonalApiKey(),&Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->apiKey(),           &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->clientID(),         &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->userName(),         &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->password(),         &Fact::rawValueChanged, this, &AirMapManager::_settingsChanged);
    connect(ap->enableAirspace(),   &Fact::rawValueChanged, this, &AirMapManager::_airspaceEnabled);
    connect(&_settingsTimer,        &QTimer::timeout,       this, &AirMapManager::_settingsTimeout);
    _settingsTimeout();
}

//-----------------------------------------------------------------------------
bool
AirMapManager::connected() const
{
    return _shared.client() != nullptr;
}

//-----------------------------------------------------------------------------
void
AirMapManager::_error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails)
{
    qCDebug(AirMapManagerLog) << "Error: "<<what<<", msg: "<<airmapdMessage<<", details: "<<airmapdDetails;
    qgcApp()->showMessage(QString("Error: %1. %2").arg(what).arg(airmapdMessage));
}

//-----------------------------------------------------------------------------
void
AirMapManager::_authStatusChanged(AirspaceManager::AuthStatus status)
{
    _authStatus = status;
    emit authStatusChanged();
}

//-----------------------------------------------------------------------------
void
AirMapManager::_settingsChanged()
{
    _settingsTimer.start(1000);
}

//-----------------------------------------------------------------------------
void
AirMapManager::_airspaceEnabled()
{
    if(qgcApp()->toolbox()->settingsManager()->airMapSettings()->enableAirspace()->rawValue().toBool()) {
        if(_airspaces) {
            _airspaces->setROI(_roi, true);
        }
    }
}

//-----------------------------------------------------------------------------
void
AirMapManager::_settingsTimeout()
{
    qCDebug(AirMapManagerLog) << "AirMap settings changed";
    _connectStatus.clear();
    emit connectStatusChanged();
    AirMapSettings* ap = _toolbox->settingsManager()->airMapSettings();
    //-- If we are disabled, there is nothing else to do.
    if (!ap->enableAirMap()->rawValue().toBool()) {
        _shared.logout();
        if(_shared.client()) {
            delete _shared.client();
            _shared.setClient(nullptr);
            emit connectedChanged();
        }
        return;
    }
    AirMapSharedState::Settings settings;
    if(ap->usePersonalApiKey()->rawValue().toBool()) {
        settings.apiKey     = ap->apiKey()->rawValueString();
        settings.clientID   = ap->clientID()->rawValueString();
    }
    //-- If we have a hardwired key (and no custom key is present), set it.
#if defined(QGC_AIRMAP_KEY_AVAILABLE)
    if(!ap->usePersonalApiKey()->rawValue().toBool()) {
        settings.apiKey     = AirmapAPIKey();
        settings.clientID   = AirmapClientID();
    }
    bool authChanged = settings.apiKey != _shared.settings().apiKey || settings.apiKey.isEmpty();
#else
    bool authChanged = settings.apiKey != _shared.settings().apiKey;
#endif
    settings.userName = ap->userName()->rawValueString();
    settings.password = ap->password()->rawValueString();
    if(settings.userName != _shared.settings().userName || settings.password != _shared.settings().password) {
        authChanged = true;
    }
    _shared.setSettings(settings);
    //-- Need to re-create the client if the API key or user name/password changed
    if ((_shared.client() && authChanged) || !ap->enableAirMap()->rawValue().toBool()) {
        delete _shared.client();
        _shared.setClient(nullptr);
        emit connectedChanged();
    }
    if (!_shared.client() && settings.apiKey != "") {
        qCDebug(AirMapManagerLog) << "Creating AirMap client";
        auto credentials    = Credentials{};
        credentials.api_key = _shared.settings().apiKey.toStdString();
        auto configuration  = Client::default_production_configuration(credentials);
        configuration.telemetry.host = "udp.telemetry.k8s.airmap.io";
        configuration.telemetry.port = 7070;
        qt::Client::create(configuration, _dispatchingLogger, this, [this](const qt::Client::CreateResult& result) {
            if (result) {
                qCDebug(AirMapManagerLog) << "Successfully created airmap::qt::Client instance";
                _shared.setClient(result.value());
                emit connectedChanged();
                _connectStatus = tr("AirMap Enabled");
                emit connectStatusChanged();
                //-- Now test authentication
                _shared.login();
            } else {
                qWarning("Failed to create airmap::qt::Client instance");
                QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
                QString error = QString::fromStdString(result.error().message());
                _error(tr("Failed to create airmap::qt::Client instance"),
                        error, description);
                _connectStatus = error;
                if(!description.isEmpty()) {
                    _connectStatus += "\n";
                    _connectStatus += description;
                }
                emit connectStatusChanged();
            }
        });
    } else {
        if(settings.apiKey == "") {
            _connectStatus = tr("No API key for AirMap");
            emit connectStatusChanged();
            qCDebug(AirMapManagerLog) << _connectStatus;
        }
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
