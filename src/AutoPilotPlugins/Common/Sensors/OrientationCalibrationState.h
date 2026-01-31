#pragma once

#include "SensorCalibrationSide.h"

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(OrientationCalibrationStateLog)

/// Tracks the state of all 6 sides for orientation-based sensor calibration.
/// This class manages the Done, Visible, InProgress, and Rotate states for each side.
/// Used by both APM and PX4 sensor calibration workflows.
class OrientationCalibrationState : public QObject
{
    Q_OBJECT

public:
    explicit OrientationCalibrationState(QObject* parent = nullptr);
    ~OrientationCalibrationState() override = default;

    /// Reset all side states to initial values
    /// @param allDone If true, set all sides to done (for gyro cal that only uses one side)
    void reset(bool allDone = false);

    /// Set which sides are visible based on a bitmask
    /// @param visibleMask Bitmask of visible sides (use SensorCalibrationSide::SideMask)
    void setVisibleSides(int visibleMask);

    /// Mark a side as in progress
    void setSideInProgress(CalibrationSide side, bool inProgress);

    /// Mark a side as done
    void setSideDone(CalibrationSide side);

    /// Mark a side as requiring rotation (for mag calibration)
    void setSideRotate(CalibrationSide side, bool rotate);

    /// Check if a side is done
    bool isSideDone(CalibrationSide side) const;

    /// Check if a side is visible
    bool isSideVisible(CalibrationSide side) const;

    /// Check if a side is in progress
    bool isSideInProgress(CalibrationSide side) const;

    /// Check if a side requires rotation
    bool isSideRotate(CalibrationSide side) const;

    /// Get the current side being calibrated (first in-progress side)
    CalibrationSide currentSide() const;

    /// Check if all visible sides are done
    bool allVisibleSidesDone() const;

    /// Get the number of completed sides
    int completedSideCount() const;

    /// Get the number of visible sides
    int visibleSideCount() const;

    // Individual side accessors for QML binding compatibility
    bool downSideDone() const { return _done[static_cast<int>(CalibrationSide::Down)]; }
    bool upSideDone() const { return _done[static_cast<int>(CalibrationSide::Up)]; }
    bool leftSideDone() const { return _done[static_cast<int>(CalibrationSide::Left)]; }
    bool rightSideDone() const { return _done[static_cast<int>(CalibrationSide::Right)]; }
    bool frontSideDone() const { return _done[static_cast<int>(CalibrationSide::Front)]; }
    bool backSideDone() const { return _done[static_cast<int>(CalibrationSide::Back)]; }

    bool downSideVisible() const { return _visible[static_cast<int>(CalibrationSide::Down)]; }
    bool upSideVisible() const { return _visible[static_cast<int>(CalibrationSide::Up)]; }
    bool leftSideVisible() const { return _visible[static_cast<int>(CalibrationSide::Left)]; }
    bool rightSideVisible() const { return _visible[static_cast<int>(CalibrationSide::Right)]; }
    bool frontSideVisible() const { return _visible[static_cast<int>(CalibrationSide::Front)]; }
    bool backSideVisible() const { return _visible[static_cast<int>(CalibrationSide::Back)]; }

    bool downSideInProgress() const { return _inProgress[static_cast<int>(CalibrationSide::Down)]; }
    bool upSideInProgress() const { return _inProgress[static_cast<int>(CalibrationSide::Up)]; }
    bool leftSideInProgress() const { return _inProgress[static_cast<int>(CalibrationSide::Left)]; }
    bool rightSideInProgress() const { return _inProgress[static_cast<int>(CalibrationSide::Right)]; }
    bool frontSideInProgress() const { return _inProgress[static_cast<int>(CalibrationSide::Front)]; }
    bool backSideInProgress() const { return _inProgress[static_cast<int>(CalibrationSide::Back)]; }

    bool downSideRotate() const { return _rotate[static_cast<int>(CalibrationSide::Down)]; }
    bool upSideRotate() const { return _rotate[static_cast<int>(CalibrationSide::Up)]; }
    bool leftSideRotate() const { return _rotate[static_cast<int>(CalibrationSide::Left)]; }
    bool rightSideRotate() const { return _rotate[static_cast<int>(CalibrationSide::Right)]; }
    bool frontSideRotate() const { return _rotate[static_cast<int>(CalibrationSide::Front)]; }
    bool backSideRotate() const { return _rotate[static_cast<int>(CalibrationSide::Back)]; }

signals:
    void sidesDoneChanged();
    void sidesVisibleChanged();
    void sidesInProgressChanged();
    void sidesRotateChanged();

private:
    static constexpr int SideCount = static_cast<int>(CalibrationSide::Count);

    bool _done[SideCount] = {false};
    bool _visible[SideCount] = {false};
    bool _inProgress[SideCount] = {false};
    bool _rotate[SideCount] = {false};
};
