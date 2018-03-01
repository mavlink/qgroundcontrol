/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "AirMapSharedState.h"
#include "AirspaceManager.h"
#include "QGCLoggingCategory.h"

#include <airmap/qt/logger.h>
#include <airmap/qt/types.h>

#include <memory>

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
    virtual ~AirMapManager();

    void setToolbox (QGCToolbox* toolbox) override;

    QString                         providerName                            () const override { return QString("AirMap"); }
    AirspaceVehicleManager*         instantiateVehicle                      (const Vehicle& vehicle) override;

protected:
    AirspaceRulesetsProvider*       _instantiateRulesetsProvider            () override;
    AirspaceWeatherInfoProvider*    _instatiateAirspaceWeatherInfoProvider  () override;
    AirspaceAdvisoryProvider*       _instatiateAirspaceAdvisoryProvider     () override;
    AirspaceRestrictionProvider*    _instantiateAirspaceRestrictionProvider () override;
    AirspaceFlightPlanProvider*     _instantiateAirspaceFlightPlanProvider  () override;

private slots:
    void _error             (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void _settingsChanged   ();

private:
    AirMapSharedState                               _shared;
    std::shared_ptr<airmap::qt::Logger>             _logger;
    std::shared_ptr<airmap::qt::DispatchingLogger>  _dispatchingLogger;
};


