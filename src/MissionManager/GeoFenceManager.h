/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef GeoFenceManager_H
#define GeoFenceManager_H

#include <QObject>
#include <QGeoCoordinate>

#include "QGCLoggingCategory.h"
#include "FactSystem.h"
#include "PlanManager.h"
#include "QGCFencePolygon.h"
#include "QGCFenceCircle.h"

class Vehicle;
class QmlObjectListModel;
class PlanManager;

Q_DECLARE_LOGGING_CATEGORY(GeoFenceManagerLog)

/// This is the base class for firmware specific geofence managers. A geofence manager is responsible
/// for communicating with the vehicle to set/get geofence settings.
class GeoFenceManager : public QObject
{
    Q_OBJECT
    
public:
    GeoFenceManager(Vehicle* vehicle);
    ~GeoFenceManager();
    
    /// Returns true if the manager is currently communicating with the vehicle
    virtual bool inProgress(void) const;

    /// Load the current settings from the vehicle
    ///     Signals loadComplete when done
    virtual void loadFromVehicle(void);

    /// Send the geofence settings to the vehicle
    ///     Signals sendComplete when done
    virtual void sendToVehicle(const QGeoCoordinate&    breachReturn,   ///< Breach return point
                               QmlObjectListModel&      polygons,       ///< List of QGCFencePolygons
                               QmlObjectListModel&      circles);       ///< List of QGCFenceCircles

    /// Remove all fence related items from vehicle (does not affect parameters)
    ///     Signals removeAllComplete when done
    virtual void removeAll(void);

    /// Returns true if this vehicle support polygon fence
    ///     Signal: polygonSupportedChanged
    virtual bool polygonSupported(void) const { return true; }

    /// Returns true if polygon fence is currently enabled on this vehicle
    ///     Signal: polygonEnabledChanged
    virtual bool polygonEnabled(void) const { return true; }

    /// Returns true if breach return is supported by this vehicle
    ///     Signal: breachReturnSupportedChanged
    virtual bool breachReturnSupported(void) const { return false; }

    /// Returns a list of parameter facts that relate to geofence support for the vehicle
    ///     Signal: paramsChanged
    virtual QVariantList params(void) const { return _params; }

    /// Returns the user visible labels for the paremeters returned by params method
    ///     Signal: paramLabelsChanged
    virtual QStringList paramLabels(void) const { return _paramLabels; }

    /// Returns true if circular fence is currently enabled on vehicle
    ///     Signal: circleEnabledChanged
    virtual bool circleEnabled(void) const { return false; }

    /// Returns the fact which controls the fence circle radius. NULL if not supported
    ///     Signal: circleRadiusFactChanged
    virtual Fact* circleRadiusFact(void) const { return _circleRadiusFact; }

    const QList<QGCFencePolygon>&   polygons(void) { return _polygons; }
    const QList<QGCFenceCircle>&    circles(void) { return _circles; }
    const QGeoCoordinate&           breachReturnPoint(void) const { return _breachReturnPoint; }

    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        PolygonTooFewPoints,    ///< Too few points for valid fence polygon
        PolygonTooManyPoints,   ///< Too many points for valid fence polygon
        IncompletePolygonLoad,  ///< Incomplete polygon loaded
        UnsupportedCommand,     ///< Usupported command in mission type
        BadPolygonItemFormat,   ///< Error re-creating polygons from mission items
        InvalidCircleRadius,
    } ErrorCode_t;
    
signals:
    void loadComplete                   (void);
    void inProgressChanged              (bool inProgress);
    void error                          (int errorCode, const QString& errorMsg);
    void paramsChanged                  (QVariantList params);
    void paramLabelsChanged             (QStringList paramLabels);
    void circleEnabledChanged           (bool circleEnabled);
    void circleRadiusFactChanged        (Fact* circleRadiusFact);
    void polygonSupportedChanged        (bool polygonSupported);
    void polygonEnabledChanged          (bool polygonEnabled);
    void breachReturnSupportedChanged   (bool breachReturnSupported);
    void removeAllComplete              (bool error);
    void sendComplete                   (bool error);

private slots:
    void _parametersReady           (void);
    void _sendComplete              (bool error);
    void _planManagerLoadComplete   (bool removeAllRequested);

private:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);

    Vehicle*                _vehicle;
    PlanManager             _planManager;
    QList<QGCFencePolygon>  _polygons;
    QList<QGCFenceCircle>   _circles;
    QGeoCoordinate          _breachReturnPoint;
    bool                    _firstParamLoadComplete;
    Fact*                   _circleRadiusFact;
    QVariantList            _params;
    QStringList             _paramLabels;
    QList<QGCFencePolygon>  _sendPolygons;
    QList<QGCFenceCircle>   _sendCircles;
};

#endif
