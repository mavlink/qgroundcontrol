/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirMapAdvisories.h"
#include "AirMapWeatherInformation.h"
#include "AirMapRestrictionManager.h"
#include "AirMapRulesetsManager.h"
#include "AirMapSettings.h"
#include "AirMapTelemetry.h"
#include "AirMapTrafficMonitor.h"
#include "AirMapVehicleManager.h"

#include "QmlObjectListModel.h"
#include "JsonHelper.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"
#include "QGCApplication.h"

#include <airmap/authenticator.h>

using namespace airmap;

QGC_LOGGING_CATEGORY(AirMapManagerLog, "AirMapManagerLog")

AirMapManager::AirMapManager(QGCApplication* app, QGCToolbox* toolbox)
    : AirspaceManager(app, toolbox)
{
    _logger = std::make_shared<qt::Logger>();
    qt::register_types(); // TODO: still needed?s
    _logger->logging_category().setEnabled(QtDebugMsg, true);
    _logger->logging_category().setEnabled(QtInfoMsg,  true);
    _logger->logging_category().setEnabled(QtWarningMsg, true);
    _dispatchingLogger = std::make_shared<qt::DispatchingLogger>(_logger);
    connect(&_shared, &AirMapSharedState::error, this, &AirMapManager::_error);
}

AirMapManager::~AirMapManager()
{
    if (_shared.client()) {
        delete _shared.client();
    }
}

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

void
AirMapManager::_error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails)
{
    qCDebug(AirMapManagerLog) << "Error: "<<what<<", msg: "<<airmapdMessage<<", details: "<<airmapdDetails;
    qgcApp()->showMessage(QString("AirMap Error: %1. %2").arg(what).arg(airmapdMessage));
}

void
AirMapManager::_settingsChanged()
{
    qCDebug(AirMapManagerLog) << "AirMap settings changed";
    AirMapSettings* ap = _toolbox->settingsManager()->airMapSettings();
    AirMapSharedState::Settings settings;
    settings.apiKey = ap->apiKey()->rawValueString();
    bool apiKeyChanged = settings.apiKey != _shared.settings().apiKey;
    settings.clientID = ap->clientID()->rawValueString();
    settings.userName = ap->userName()->rawValueString();
    settings.password = ap->password()->rawValueString();
    _shared.setSettings(settings);
    // need to re-create the client if the API key changed
    if (_shared.client() && apiKeyChanged) {
        delete _shared.client();
        _shared.setClient(nullptr);
    }
    if (!_shared.client() && settings.apiKey != "") {
        qCDebug(AirMapManagerLog) << "Creating AirMap client";
        auto credentials    = Credentials{};
        credentials.api_key = _shared.settings().apiKey.toStdString();
        auto configuration  = Client::default_staging_configuration(credentials);
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

AirspaceVehicleManager*
AirMapManager::instantiateVehicle(const Vehicle& vehicle)
{
    AirMapVehicleManager* manager = new AirMapVehicleManager(_shared, vehicle, *_toolbox);
    connect(manager, &AirMapVehicleManager::error, this, &AirMapManager::_error);
    return manager;
}

AirspaceRestrictionProvider*
AirMapManager::instantiateRestrictionProvider()
{
    AirMapRestrictionManager* restrictionManager = new AirMapRestrictionManager(_shared);
    connect(restrictionManager, &AirMapRestrictionManager::error, this, &AirMapManager::_error);
    return restrictionManager;
}

AirspaceRulesetsProvider*
AirMapManager::instantiateRulesetsProvider()
{
    AirMapRulesetsManager* rulesetsManager = new AirMapRulesetsManager(_shared);
    connect(rulesetsManager, &AirMapRulesetsManager::error, this, &AirMapManager::_error);
    return rulesetsManager;
}

AirspaceWeatherInfoProvider*
AirMapManager::instatiateAirspaceWeatherInfoProvider()
{
    AirMapWeatherInformation* weatherInfo = new AirMapWeatherInformation(_shared);
    connect(weatherInfo, &AirMapWeatherInformation::error, this, &AirMapManager::_error);
    return weatherInfo;
}

AirspaceAdvisoryProvider*
AirMapManager::instatiateAirspaceAdvisoryProvider()
{
    AirMapAdvisories* advisories = new AirMapAdvisories(_shared);
    connect(advisories, &AirMapAdvisories::error, this, &AirMapManager::_error);
    return advisories;
}
