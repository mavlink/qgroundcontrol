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
    
    /// Returns true if the manager is currently communicating with the vehicle
    virtual bool inProgress(void) const { return false; }

    /// Load the current settings from the vehicle
    virtual void loadFromVehicle(void);

    /// Send the current settings to the vehicle
    virtual void sendToVehicle(const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon);

    virtual bool circleEnabled          (void) const { return false; }
    virtual bool polygonEnabled         (void) const { return false; }
    virtual bool breachReturnEnabled    (void) const { return false; }

    virtual float           circleRadius        (void) const { return 0.0; }
    QList<QGeoCoordinate>   polygon             (void) const { return _polygon; }
    QGeoCoordinate          breachReturnPoint   (void) const { return _breachReturnPoint; }

    virtual QVariantList    params      (void) const { return QVariantList(); }
    virtual QStringList     paramLabels (void) const { return QStringList(); }

    virtual QString editorQml(void) const { return QStringLiteral("qrc:/FirmwarePlugin/NoGeoFenceEditor.qml"); }

    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        TooFewPoints,           ///< Too few points for valid geofence
        TooManyPoints,          ///< Too many points for valid geofence
        InvalidCircleRadius,
    } ErrorCode_t;
    
signals:
    void loadComplete               (const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon);
    void circleEnabledChanged       (bool circleEnabled);
    void polygonEnabledChanged      (bool polygonEnabled);
    void breachReturnEnabledChanged (bool fenceEnabled);
    void circleRadiusChanged        (float circleRadius);
    void inProgressChanged          (bool inProgress);
    void error                      (int errorCode, const QString& errorMsg);
    void paramsChanged              (QVariantList params);
    void paramLabelsChanged         (QStringList paramLabels);
    
protected:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);

    Vehicle*                _vehicle;
    QList<QGeoCoordinate>   _polygon;
    QGeoCoordinate          _breachReturnPoint;
};

#endif
