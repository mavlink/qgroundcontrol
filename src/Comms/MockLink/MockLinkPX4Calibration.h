#pragma once

#include <QtCore/QMutex>

class MockLink;

/// \brief Simulates the PX4 commander magnetometer and accelerometer calibration protocols for MockLink.
///
/// Replays the [cal] STATUSTEXT protocol emitted by the PX4 firmware
/// (src/modules/commander/mag_calibration.cpp, accelerometer_calibration.cpp and
/// calibration_routines.cpp). Magnetometer:
///
///     [cal] calibration started: 2 mag
///     [cal] <side> orientation detected
///     [cal] <side> side calibration: progress <N>
///     [cal] <side> side done, rotate to a different side
///     [cal] progress <100>
///     [cal] calibration done: mag
///
/// Accelerometer:
///
///     [cal] calibration started: 2 accel
///     [cal] <side> orientation detected
///     [cal] Hold still, measuring <side> side
///     [cal] <side> side result: [x y z]
///     [cal] progress <17*doneCount>
///     [cal] <side> side done, rotate to a different side
///     [cal] calibration done: accel
///
/// The simulation is pose-driven: a unit test places the simulated vehicle into a
/// pose with setPose(). On subsequent 10Hz ticks the state machine detects the
/// orientation, simulates the rotation/sampling phase and completes the side.
/// Setting a pose for an already completed side emits
/// "[cal] <side> side already completed", matching firmware behavior.
///
/// All six sides are required, matching CAL_MAG_SIDES=63 in PX4MockLink.params.
///
/// An all-zero MAV_CMD_PREFLIGHT_CALIBRATION while a calibration is active cancels
/// it with "[cal] calibration cancelled" (see PX4 calibrate_cancel_check()).
class MockLinkPX4Calibration
{
public:
    /// Vehicle poses. Order matches the PX4 detect_orientation_return enum.
    enum class Pose {
        None = -1,
        TailDown = 0,   ///< "back"  accel [  g,  0,  0 ]
        NoseDown,       ///< "front" accel [ -g,  0,  0 ]
        Left,           ///< "left"  accel [  0,  g,  0 ]
        Right,          ///< "right" accel [  0, -g,  0 ]
        UpsideDown,     ///< "up"    accel [  0,  0,  g ]
        RightSideUp,    ///< "down"  accel [  0,  0, -g ]
    };
    static constexpr int kSideCount = 6;

    explicit MockLinkPX4Calibration(MockLink *mockLink);

    /// Starts simulated mag calibration: sends "[cal] calibration started: 2 mag"
    /// and resets the side state machine.
    void startMagCalibration();

    /// Starts simulated accel calibration: sends "[cal] calibration started: 2 accel"
    /// and resets the side state machine.
    void startAccelCalibration();

    /// Handles an all-zero MAV_CMD_PREFLIGHT_CALIBRATION (cancel request).
    ///     @return true if a calibration was active and has been cancelled
    bool cancel();

    bool calibrationActive() const;

    /// Test API: places the simulated vehicle into the given pose. The state
    /// machine advances on subsequent 10Hz ticks. Thread-safe.
    void setPose(Pose pose);

    /// Called by MockLink::run10HzTasks on the worker thread.
    void run10HzTasks();

private:
    enum class CalType {
        None,
        Mag,
        Accel,
    };

    void _startCalibration(CalType calType);
    void _sideTick();
    void _magSideTick();
    void _accelSideTick();
    void _detectPose();
    int _doneCount() const;

    MockLink *_mockLink = nullptr;

    mutable QMutex _mutex;
    CalType _calType = CalType::None;
    int _requestedPose = -1;            ///< Pose set by test, -1 when consumed/none
    int _activeSide = -1;               ///< Side currently calibrating, -1 when waiting for pose
    int _sideTickCount = 0;             ///< Ticks elapsed in the active side
    int _finishTickCount = -1;          ///< Ticks elapsed in the finishing phase, -1 when not finishing
    bool _sideDone[kSideCount] = {};

    static constexpr int kTicksPerSide = 5;     ///< 10Hz ticks to complete one side
    static constexpr int kTicksToFinish = 10;   ///< 10Hz ticks simulating the final calibration calculation
};
