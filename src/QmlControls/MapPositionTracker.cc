/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapPositionTracker.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MapPositionTrackerLog, "QMLControls.MapPositionTracker")

MapPositionTracker::MapPositionTracker(QObject* parent) : QObject(parent)
{
    _resumeTimer.setSingleShot(true);
    _resumeTimer.setInterval(kResumeDelayMs);
    connect(&_resumeTimer, &QTimer::timeout, this, &MapPositionTracker::_resumeFollowing);
}

void MapPositionTracker::setActive(bool active)
{
    if (_active == active) {
        return;
    }
    _active = active;
    emit activeChanged();
    if (_active) {
        // Re-run the one-shots for positions that arrived while inactive
        if (!_evaluateVehicleOneShot()) {
            _evaluateGCSOneShot();
        }
    }
}

void MapPositionTracker::setGcsPosition(const QGeoCoordinate& position)
{
    if (_gcsPosition == position) {
        return;
    }
    _gcsPosition = position;
    emit gcsPositionChanged();
    _evaluateGCSOneShot();
}

void MapPositionTracker::setVehicleCoordinate(const QGeoCoordinate& coordinate)
{
    if (_vehicleCoordinate == coordinate) {
        return;
    }
    _vehicleCoordinate = coordinate;
    emit vehicleCoordinateChanged();
    if (_evaluateVehicleOneShot()) {
        return;
    }
    // Continuous hard follow
    if (_active && _keepVehicleCentered && _vehicleCoordinate.isValid() && !_paused()) {
        emit centerMap(_vehicleCoordinate, false);
    }
}

void MapPositionTracker::setAllowGCSLocationCenter(bool allow)
{
    if (_allowGCSLocationCenter == allow) {
        return;
    }
    _allowGCSLocationCenter = allow;
    emit allowGCSLocationCenterChanged();
    if (allow) {
        // A position may already be valid; don't leave the one-shot dormant
        _evaluateGCSOneShot();
    }
}

void MapPositionTracker::setAllowVehicleLocationCenter(bool allow)
{
    if (_allowVehicleLocationCenter == allow) {
        return;
    }
    _allowVehicleLocationCenter = allow;
    emit allowVehicleLocationCenterChanged();
    if (allow) {
        // A position may already be valid; don't leave the one-shot dormant
        _evaluateVehicleOneShot();
    }
}

void MapPositionTracker::setCenterGCSWhenVehicleValid(bool center)
{
    if (_centerGCSWhenVehicleValid == center) {
        return;
    }
    _centerGCSWhenVehicleValid = center;
    emit centerGCSWhenVehicleValidChanged();
}

void MapPositionTracker::setKeepVehicleCentered(bool keep)
{
    if (_keepVehicleCentered == keep) {
        return;
    }
    _keepVehicleCentered = keep;
    emit keepVehicleCenteredChanged();
}

void MapPositionTracker::setUserInteracting(bool interacting)
{
    if (_userInteracting == interacting) {
        return;
    }
    _userInteracting = interacting;
    emit userInteractingChanged();
    if (_userInteracting) {
        _resumeTimer.stop();
        qCDebug(MapPositionTrackerLog) << "user interaction started, following paused";
    } else {
        _resumeTimer.start();
    }
}

void MapPositionTracker::setAnimating(bool animating)
{
    if (_animating == animating) {
        return;
    }
    _animating = animating;
    emit animatingChanged();
}

void MapPositionTracker::setFirstVehiclePositionReceived(bool received)
{
    if (_firstVehiclePositionReceived == received) {
        return;
    }
    _firstVehiclePositionReceived = received;
    emit firstVehiclePositionReceivedChanged();
}

bool MapPositionTracker::_evaluateVehicleOneShot()
{
    if (!_active || _firstVehiclePositionReceived || !_allowVehicleLocationCenter || !_vehicleCoordinate.isValid()) {
        return false;
    }
    setFirstVehiclePositionReceived(true);
    qCDebug(MapPositionTrackerLog) << "centering on first vehicle position";
    emit centerMap(_vehicleCoordinate, true);
    return true;
}

void MapPositionTracker::_evaluateGCSOneShot()
{
    if (!_active || _firstGCSPositionReceived || _firstVehiclePositionReceived || !_allowGCSLocationCenter ||
        !_gcsPosition.isValid()) {
        return;
    }
    // Deliberately latched even when no centering occurs below (legacy
    // FlightMap parity): the one-shot is spent on the first valid fix
    _firstGCSPositionReceived = true;
    // Only center on the GCS when there is no vehicle to look at, or when the
    // map is going to keep following the vehicle anyway
    if (_keepVehicleCentered || _centerGCSWhenVehicleValid || !_vehicleCoordinate.isValid()) {
        qCDebug(MapPositionTrackerLog) << "centering on first GCS position";
        emit centerMap(_gcsPosition, false);
    }
}

void MapPositionTracker::_resumeFollowing()
{
    qCDebug(MapPositionTrackerLog) << "following resumed after user interaction";
    if (_active && _keepVehicleCentered && _vehicleCoordinate.isValid()) {
        emit centerMap(_vehicleCoordinate, false);
    }
    // Inset following resumes on the consumer's next evaluateInsetFollow call
}

void MapPositionTracker::evaluateInsetFollow(const QPointF& vehicleScreenPoint, const QRectF& centerRect,
                                             const QVariantList& cornerRects)
{
    if (!_active || _keepVehicleCentered || !_firstVehiclePositionReceived || !_vehicleCoordinate.isValid() ||
        _paused() || _animating) {
        return;
    }
    bool recenterNeeded = !centerRect.contains(vehicleScreenPoint);
    if (!recenterNeeded) {
        // Inside the center rect, but possibly hidden under a corner UI element
        for (const QVariant& rect : cornerRects) {
            if (rect.toRectF().contains(vehicleScreenPoint)) {
                recenterNeeded = true;
                break;
            }
        }
    }
    if (recenterNeeded) {
        emit recenterVehicleTo(centerRect.center());
    }
}
