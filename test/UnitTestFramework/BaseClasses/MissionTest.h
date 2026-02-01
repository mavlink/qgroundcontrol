#pragma once

#include "MAVLinkLib.h"
#include "UnitTest.h"
#include "VehicleTest.h"

class PlanMasterController;
class MissionController;
class GeoFenceController;
class RallyPointController;

/// @file
/// @brief Base classes for mission-related tests

/// Test fixture for mission planning tests. Provides a connected vehicle
/// with PlanMasterController and MissionController ready to use.
///
/// Example usage:
/// @code
/// class MyMissionTest : public MissionTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testMissionUpload() {
///         missionController()->insertSimpleMissionItem(coord, 1);
///         QVERIFY(missionController()->dirty());
///     }
/// };
/// @endcode
class MissionTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit MissionTest(QObject* parent = nullptr);
    ~MissionTest() override = default;

    /// Returns the plan master controller
    PlanMasterController* planController() const
    {
        return _planController;
    }

    /// Returns the mission controller
    MissionController* missionController() const;

    /// Returns the geo-fence controller
    GeoFenceController* geoFenceController() const;

    /// Returns the rally point controller
    RallyPointController* rallyPointController() const;

protected slots:
    void init() override;
    void cleanup() override;

protected:
    /// Clears the current mission
    void clearMission();

    /// Clears all plan data (mission, geofence, rally points)
    void clearAllPlanData();

    /// Sets whether to use fly view mode (default: false = plan view)
    void setFlyView(bool flyView)
    {
        _flyView = flyView;
    }

private:
    PlanMasterController* _planController = nullptr;
    bool _flyView = false;
};

/// Test fixture for offline mission editing (no vehicle connection).
/// Provides PlanMasterController configured for offline editing.
///
/// Example usage:
/// @code
/// class MyOfflineTest : public OfflineMissionTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testOfflineEditing() {
///         missionController()->insertSimpleMissionItem(coord, 1);
///         // No vehicle required
///     }
/// };
/// @endcode
class OfflineMissionTest : public UnitTest
{
    Q_OBJECT

public:
    explicit OfflineMissionTest(QObject* parent = nullptr);
    ~OfflineMissionTest() override = default;

    /// Returns the plan master controller
    PlanMasterController* planController() const
    {
        return _planController;
    }

    /// Returns the mission controller
    MissionController* missionController() const;

    /// Returns the geo-fence controller
    GeoFenceController* geoFenceController() const;

    /// Returns the rally point controller
    RallyPointController* rallyPointController() const;

protected slots:
    void init() override;
    void cleanup() override;

protected:
    /// Sets the offline firmware type
    void setOfflineFirmwareType(MAV_AUTOPILOT firmwareType)
    {
        _offlineFirmwareType = firmwareType;
    }

    /// Sets the offline vehicle type
    void setOfflineVehicleType(MAV_TYPE vehicleType)
    {
        _offlineVehicleType = vehicleType;
    }

    /// Clears the current mission
    void clearMission();

private:
    PlanMasterController* _planController = nullptr;
    MAV_AUTOPILOT _offlineFirmwareType = MAV_AUTOPILOT_PX4;
    MAV_TYPE _offlineVehicleType = MAV_TYPE_QUADROTOR;
};
