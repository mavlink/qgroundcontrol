#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class QVariant;
class Vehicle;
class Viewer3DMapProvider;

class Viewer3DManager : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(QGCViewer3DManager)
    QML_SINGLETON
    Q_MOC_INCLUDE("Viewer3DMapProvider.h")

    Q_PROPERTY(Viewer3DMapProvider *mapProvider  READ mapProvider  CONSTANT)
    Q_PROPERTY(QGeoCoordinate       gpsRef      READ gpsRef       NOTIFY gpsRefChanged)
    Q_PROPERTY(DisplayMode          displayMode READ displayMode  NOTIFY displayModeChanged)
    Q_PROPERTY(bool vehicleOutsideMapRegion READ vehicleOutsideMapRegion NOTIFY vehicleOutsideMapRegionChanged)

public:
    enum DisplayMode {
        Map,
        View3D,
    };
    Q_ENUM(DisplayMode)

    explicit Viewer3DManager(QObject *parent = nullptr);

    Viewer3DMapProvider *mapProvider() const { return _mapProvider; }
    QGeoCoordinate gpsRef() const { return _gpsRef; }
    DisplayMode displayMode() const { return _displayMode; }
    bool vehicleOutsideMapRegion() const { return _vehicleOutsideMapRegion; }

    /// Returns true if the coordinate lies within the lat/lon bounding box. Invalid inputs return false.
    static bool coordinateWithinRegion(const QGeoCoordinate &coordinate, const QGeoCoordinate &bbMin, const QGeoCoordinate &bbMax);

    Q_INVOKABLE void setDisplayMode(DisplayMode mode);

signals:
    void gpsRefChanged();
    void displayModeChanged();
    void vehicleOutsideMapRegionChanged();

private:
    enum class GpsRefSource {
        None,
        Map,
        Vehicle,
    };

    void _onGpsRefChanged(const QGeoCoordinate &newGpsRef, bool isRefSet);
    void _onActiveVehicleChanged(Vehicle *vehicle);
    void _onActiveVehicleCoordinateChanged(const QGeoCoordinate &newCoordinate);
    void _onEnabledChanged(const QVariant &value);
    void _updateVehicleOutsideMapRegion();

    Viewer3DMapProvider *_mapProvider = nullptr;
    Vehicle *_activeVehicle = nullptr;

    QGeoCoordinate _gpsRef;
    GpsRefSource _gpsRefSource = GpsRefSource::None;
    DisplayMode _displayMode = DisplayMode::Map;
    bool _vehicleOutsideMapRegion = false;
};
