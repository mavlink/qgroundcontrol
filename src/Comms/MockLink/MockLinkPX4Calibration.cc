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

MockLinkPX4Calibration::MockLinkPX4Calibration(MockLink *mockLink)
    : _mockLink(mockLink)
{
}

void MockLinkPX4Calibration::startMagCalibration()
{
    QMutexLocker locker(&_mutex);

    _magCalActive = true;
    _requestedPose = -1;
    _activeSide = -1;
    _sideTickCount = 0;
    _finishTickCount = -1;
    std::fill(std::begin(_sideDone), std::end(_sideDone), false);

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] calibration started: 2 mag"));
}

bool MockLinkPX4Calibration::cancel()
{
    QMutexLocker locker(&_mutex);

    if (!_magCalActive) {
        return false;
    }

    _magCalActive = false;
    _requestedPose = -1;
    _activeSide = -1;
    _finishTickCount = -1;

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_CRITICAL, QStringLiteral("[cal] calibration cancelled"));
    return true;
}

bool MockLinkPX4Calibration::magCalActive() const
{
    QMutexLocker locker(&_mutex);
    return _magCalActive;
}

void MockLinkPX4Calibration::setPose(Pose pose)
{
    QMutexLocker locker(&_mutex);
    _requestedPose = static_cast<int>(pose);
}

void MockLinkPX4Calibration::run10HzTasks()
{
    QMutexLocker locker(&_mutex);

    if (!_magCalActive) {
        return;
    }

    if (_finishTickCount >= 0) {
        // Finishing phase: simulates the firmware calculating the final mag fit
        // before reporting completion. Gives the UI time to display the last
        // side as completed while the calibration is still active.
        _finishTickCount++;
        if (_finishTickCount >= kTicksToFinish) {
            _magCalActive = false;
            _finishTickCount = -1;
            _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <100>"));
            _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] calibration done: mag"));
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
    _activeSide = pose;
    _sideTickCount = 0;
}

/// Side in progress: simulate the rotation/sampling phase with progress messages,
/// then complete the side. Completing the last side finishes the calibration.
void MockLinkPX4Calibration::_sideTick()
{
    _sideTickCount++;

    if (_sideTickCount < kTicksPerSide) {
        // Same progress calculation as PX4 mag_calibration_worker()
        const unsigned progress = (_doneCount() * 100 / kSideCount) + ((100 / kSideCount) * _sideTickCount / kTicksPerSide);
        _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side calibration: progress <%2>").arg(QLatin1String(kSideNames[_activeSide])).arg(progress));
        return;
    }

    _sideDone[_activeSide] = true;
    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] %1 side done, rotate to a different side").arg(QLatin1String(kSideNames[_activeSide])));
    _activeSide = -1;

    if (_doneCount() == kSideCount) {
        // All sides collected: enter the finishing phase
        _finishTickCount = 0;
    }
}
