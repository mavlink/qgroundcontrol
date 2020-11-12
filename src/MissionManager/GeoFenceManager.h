/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>

#if defined(QGC_AIRMAP_ENABLED)
#include "AirspaceManager.h"
#endif

#include "QGCLoggingCategory.h"
#include "FactSystem.h"
#include "PlanManager.h"
#include "QGCFencePolygon.h"
#include "QGCFenceCircle.h"
#include "PlanManager.h"

class Vehicle;
class QmlObjectListModel;

Q_DECLARE_LOGGING_CATEGORY(GeoFenceManagerLog)

/// This is the base class for firmware specific geofence managers. A geofence manager is responsible
/// for communicating with the vehicle to set/get geofence settings.
class GeoFenceManager : public PlanManager
{
    Q_OBJECT
    
public:
    GeoFenceManager(Vehicle* vehicle);
    ~GeoFenceManager();
    
    bool supported(void) const;

    /// Signals sendComplete when done
    void sendToVehicle(const QGeoCoordinate&    breachReturn,   ///< Breach return point
                       QmlObjectListModel&      polygons,       ///< List of QGCFencePolygons
                       QmlObjectListModel&      circles);       ///< List of QGCFenceCircles

    /// Signals removeAllComplete when done
    void removeAll(void);

    /// Returns true if polygon fence is currently enabled on this vehicle
    ///     Signal: polygonEnabledChanged
    bool polygonEnabled(void) const { return true; }

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
    void loadComplete       (void);
    void inProgressChanged  (bool inProgress);
    void error              (int errorCode, const QString& errorMsg);
    void removeAllComplete  (bool error);
    void sendComplete       (bool error);

private slots:
    void _sendComplete              (bool error);
    void _planManagerLoadComplete   (bool removeAllRequested);

private:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);

    QList<QGCFencePolygon>  _polygons;
    QList<QGCFenceCircle>   _circles;
    QGeoCoordinate          _breachReturnPoint;
    bool                    _firstParamLoadComplete = false;
    QList<QGCFencePolygon>  _sendPolygons;
    QList<QGCFenceCircle>   _sendCircles;
#if defined(QGC_AIRMAP_ENABLED)
    AirspaceManager*        _airspaceManager        = nullptr;
#endif
};
