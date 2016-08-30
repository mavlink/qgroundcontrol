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
#include <QTimer>
#include <QGeoCoordinate>

#include "MissionItem.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "LinkInterface.h"
#include "QGCMapPolygon.h"

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(GeoFenceManagerLog)

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

    bool inProgress(void) const;

    /// Request the geo fence from the vehicle
    void requestGeoFence(void);

    /// Returns the current geofence settings
    const GeoFence_t& geoFence(void) const { return _geoFence; }

    /// Set and send the specified geo fence to the vehicle
    void setGeoFence(const GeoFence_t& geoFence);

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
    
private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    //void _ackTimeout(void);
    
private:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);
    void _clearGeoFence(void);
    void _requestFencePoint(uint8_t pointIndex);
    void _sendFencePoint(uint8_t pointIndex);
    bool _geoFenceSupported(void);

private:
    Vehicle*            _vehicle;
    
    bool        _readTransactionInProgress;
    bool        _writeTransactionInProgress;

    uint8_t     _cReadFencePoints;
    uint8_t     _currentFencePoint;
    QVariant    _savedWriteFenceAction;

    GeoFence_t  _geoFence;

    static const char* _fenceTotalParam;
    static const char* _fenceActionParam;
};

#endif
