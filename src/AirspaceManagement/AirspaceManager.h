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
 * - AirspaceRestrictionProvider
 *   provides airspace restrictions. Currently only used by AirspaceManager, but
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
class AirspaceRestrictionProvider;
class AirspaceRulesetsProvider;
class AirspaceVehicleManager;

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

    /**
     * Factory method to create an AirspaceVehicleManager object
     */
    virtual AirspaceVehicleManager*         instantiateVehicle                      (const Vehicle& vehicle) = 0;

    /**
     * Factory method to create an AirspaceRestrictionProvider object
     */
    virtual AirspaceRestrictionProvider*    instantiateRestrictionProvider          () = 0;

    /**
     * Factory method to create an AirspaceRulesetsProvider object
     */
    virtual AirspaceRulesetsProvider*       instantiateRulesetsProvider             () = 0;

    /**
     * Factory method to create an AirspaceRulesetsProvider object
     */
    virtual AirspaceWeatherInfoProvider*    instatiateAirspaceWeatherInfoProvider   () = 0;
    /**
     * Set the ROI for airspace information (restrictions shown in UI)
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    void setROI                                 (const QGeoCoordinate& center, double radiusMeters);

    QmlObjectListModel* polygonRestrictions     () { return &_polygonRestrictions; }
    QmlObjectListModel* circularRestrictions    () { return &_circleRestrictions;  }
    AirspaceWeatherInfoProvider* weatherInfo    () { return _weather;}

    void setToolbox(QGCToolbox* toolbox) override;

    /**
     * Name of the airspace management provider (used in the UI)
     */
    virtual QString name            () const = 0;

protected slots:
    virtual void _rulessetsUpdated  (bool success);

private slots:
    void _restrictionsUpdated       (bool success);

private:
    void _updateToROI   ();

    AirspaceRestrictionProvider*    _restrictionsProvider   = nullptr; ///< Restrictions that are shown in the UI
    AirspaceRulesetsProvider*       _rulesetsProvider       = nullptr; ///< Restrictions that are shown in the UI
    AirspaceWeatherInfoProvider*    _weatherProvider        = nullptr; ///< Weather info that is shown in the UI

    QmlObjectListModel _polygonRestrictions;    ///< current polygon restrictions
    QmlObjectListModel _circleRestrictions;     ///< current circle restrictions

    QTimer                          _roiUpdateTimer;
    QGeoCoordinate                  _roiCenter;
    double                          _roiRadius;
    AirspaceWeatherInfoProvider*    _weather;
};
