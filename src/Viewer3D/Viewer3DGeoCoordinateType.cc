#include "Viewer3DGeoCoordinateType.h"

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(Viewer3DGeoCoordinateTypeLog, "Viewer3d.Viewer3DGeoCoordinateType")

void Viewer3DGeoCoordinateType::_gpsToLocal()
{
    const QVector3D localPose = QGCGeo::convertGpsToEnu(_coordinate, _gpsRef);

    if (_localCoordinate != localPose) {
        _localCoordinate = localPose;
        _localCoordinate.setZ(_coordinate.altitude());
        emit localCoordinateChanged();
    }
}

void Viewer3DGeoCoordinateType::setGpsRef(const QGeoCoordinate &newGpsRef)
{
    if (_gpsRef == newGpsRef) {
        return;
    }
    _gpsRef = newGpsRef;
    qCDebug(Viewer3DGeoCoordinateTypeLog) << "GPS ref set to" << _gpsRef;
    _gpsToLocal();
    emit gpsRefChanged();
}

void Viewer3DGeoCoordinateType::setCoordinate(const QGeoCoordinate &newCoordinate)
{
    if (_coordinate == newCoordinate) {
        return;
    }
    _coordinate = newCoordinate;
    _gpsToLocal();
    emit coordinateChanged();
}
