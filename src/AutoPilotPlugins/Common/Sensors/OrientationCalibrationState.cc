#include "OrientationCalibrationState.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(OrientationCalibrationStateLog, "Sensors.OrientationCalibrationState")

OrientationCalibrationState::OrientationCalibrationState(QObject* parent)
    : QObject(parent)
{
    reset();
}

void OrientationCalibrationState::reset(bool allDone)
{
    for (int i = 0; i < SideCount; i++) {
        _done[i] = allDone;
        _visible[i] = false;
        _inProgress[i] = false;
        _rotate[i] = false;
    }

    emit sidesDoneChanged();
    emit sidesVisibleChanged();
    emit sidesInProgressChanged();
    emit sidesRotateChanged();
}

void OrientationCalibrationState::setVisibleSides(int visibleMask)
{
    _visible[static_cast<int>(CalibrationSide::Down)]  = (visibleMask & CalibrationSideMask::MaskDown) != 0;
    _visible[static_cast<int>(CalibrationSide::Up)]    = (visibleMask & CalibrationSideMask::MaskUp) != 0;
    _visible[static_cast<int>(CalibrationSide::Left)]  = (visibleMask & CalibrationSideMask::MaskLeft) != 0;
    _visible[static_cast<int>(CalibrationSide::Right)] = (visibleMask & CalibrationSideMask::MaskRight) != 0;
    _visible[static_cast<int>(CalibrationSide::Front)] = (visibleMask & CalibrationSideMask::MaskFront) != 0;
    _visible[static_cast<int>(CalibrationSide::Back)]  = (visibleMask & CalibrationSideMask::MaskBack) != 0;

    emit sidesVisibleChanged();
}

void OrientationCalibrationState::setSideInProgress(CalibrationSide side, bool inProgress)
{
    int idx = static_cast<int>(side);
    if (idx >= 0 && idx < SideCount) {
        _inProgress[idx] = inProgress;
        emit sidesInProgressChanged();
    }
}

void OrientationCalibrationState::setSideDone(CalibrationSide side)
{
    int idx = static_cast<int>(side);
    if (idx >= 0 && idx < SideCount) {
        _done[idx] = true;
        _inProgress[idx] = false;
        _rotate[idx] = false;
        emit sidesDoneChanged();
        emit sidesInProgressChanged();
        emit sidesRotateChanged();
    }
}

void OrientationCalibrationState::setSideRotate(CalibrationSide side, bool rotate)
{
    int idx = static_cast<int>(side);
    if (idx >= 0 && idx < SideCount) {
        _rotate[idx] = rotate;
        emit sidesRotateChanged();
    }
}

bool OrientationCalibrationState::isSideDone(CalibrationSide side) const
{
    int idx = static_cast<int>(side);
    return (idx >= 0 && idx < SideCount) ? _done[idx] : false;
}

bool OrientationCalibrationState::isSideVisible(CalibrationSide side) const
{
    int idx = static_cast<int>(side);
    return (idx >= 0 && idx < SideCount) ? _visible[idx] : false;
}

bool OrientationCalibrationState::isSideInProgress(CalibrationSide side) const
{
    int idx = static_cast<int>(side);
    return (idx >= 0 && idx < SideCount) ? _inProgress[idx] : false;
}

bool OrientationCalibrationState::isSideRotate(CalibrationSide side) const
{
    int idx = static_cast<int>(side);
    return (idx >= 0 && idx < SideCount) ? _rotate[idx] : false;
}

CalibrationSide OrientationCalibrationState::currentSide() const
{
    for (int i = 0; i < SideCount; i++) {
        if (_inProgress[i]) {
            return static_cast<CalibrationSide>(i);
        }
    }
    return CalibrationSide::Down;
}

bool OrientationCalibrationState::allVisibleSidesDone() const
{
    for (int i = 0; i < SideCount; i++) {
        if (_visible[i] && !_done[i]) {
            return false;
        }
    }
    return true;
}

int OrientationCalibrationState::completedSideCount() const
{
    int count = 0;
    for (int i = 0; i < SideCount; i++) {
        if (_visible[i] && _done[i]) {
            count++;
        }
    }
    return count;
}

int OrientationCalibrationState::visibleSideCount() const
{
    int count = 0;
    for (int i = 0; i < SideCount; i++) {
        if (_visible[i]) {
            count++;
        }
    }
    return count;
}
