#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(Viewer3DManagerLog)

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

    Q_INVOKABLE void setDisplayMode(DisplayMode mode);

signals:
    void gpsRefChanged();
    void displayModeChanged();

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

    Viewer3DMapProvider *_mapProvider = nullptr;
    Vehicle *_activeVehicle = nullptr;

    QGeoCoordinate _gpsRef;
    GpsRefSource _gpsRefSource = GpsRefSource::None;
    DisplayMode _displayMode = DisplayMode::Map;
};
