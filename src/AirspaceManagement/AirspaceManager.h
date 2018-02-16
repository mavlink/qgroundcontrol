/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @file AirspaceManager.h
 * This file contains the interface definitions used by an airspace management implementation (AirMap).
 * There are 3 base classes that must be subclassed:
 * - AirspaceManager
 *   main manager that contains the restrictions for display. It acts as a factory to create instances of the other
 *   classes.
 * - AirspaceVehicleManager
 *   this provides the multi-vehicle support - each vehicle has an instance
 * - AirspaceAdvisoriesProvider
 *   Provides airspace advisories and restrictions. Currently only used by AirspaceManager, but
 *   each vehicle could have its own restrictions.
 */

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QGeoCoordinate>
#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>

class Vehicle;
class QGCApplication;
class AirspaceWeatherInfoProvider;
class AirspaceRulesetsProvider;
class AirspaceVehicleManager;
class AirspaceAdvisoryProvider;
class AirspaceRestrictionProvider;
class MissionController;

Q_DECLARE_LOGGING_CATEGORY(AirspaceManagementLog)

//-----------------------------------------------------------------------------
/**
 * @class AirspaceManager
 * Base class for airspace management. There is one (global) instantiation of this
 */
class AirspaceManager : public QGCTool {
    Q_OBJECT
public:
    AirspaceManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~AirspaceManager();

    Q_PROPERTY(QString                      providerName        READ providerName       CONSTANT)
    Q_PROPERTY(AirspaceWeatherInfoProvider* weatherInfo         READ weatherInfo        CONSTANT)
    Q_PROPERTY(AirspaceAdvisoryProvider*    advisories          READ advisories         CONSTANT)
    Q_PROPERTY(AirspaceRulesetsProvider*    ruleSets            READ ruleSets           CONSTANT)
    Q_PROPERTY(AirspaceRestrictionProvider* airspaces           READ airspaces          CONSTANT)
    Q_PROPERTY(bool                         airspaceVisible     READ airspaceVisible    WRITE setAirspaceVisible    NOTIFY airspaceVisibleChanged)

    Q_INVOKABLE void setROI                     (QGeoCoordinate center, double radius);

    AirspaceWeatherInfoProvider* weatherInfo    () { return _weatherProvider; }
    AirspaceAdvisoryProvider*    advisories     () { return _advisories; }
    AirspaceRulesetsProvider*    ruleSets       () { return _ruleSetsProvider; }
    AirspaceRestrictionProvider* airspaces      () { return _airspaces; }

    void setToolbox(QGCToolbox* toolbox) override;

    virtual QString             providerName    () const = 0;   ///< Name of the airspace management provider (used in the UI)

    virtual bool                airspaceVisible () { return _airspaceVisible; }
    virtual void             setAirspaceVisible (bool set) { _airspaceVisible = set; emit airspaceVisibleChanged(); }

    /**
     * Factory method to create an AirspaceVehicleManager object
     */
    virtual AirspaceVehicleManager*         instantiateVehicle                      (const Vehicle& vehicle) = 0;

    /**
     * Create/upload a flight from a mission.
     */
    virtual void                            createFlight                            (MissionController* missionController) = 0;

signals:
    void    airspaceVisibleChanged  ();

protected:
    /**
     * Set the ROI for airspace information (restrictions shown in UI)
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    virtual void                            _setROI                                 (const QGeoCoordinate& center, double radiusMeters);

    /**
     * Factory method to create an AirspaceRulesetsProvider object
     */
    virtual AirspaceRulesetsProvider*       _instantiateRulesetsProvider            () = 0;

    /**
     * Factory method to create an AirspaceRulesetsProvider object
     */
    virtual AirspaceWeatherInfoProvider*    _instatiateAirspaceWeatherInfoProvider  () = 0;

    /**
     * Factory method to create an AirspaceAdvisoryProvider object
     */
    virtual AirspaceAdvisoryProvider*       _instatiateAirspaceAdvisoryProvider     () = 0;

    /**
     * Factory method to create an AirspaceRestrictionProvider object
     */
    virtual AirspaceRestrictionProvider*    _instantiateAirspaceRestrictionProvider () = 0;

protected:
    bool                            _airspaceVisible;
    AirspaceRulesetsProvider*       _ruleSetsProvider       = nullptr; ///< Rulesets that are shown in the UI
    AirspaceWeatherInfoProvider*    _weatherProvider        = nullptr; ///< Weather info that is shown in the UI
    AirspaceAdvisoryProvider*       _advisories             = nullptr; ///< Advisory info that is shown in the UI
    AirspaceRestrictionProvider*    _airspaces              = nullptr; ///< Airspace info that is shown in the UI
    QTimer                          _roiUpdateTimer;
    QGeoCoordinate                  _roiCenter;
    double                          _roiRadius;

private:
    void _updateToROI   ();

};
