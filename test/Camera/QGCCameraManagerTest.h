#pragma once

#include "TestFixtures.h"

/// Unit tests for QGCCameraManager and CameraMetaData.
/// Tests camera metadata parsing and validation.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class QGCCameraManagerTest : public OfflineTest
{
    Q_OBJECT

public:
    QGCCameraManagerTest() = default;

private slots:
    // Parsing tests
    void _testCameraListParsing();
    void _testCameraListNotEmpty();

    // Validation tests
    void _testCameraPropertiesValid();
    void _testCameraSensorDimensions();
    void _testCameraImageDimensions();
    void _testCameraFocalLength();
    void _testCameraBrandsExist();
};
