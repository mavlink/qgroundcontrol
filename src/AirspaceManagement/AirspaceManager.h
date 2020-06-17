/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGCGeoBoundingCube.h"

#include <QGeoCoordinate>
#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>

class AirspaceAdvisoryProvider;
class AirspaceFlightPlanProvider;
class AirspaceRestrictionProvider;
class AirspaceRulesetsProvider;
class AirspaceVehicleManager;
class AirspaceWeatherInfoProvider;
class PlanMasterController;
class QGCApplication;
class Vehicle;

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
    virtual ~AirspaceManager() override;

    enum AuthStatus {
        Unknown,
        Anonymous,
        Authenticated,
        Error
    };

    Q_ENUM(AuthStatus)

    Q_PROPERTY(QString                      providerName        READ providerName       CONSTANT)
    Q_PROPERTY(AirspaceWeatherInfoProvider* weatherInfo         READ weatherInfo        CONSTANT)
    Q_PROPERTY(AirspaceAdvisoryProvider*    advisories          READ advisories         CONSTANT)
    Q_PROPERTY(AirspaceRulesetsProvider*    ruleSets            READ ruleSets           CONSTANT)
    Q_PROPERTY(AirspaceRestrictionProvider* airspaces           READ airspaces          CONSTANT)
    Q_PROPERTY(AirspaceFlightPlanProvider*  flightPlan          READ flightPlan         CONSTANT)
    Q_PROPERTY(bool                         connected           READ connected          NOTIFY connectedChanged)
    Q_PROPERTY(QString                      connectStatus       READ connectStatus      NOTIFY connectStatusChanged)
    Q_PROPERTY(AirspaceManager::AuthStatus  authStatus          READ authStatus         NOTIFY authStatusChanged)
    Q_PROPERTY(bool                         airspaceVisible     READ airspaceVisible    WRITE  setAirspaceVisible    NOTIFY airspaceVisibleChanged)

    Q_INVOKABLE void setROI                     (const QGeoCoordinate& pointNW, const QGeoCoordinate& pointSE, bool planView, bool reset = false);

    AirspaceWeatherInfoProvider* weatherInfo    () { return _weatherProvider; }
    AirspaceAdvisoryProvider*    advisories     () { return _advisories; }
    AirspaceRulesetsProvider*    ruleSets       () { return _ruleSetsProvider; }
    AirspaceRestrictionProvider* airspaces      () { return _airspaces; }
    AirspaceFlightPlanProvider*  flightPlan     () { return _flightPlan; }

    void setToolbox(QGCToolbox* toolbox) override;

    virtual QString             providerName    () const = 0;   ///< Name of the airspace management provider (used in the UI)

    virtual bool                airspaceVisible () { return _airspaceVisible; }
    virtual void             setAirspaceVisible (bool set) { _airspaceVisible = set; emit airspaceVisibleChanged(); }
    virtual bool                connected       () const = 0;
    virtual QString             connectStatus   () const { return QString(); }
    virtual double             maxAreaOfInterest() const { return _maxAreaOfInterest; }

    virtual AirspaceManager::AuthStatus authStatus () const { return Anonymous; }

    /**
     * Factory method to create an AirspaceVehicleManager object
     */
    virtual AirspaceVehicleManager*         instantiateVehicle                      (const Vehicle& vehicle) = 0;

signals:
    void                airspaceVisibleChanged  ();
    void                connectedChanged        ();
    void                connectStatusChanged    ();
    void                authStatusChanged       ();

protected:
    /**
     * Set the ROI for airspace information (restrictions shown in UI)
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    virtual void                            _setROI                                 (const QGCGeoBoundingCube& roi);

    /**
     * Factory methods
     */
    virtual AirspaceRulesetsProvider*       _instantiateRulesetsProvider            () = 0;
    virtual AirspaceWeatherInfoProvider*    _instatiateAirspaceWeatherInfoProvider  () = 0;
    virtual AirspaceAdvisoryProvider*       _instatiateAirspaceAdvisoryProvider     () = 0;
    virtual AirspaceRestrictionProvider*    _instantiateAirspaceRestrictionProvider () = 0;
    virtual AirspaceFlightPlanProvider*     _instantiateAirspaceFlightPlanProvider  () = 0;

protected:
    bool                            _airspaceVisible        = false;
    AirspaceRulesetsProvider*       _ruleSetsProvider       = nullptr;  ///< Rulesets
    AirspaceWeatherInfoProvider*    _weatherProvider        = nullptr;  ///< Weather info
    AirspaceAdvisoryProvider*       _advisories             = nullptr;  ///< Advisory info
    AirspaceRestrictionProvider*    _airspaces              = nullptr;  ///< Airspace info
    AirspaceFlightPlanProvider*     _flightPlan             = nullptr;  ///< Flight plan management
    double                          _maxAreaOfInterest      = 500.0;    ///< Ignore area larger than 500km^2
    QTimer                          _ruleUpdateTimer;
    QTimer                          _updateTimer;
    QGCGeoBoundingCube              _roi;

private slots:
    void _updateRulesTimeout        ();
    void _updateTimeout             ();
    void _rulesChanged              ();

private:
    void _updateToROI               (bool reset = false);

};
