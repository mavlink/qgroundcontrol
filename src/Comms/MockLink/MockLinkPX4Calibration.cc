#include "MockLinkPX4Calibration.h"
#include "MAVLinkLib.h"
#include "MockLink.h"

#include <algorithm>

// Side names in PX4 detect_orientation_return order (see PX4 detect_orientation_str())
static constexpr const char *kSideNames[MockLinkPX4Calibration::kSideCount] = {
    "back",     // tail down
    "front",    // nose down
    "left",
    "right",
    "up",       // upside down
    "down",     // right side up
};

// Ideal accel readings per side, same order as kSideNames (see PX4 accel_calibration_worker())
static constexpr const char *kSideAccelResults[MockLinkPX4Calibration::kSideCount] = {
    "[9.810 0.000 0.000]",
    "[-9.810 0.000 0.000]",
    "[0.000 9.810 0.000]",
    "[0.000 -9.810 0.000]",
    "[0.000 0.000 9.810]",
    "[0.000 0.000 -9.810]",
};

// Device ids stored in CAL_MAG0_ID/CAL_ACC0_ID after a successful calibration,
// matching the values in PX4MockLink.params
static constexpr int32_t kMagDeviceId = 197388;
static constexpr int32_t kAccelDeviceId = 1310988;

MockLinkPX4Calibration::MockLinkPX4Calibration(MockLink *mockLink)
    : _mockLink(mockLink)
{
}

void MockLinkPX4Calibration::startMagCalibration()
{
    _startCalibration(CalType::Mag);
}

void MockLinkPX4Calibration::startAccelCalibration()
{
    _startCalibration(CalType::Accel);
}

void MockLinkPX4Calibration::_startCalibration(CalType calType)
{
    QMutexLocker locker(&_mutex);

    _calType = calType;
    _requestedPose = -1;
    _activeSide = -1;
    _sideTickCount = 0;
    _finishTickCount = -1;
    std::fill(std::begin(_sideDone), std::end(_sideDone), false);

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] calibration started: 2 %1")
        .arg(QLatin1String((calType == CalType::Mag) ? "mag" : "accel")));
}

bool MockLinkPX4Calibration::cancel()
{
    QMutexLocker locker(&_mutex);

    if (_calType == CalType::None) {
        return false;
    }

    _calType = CalType::None;
    _requestedPose = -1;
    _activeSide = -1;
    _finishTickCount = -1;

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_CRITICAL, QStringLiteral("[cal] calibration cancelled"));
    return true;
}

bool MockLinkPX4Calibration::calibrationActive() const
{
    QMutexLocker locker(&_mutex);
    return _calType != CalType::None;
}

void MockLinkPX4Calibration::setPose(Pose pose)
{
    QMutexLocker locker(&_mutex);
    _requestedPose = static_cast<int>(pose);
}

void MockLinkPX4Calibration::run10HzTasks()
{
    QMutexLocker locker(&_mutex);

    if (_calType == CalType::None) {
        return;
    }

    if (_finishTickCount >= 0) {
        // Finishing phase: simulates the firmware calculating the final calibration
        // values before reporting completion. Gives the UI time to display the last
        // side as completed while the calibration is still active.
        _finishTickCount++;
        if (_finishTickCount >= kTicksToFinish) {
            const CalType calType = _calType;
            _calType = CalType::None;
            _finishTickCount = -1;
            if (calType == CalType::Mag) {
                // Store the calibration result like the firmware does, before reporting
                // done so the subsequent param refresh by QGC picks it up
                _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAL_MAG0_ID"), kMagDeviceId);
                // The mag fit calculation reports its own final progress
                _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <100>"));
            } else {
                _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAL_ACC0_ID"), kAccelDeviceId);
            }
            _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] calibration done: %1")
                .arg(QLatin1String((calType == CalType::Mag) ? "mag" : "accel")));
        }
        return;
    }

    if (_activeSide >= 0) {
        _sideTick();
    } else {
        _detectPose();
    }
}

int MockLinkPX4Calibration::_doneCount() const
{
    int count = 0;
    for (bool done : _sideDone) {
        if (done) {
            count++;
        }
    }
    return count;
}

/// Waiting for a pose: detect the requested orientation as the firmware would.
void MockLinkPX4Calibration::_detectPose()
{
    const int pose = _requestedPose;
    if ((pose < 0) || (pose >= kSideCount)) {
        return;
    }
    _requestedPose = -1; // consume

    if (_sideDone[pose]) {
        _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side already completed").arg(QLatin1String(kSideNames[pose])));
        return;
    }

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 orientation detected").arg(QLatin1String(kSideNames[pose])));
    if (_calType == CalType::Accel) {
        // The accel worker starts sampling immediately (see PX4 accel_calibration_worker())
        _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] Hold still, measuring %1 side").arg(QLatin1String(kSideNames[pose])));
    }
    _activeSide = pose;
    _sideTickCount = 0;
}

/// Side in progress: simulate the sampling phase, then complete the side.
/// Completing the last side enters the finishing phase.
void MockLinkPX4Calibration::_sideTick()
{
    _sideTickCount++;

    switch (_calType) {
    case CalType::Mag:
        _magSideTick();
        break;
    case CalType::Accel:
        _accelSideTick();
        break;
    case CalType::None:
        return;
    }

    if (_doneCount() == kSideCount) {
        // All sides collected: enter the finishing phase
        _finishTickCount = 0;
    }
}

/// Mag side: rotation phase with per-tick progress messages.
void MockLinkPX4Calibration::_magSideTick()
{
    if (_sideTickCount < kTicksPerSide) {
        // Same progress calculation as PX4 mag_calibration_worker()
        const unsigned progress = (_doneCount() * 100 / kSideCount) + ((100 / kSideCount) * _sideTickCount / kTicksPerSide);
        _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side calibration: progress <%2>").arg(QLatin1String(kSideNames[_activeSide])).arg(progress));
        return;
    }

    _sideDone[_activeSide] = true;
    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side done, rotate to a different side").arg(QLatin1String(kSideNames[_activeSide])));
    _activeSide = -1;
}

/// Accel side: hold-still sampling phase with no progress messages, then the
/// side result and a single progress jump (see PX4 accel_calibration_worker()).
void MockLinkPX4Calibration::_accelSideTick()
{
    if (_sideTickCount < kTicksPerSide) {
        return;
    }

    _sideDone[_activeSide] = true;
    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side result: %2").arg(QLatin1String(kSideNames[_activeSide]), QLatin1String(kSideAccelResults[_activeSide])));
    // Same progress calculation as PX4 accel_calibration_worker(): 17 * doneCount.
    // Intentionally reaches 102 after the final side, matching firmware; the UI
    // progress bar clamps it to 100%.
    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <%1>").arg(17 * _doneCount()));
    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side done, rotate to a different side").arg(QLatin1String(kSideNames[_activeSide])));
    _activeSide = -1;
}
