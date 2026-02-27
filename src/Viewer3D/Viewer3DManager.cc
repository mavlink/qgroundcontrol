#include "Viewer3DManager.h"

#include "MultiVehicleManager.h"
#include "OsmParser.h"
#include "QGCLoggingCategory.h"
#include "Fact.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "Viewer3DMapProvider.h"
#include "Viewer3DSettings.h"

QGC_LOGGING_CATEGORY(Viewer3DManagerLog, "Viewer3d.Viewer3DManager")

Viewer3DManager::Viewer3DManager(QObject *parent)
    : QObject(parent)
    , _mapProvider(new OsmParser(this))
{
    _onActiveVehicleChanged(MultiVehicleManager::instance()->activeVehicle());

    connect(_mapProvider, &Viewer3DMapProvider::gpsRefChanged, this, &Viewer3DManager::_onGpsRefChanged);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &Viewer3DManager::_onActiveVehicleChanged);
    connect(SettingsManager::instance()->viewer3DSettings()->enabled(), &Fact::rawValueChanged, this, &Viewer3DManager::_onEnabledChanged);
}

void Viewer3DManager::setDisplayMode(DisplayMode mode)
{
    if (_displayMode == mode) {
        return;
    }

    if (mode == DisplayMode::View3D) {
        if (!SettingsManager::instance()->viewer3DSettings()->enabled()->rawValue().toBool()) {
            return;
        }
    }

    _displayMode = mode;
    qCDebug(Viewer3DManagerLog) << "Display mode changed to" << (mode == DisplayMode::View3D ? "View3D" : "Map");
    emit displayModeChanged();
}

void Viewer3DManager::_onActiveVehicleChanged(Vehicle *vehicle)
{
    if (_activeVehicle) {
        disconnect(_activeVehicle, &Vehicle::coordinateChanged, this, &Viewer3DManager::_onActiveVehicleCoordinateChanged);
    }

    _activeVehicle = vehicle;
    if (!_activeVehicle) {
        if (_gpsRefSource == GpsRefSource::Vehicle) {
            _gpsRefSource = GpsRefSource::None;
            _gpsRef = QGeoCoordinate();
            emit gpsRefChanged();
        }
    } else {
        _onActiveVehicleCoordinateChanged(_activeVehicle->coordinate());
        connect(_activeVehicle, &Vehicle::coordinateChanged, this, &Viewer3DManager::_onActiveVehicleCoordinateChanged);
    }
}

void Viewer3DManager::_onActiveVehicleCoordinateChanged(const QGeoCoordinate &newCoordinate)
{
    if (_gpsRefSource == GpsRefSource::None) {
        if (newCoordinate.isValid()) {
            _gpsRef = newCoordinate;
            _gpsRef.setAltitude(0);
            _gpsRefSource = GpsRefSource::Vehicle;
            emit gpsRefChanged();

            qCDebug(Viewer3DManagerLog) << "GPS ref set by vehicle:" << _gpsRef.latitude() << _gpsRef.longitude() << _gpsRef.altitude();
        }
    }
}

void Viewer3DManager::_onGpsRefChanged(const QGeoCoordinate &newGpsRef, bool isRefSet)
{
    if (isRefSet) {
        _gpsRef = newGpsRef;
        _gpsRefSource = GpsRefSource::Map;
        emit gpsRefChanged();
        qCDebug(Viewer3DManagerLog) << "GPS ref set by map provider:" << _gpsRef.latitude() << _gpsRef.longitude() << _gpsRef.altitude();
    } else {
        if (_gpsRefSource != GpsRefSource::Map) {
            return;
        }

        _gpsRefSource = GpsRefSource::None;
        if (_activeVehicle && _activeVehicle->coordinate().isValid()) {
            _gpsRef = _activeVehicle->coordinate();
            _gpsRef.setAltitude(0);
            _gpsRefSource = GpsRefSource::Vehicle;
        } else {
            _gpsRef = QGeoCoordinate();
        }
        emit gpsRefChanged();
    }
}

void Viewer3DManager::_onEnabledChanged(const QVariant &value)
{
    if (!value.toBool()) {
        setDisplayMode(DisplayMode::Map);
    }
}
