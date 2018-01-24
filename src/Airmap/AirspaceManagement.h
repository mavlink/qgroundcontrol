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
 * @file AirspaceManagement.h
 * This file contains the interface definitions used by an airspace management implementation (AirMap).
 * There are 3 base classes that must be subclassed:
 * - AirspaceManager
 *   main manager that contains the restrictions for display. It acts as a factory to create instances of the other
 *   classes.
 * - AirspaceManagerPerVehicle
 *   this provides the multi-vehicle support - each vehicle has an instance
 * - AirspaceRestrictionProvider
 *   provides airspace restrictions. Currently only used by AirspaceManager, but
 *   each vehicle could have its own restrictions.
 */

#include "QGCToolbox.h"
#include "MissionItem.h"

#include <QObject>
#include <QString>
#include <QList>

#include <QGCLoggingCategory.h>

Q_DECLARE_LOGGING_CATEGORY(AirspaceManagementLog)

/**
 * Contains the status of the Airspace authorization
 */
class AirspaceAuthorization : public QObject {
    Q_OBJECT
public:
    enum PermitStatus {
        PermitUnknown = 0,
        PermitPending,
        PermitAccepted,
        PermitRejected,
    };
    Q_ENUMS(PermitStatus);
};

/**
 * Base class for an airspace restriction
 */
class AirspaceRestriction : public QObject
{
    Q_OBJECT

public:
    AirspaceRestriction(QObject* parent = NULL);
};

class PolygonAirspaceRestriction : public AirspaceRestriction
{
    Q_OBJECT

public:
    PolygonAirspaceRestriction(const QVariantList& polygon, QObject* parent = NULL);

    Q_PROPERTY(QVariantList polygon MEMBER _polygon CONSTANT)

    const QVariantList& getPolygon() const { return _polygon; }

private:
    QVariantList    _polygon;
};

class CircularAirspaceRestriction : public AirspaceRestriction
{
    Q_OBJECT

public:
    CircularAirspaceRestriction(const QGeoCoordinate& center, double radius, QObject* parent = NULL);

    Q_PROPERTY(QGeoCoordinate   center MEMBER _center CONSTANT)
    Q_PROPERTY(double           radius MEMBER _radius CONSTANT)

private:
    QGeoCoordinate  _center;
    double          _radius;
};


/**
 * @class AirspaceRestrictionProvider
 * Base class that queries for airspace restrictions
 */
class AirspaceRestrictionProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRestrictionProvider() = default;
    ~AirspaceRestrictionProvider() = default;


    /**
     * Set region of interest that should be queried. When finished, the requestDone() signal will be emmited.
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    virtual void setROI(const QGeoCoordinate& center, double radiusMeters) = 0;

    const QList<PolygonAirspaceRestriction*>& polygons() const { return _polygonList; }
    const QList<CircularAirspaceRestriction*>& circles() const { return _circleList; }

signals:
    void requestDone(bool success);

protected:
    QList<PolygonAirspaceRestriction*> _polygonList;
    QList<CircularAirspaceRestriction*> _circleList;
};

/**
 * @class AirspaceRulesetsProvider
 * Base class that queries for airspace rulesets
 */
class AirspaceRulesetsProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRulesetsProvider    () = default;
    ~AirspaceRulesetsProvider   () = default;
    /**
     * Set region of interest that should be queried. When finished, the requestDone() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void setROI         (const QGeoCoordinate& center) = 0;
signals:
    void requestDone(bool success);
};

class AirspaceManagerPerVehicle;
class Vehicle;

struct WeatherInformation
{
    QString     condition;              ///< The overall weather condition.
    QString     icon;                   ///< The icon or class of icon that should be used for display purposes.
    uint32_t    windHeading = 0;        ///< The heading in [°].
    uint32_t    windSpeed   = 0;        ///< The speed in [°].
    uint32_t    windGusting = 0;
    int32_t     temperature = 0;        ///< The temperature in [°C].
    float       humidity    = 0.0;
    uint32_t    visibility  = 0;        ///< Visibility in [m].
    uint32_t    precipitation = 0;      ///< The probability of precipitation in [%].
};
Q_DECLARE_METATYPE(WeatherInformation);

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
     * Factory method to create an AirspaceManagerPerVehicle object
     */
    virtual AirspaceManagerPerVehicle* instantiateVehicle(const Vehicle& vehicle) = 0;

    /**
     * Factory method to create an AirspaceRestrictionProvider object
     */
    virtual AirspaceRestrictionProvider* instantiateRestrictionProvider() = 0;

    /**
     * Factory method to create an AirspaceRulesetsProvider object
     */
    virtual AirspaceRulesetsProvider* instantiateRulesetsProvider() = 0;

    /**
     * Set the ROI for airspace information (restrictions shown in UI)
     * @param center Center coordinate for ROI
     * @param radiusMeters Radius in meters around center which is of interest
     */
    void setROI(const QGeoCoordinate& center, double radiusMeters);

    QmlObjectListModel* polygonRestrictions(void) { return &_polygonRestrictions; }
    QmlObjectListModel* circularRestrictions(void) { return &_circleRestrictions; }

    void setToolbox(QGCToolbox* toolbox) override;

    /**
     * Name of the airspace management provider (used in the UI)
     */
    virtual QString name() const = 0;

    /**
     * Request weather information update. When done, it will emit the weatherUpdate() signal.
     * @param coordinate request update for this coordinate
     */
    virtual void requestWeatherUpdate(const QGeoCoordinate& coordinate) = 0;

signals:
    void weatherUpdate(bool success, QGeoCoordinate coordinate, WeatherInformation weather);

private slots:
    void _restrictionsUpdated(bool success);

private:
    void _updateToROI();

    AirspaceRestrictionProvider*    _restrictionsProvider   = nullptr; ///< restrictions that are shown in the UI
    AirspaceRulesetsProvider*       _rulesetsProvider       = nullptr; ///< restrictions that are shown in the UI

    QmlObjectListModel _polygonRestrictions;    ///< current polygon restrictions
    QmlObjectListModel _circleRestrictions;     ///< current circle restrictions

    QTimer                  _roiUpdateTimer;
    QGeoCoordinate          _roiCenter;
    double                  _roiRadius;
};


/**
 * @class AirspaceManagerPerVehicle
 * Base class for per-vehicle management (each vehicle has one (or zero) of these)
 */
class AirspaceManagerPerVehicle : public QObject {
    Q_OBJECT
public:
    AirspaceManagerPerVehicle(const Vehicle& vehicle);
    virtual ~AirspaceManagerPerVehicle() = default;


    /**
     * create/upload a flight from a mission. This should update the flight permit status.
     * There can only be one active flight for each vehicle.
     */
    virtual void createFlight(const QList<MissionItem*>& missionItems) = 0;

    /**
     * get the current flight permit status
     */
    virtual AirspaceAuthorization::PermitStatus flightPermitStatus() const = 0;


    /**
     * Setup the connection and start sending telemetry
     */
    virtual void startTelemetryStream() = 0;

    virtual void stopTelemetryStream() = 0;

    virtual bool isTelemetryStreaming() const = 0;

public slots:
    virtual void endFlight() = 0;

signals:
    void trafficUpdate(QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);

    void flightPermitStatusChanged();

protected slots:
    virtual void vehicleMavlinkMessageReceived(const mavlink_message_t& message) = 0;

protected:
    const Vehicle& _vehicle;

private slots:
    void _vehicleArmedChanged(bool armed);

private:
    bool _vehicleWasInMissionMode = false; ///< true if the vehicle was in mission mode when arming
};
