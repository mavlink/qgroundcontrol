/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "AirspaceManagement.h"
#include "AirMapSharedState.h"

#include <airmap/qt/logger.h>
#include <airmap/qt/types.h>

Q_DECLARE_LOGGING_CATEGORY(AirMapManagerLog)

//-----------------------------------------------------------------------------
/**
 * @class LifetimeChecker
 * Base class which helps to check if an object instance still exists.
 * A subclass can take a weak pointer from _instance and then check if the object was deleted.
 * This is used in callbacks that access 'this', but the instance might already be deleted (e.g. vehicle disconnect).
 */
class LifetimeChecker
{
public:
    LifetimeChecker() : _instance(this, [](void*){}) { }
    virtual ~LifetimeChecker() = default;

protected:
    std::shared_ptr<LifetimeChecker> _instance;
};

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

    AirspaceVehicleManager*         instantiateVehicle                      (const Vehicle& vehicle) override;
    AirspaceRestrictionProvider*    instantiateRestrictionProvider          () override;
    AirspaceRulesetsProvider*       instantiateRulesetsProvider             () override;
    AirspaceWeatherInfoProvider*    instatiateAirspaceWeatherInfoProvider   () override;

    QString name            () const override { return "AirMap"; }

private slots:
    void _error             (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void _settingsChanged   ();

private:
    AirMapSharedState                               _shared;
    std::shared_ptr<airmap::qt::Logger>             _logger;
    std::shared_ptr<airmap::qt::DispatchingLogger>  _dispatchingLogger;
};


