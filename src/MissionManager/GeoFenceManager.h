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
#include "QGCMapPolygon.h"

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(GeoFenceManagerLog)

/// This is the base class for firmware specific geofence managers. A geofence manager is responsible
/// for communicating with the vehicle to set/get geofence settings.
class GeoFenceManager : public QObject
{
    Q_OBJECT
    
public:
    GeoFenceManager(Vehicle* vehicle);
    ~GeoFenceManager();
    
    typedef enum {
        GeoFenceNone,
        GeoFenceCircle,
        GeoFencePolygon,
    } GeoFenceType_t;

    typedef struct {
        GeoFenceType_t          fenceType;
        float                   circleRadius;
        QGCMapPolygon           polygon;
        QGeoCoordinate          breachReturnPoint;
    } GeoFence_t;

    /// Returns true if the manager is currently communicating with the vehicle
    virtual bool inProgress(void) const { return false; }

    /// Request the geo fence from the vehicle
    virtual void requestGeoFence(void);

    /// Set and send the specified geo fence to the vehicle
    virtual void setGeoFence(const GeoFence_t& geoFence);

    /// Returns the current geofence settings
    const GeoFence_t& geoFence(void) const { return _geoFence; }

    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        TooFewPoints,           ///< Too few points for valid geofence
        TooManyPoints,          ///< Too many points for valid geofence
        InvalidCircleRadius,
    } ErrorCode_t;
    
signals:
    void newGeoFenceAvailable(void);
    void inProgressChanged(bool inProgress);
    void error(int errorCode, const QString& errorMsg);
    
protected:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);
    void _clearGeoFence(void);

    Vehicle*    _vehicle;
    GeoFence_t  _geoFence;
};

#endif
