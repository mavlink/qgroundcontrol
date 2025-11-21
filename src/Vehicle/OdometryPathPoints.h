/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class Vehicle;

class OdometryPathPoints : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    
public:
    OdometryPathPoints(Vehicle* vehicle, QObject* parent = nullptr);

    Q_INVOKABLE QVariantList list(void) const { return _points; }
    bool enabled(void) const { return _enabled; }
    void setEnabled(bool enabled);

signals:
    void pointAdded(QGeoCoordinate coordinate);
    void updateLastPoint(QGeoCoordinate coordinate);
    void pointsCleared(void);
    void enabledChanged();

public slots:
    void clear(void);
    void addOdometryPoint(double x, double y, double z);

private:
    Vehicle*        _vehicle;
    QVariantList    _points;
    QGeoCoordinate  _lastPoint;
    bool            _enabled = false;
    QGeoCoordinate  _referenceCoordinate; // Reference point for NED to geodetic conversion

    static constexpr int _maxPointCount = 600;
};

