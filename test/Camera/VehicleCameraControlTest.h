#pragma once

#include "UnitTest.h"

namespace TestFixtures {
class LocalHttpTestServer;
}
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
    void _testRefusedDefinitionUriFallsBack();
    UT_PARAMETERIZED_TEST(_testDefinitionWithoutSaveDirectory);
    UT_PARAMETERIZED_TEST(_testCamerasOfOneModelDownloadTogether);
    UT_PARAMETERIZED_TEST(_testFtpDefinitionDownloadFailureFallsBack);
    UT_PARAMETERIZED_TEST(_testFtpDefinitionDownloadIsSavedAsCacheFile);
    UT_PARAMETERIZED_TEST(_testHttpDefinitionIsHeldToSizeAndDecompressed);
    UT_PARAMETERIZED_TEST(_testDefinitionIsReadAndParsedOnWorker);
    UT_PARAMETERIZED_TEST(_testDefinitionIsLocalised);

private:
    void _connectVehicle(MockConfiguration* mockConfig);
    /// The mock vehicle's first camera (MAV_COMP_ID_CAMERA), once both of its cameras are discovered; null if they
    /// aren't in time.
    MavlinkCameraControlInterface* _firstCamera();
    /// Makes @p content the file "/camera.xml.gz" of @p server, which it starts, if @p http, or else of the vehicle
    /// over MAVLink FTP, from @p servedDir @return The URI of the file, empty if it could not be served
    QString _serveDefinition(bool http, const QByteArray& content, TestFixtures::LocalHttpTestServer& server,
                             const QString& servedDir);

    MockLink* _mockLink = nullptr;
    Vehicle* _vehicle = nullptr;
};
