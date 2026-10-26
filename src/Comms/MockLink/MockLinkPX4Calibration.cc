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
static constexpr int32_t kBaroDeviceId = 6619404;
static constexpr int32_t kGyroDeviceId = 2293772;
static constexpr float kLevelRollOffsetDeg = 1.5f;
static constexpr float kLevelPitchOffsetDeg = -0.8f;

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

void MockLinkPX4Calibration::startBaroCalibration(bool fails)
{
    {
        QMutexLocker locker(&_mutex);
        _baroFails = fails;
    }
    _startCalibration(CalType::Baro);
}

void MockLinkPX4Calibration::startLevelCalibration(bool fails)
{
    {
        QMutexLocker locker(&_mutex);
        _levelFails = fails;
    }
    _startCalibration(CalType::Level);
}

void MockLinkPX4Calibration::startGyroCalibration(bool fails)
{
    {
        QMutexLocker locker(&_mutex);
        _gyroFails = fails;
    }
    _startCalibration(CalType::Gyro);
}

void MockLinkPX4Calibration::setInlineCalibrationStalls(bool stalls)
{
    QMutexLocker locker(&_mutex);
    _inlineStalls = stalls;
}

QString MockLinkPX4Calibration::_calTypeName(CalType calType)
{
    switch (calType) {
        case CalType::Mag:
            return QStringLiteral("mag");
        case CalType::Accel:
            return QStringLiteral("accel");
        case CalType::Baro:
            return QStringLiteral("baro");
        case CalType::Level:
            return QStringLiteral("level");
        case CalType::Gyro:
            return QStringLiteral("gyro");
        case CalType::None:
            break;
    }
    return QString();
}

void MockLinkPX4Calibration::_startCalibration(CalType calType)
{
    QMutexLocker locker(&_mutex);

    _calType = calType;
    _requestedPose = -1;
    _activeSide = -1;
    _sideTickCount = 0;
    // The barometer, gyroscope and level calibrations have no sides: they go straight to the finishing phase
    _finishTickCount = (calType == CalType::Baro || calType == CalType::Level || calType == CalType::Gyro) ? 0 : -1;
    std::fill(std::begin(_sideDone), std::end(_sideDone), false);

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO,
                                     QStringLiteral("[cal] calibration started: 2 %1").arg(_calTypeName(calType)));
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

bool MockLinkPX4Calibration::fail()
{
    QMutexLocker locker(&_mutex);

    if (_calType == CalType::None) {
        return false;
    }

    const QString name = _calTypeName(_calType);
    _calType = CalType::None;
    _requestedPose = -1;
    _activeSide = -1;
    _finishTickCount = -1;

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_CRITICAL, QStringLiteral("[cal] calibration failed: %1").arg(name));
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
        if (_inlineStalls && (_calType == CalType::Baro || _calType == CalType::Level || _calType == CalType::Gyro)) {
            return;
        }
        _finishTickCount++;
        if (_finishTickCount >= kTicksToFinish) {
            const CalType calType = _calType;
            _calType = CalType::None;
            _finishTickCount = -1;
            _finishCalibration(calType);
        }
        return;
    }

    if (_activeSide >= 0) {
        _sideTick();
    } else {
        _detectPose();
    }
}

void MockLinkPX4Calibration::_finishCalibration(CalType calType)
{
    if (calType == CalType::Baro && _baroFails) {
        _mockLink->sendStatusTextMessage(MAV_SEVERITY_CRITICAL,
                                         QStringLiteral("[cal] calibration failed: GPS required for baro cal"));
        return;
    }

    if (calType == CalType::Level || calType == CalType::Gyro) {
        _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <100>"));
        const bool fails = (calType == CalType::Level) ? _levelFails : _gyroFails;
        if (fails) {
            // The firmware reports the reason as a plain message, then the failure
            const QString reason = (calType == CalType::Level) ? QStringLiteral("motion during calibration")
                                                               : QStringLiteral("ERROR: Motion during calibration");
            _mockLink->sendStatusTextMessage(MAV_SEVERITY_CRITICAL, reason);
            _mockLink->sendStatusTextMessage(MAV_SEVERITY_CRITICAL,
                                             QStringLiteral("[cal] calibration failed: %1").arg(_calTypeName(calType)));
            return;
        }
    }

    // Store the calibration result like the firmware does, before reporting
    // done so the subsequent param refresh by QGC picks it up
    switch (calType) {
        case CalType::Mag:
            _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAL_MAG0_ID"), kMagDeviceId);
            // The mag fit calculation reports its own final progress
            _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO, QStringLiteral("[cal] progress <100>"));
            break;
        case CalType::Accel:
            _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAL_ACC0_ID"), kAccelDeviceId);
            break;
        case CalType::Baro:
            _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAL_BARO0_ID"), kBaroDeviceId);
            break;
        case CalType::Level:
            _mockLink->setFloatParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("SENS_BOARD_X_OFF"),
                                          kLevelRollOffsetDeg);
            _mockLink->setFloatParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("SENS_BOARD_Y_OFF"),
                                          kLevelPitchOffsetDeg);
            break;
        case CalType::Gyro:
            _mockLink->setInt32ParamValue(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("CAL_GYRO0_ID"), kGyroDeviceId);
            break;
        case CalType::None:
            return;
    }

    _mockLink->sendStatusTextMessage(MAV_SEVERITY_INFO,
                                     QStringLiteral("[cal] calibration done: %1").arg(_calTypeName(calType)));
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
    case CalType::Baro:
    case CalType::Level:
    case CalType::Gyro:
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
