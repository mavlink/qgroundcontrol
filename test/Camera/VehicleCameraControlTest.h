#pragma once

#include "UnitTest.h"

class MavlinkCameraControlInterface;
class MockConfiguration;
class MockLink;
class Vehicle;

class VehicleCameraControlTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void init() override;
    void cleanup() override;

    UT_PARAMETERIZED_TEST(_testCameraCapFlags);
    void _testZoomTriggersCameraSettingsRequest();
    void _testExtraStreamsStartAndStop();
    void _testThermalStreamStartsOnceNamed_data();
    void _testThermalStreamStartsOnceNamed();
    void _testStreamAnnouncedAnewIsUpdated();
    void _testNameFieldReadIsBounded();
    void _testCameraNamesCannotSteerCachePath();
    void _testOrdinaryCameraNamesUnchanged();
    void _testDistinctCameraNamesDoNotShareCacheFile();
    void _testConstructorBuildsCacheFileFromBoundedNames();
    void _testFtpDefinitionDownloadRefusedFallsBack();
    UT_PARAMETERIZED_TEST(_testFtpDefinitionDownloadIsSavedAsCacheFile);

private:
    void _connectVehicle(MockConfiguration* mockConfig);
    /// The mock vehicle's first camera (MAV_COMP_ID_CAMERA), once both of its cameras are discovered; null if they
    /// aren't in time.
    MavlinkCameraControlInterface* _firstCamera();

    MockLink* _mockLink = nullptr;
    Vehicle* _vehicle = nullptr;
};
