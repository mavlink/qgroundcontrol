/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "AirMapSharedState.h"
#include "AirspaceManager.h"
#include "QGCLoggingCategory.h"

#include <Airmap/services/logger.h>
#include <Airmap/services/types.h>

#include <memory>

#include <QTimer>

class QGCToolbox;

Q_DECLARE_LOGGING_CATEGORY(AirMapManagerLog)

//-----------------------------------------------------------------------------
/**
 * @class AirMapManager
 * AirMap implementation of AirspaceManager
 */

class AirMapManager : public AirspaceManager
{
    Q_OBJECT
public:
    AirMapManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~AirMapManager() override;

    void setToolbox (QGCToolbox* toolbox) override;

    QString                         providerName                            () const override { return QString("AirMap"); }
    AirspaceVehicleManager*         instantiateVehicle                      (const Vehicle& vehicle) override;
    bool                            connected                               () const override;
    QString                         connectStatus                           () const override { return _connectStatus; }
    AirspaceManager::AuthStatus     authStatus                              () const override { return _authStatus; }

protected:
    AirspaceRulesetsProvider*       _instantiateRulesetsProvider            () override;
    AirspaceWeatherInfoProvider*    _instatiateAirspaceWeatherInfoProvider  () override;
    AirspaceAdvisoryProvider*       _instatiateAirspaceAdvisoryProvider     () override;
    AirspaceRestrictionProvider*    _instantiateAirspaceRestrictionProvider () override;
    AirspaceFlightPlanProvider*     _instantiateAirspaceFlightPlanProvider  () override;

private slots:
    void _error             (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void _settingsChanged   ();
    void _settingsTimeout   ();
    void _airspaceEnabled   ();
    void _authStatusChanged (AirspaceManager::AuthStatus status);

private:
    QString                                                 _connectStatus;
    QTimer                                                  _settingsTimer;
    AirMapSharedState                                       _shared;
    std::shared_ptr<airmap::services::Logger>               _logger;
    std::shared_ptr<airmap::services::DispatchingLogger>    _dispatchingLogger; 
    AirspaceManager::AuthStatus                             _authStatus;
    const std::string                                       _telemetryHost = "telemetry.airmap.com";
    const uint16_t                                          _telemetryPort = 16060;
};


