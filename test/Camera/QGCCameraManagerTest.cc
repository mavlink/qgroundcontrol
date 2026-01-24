#include "QGCCameraManagerTest.h"
#include "QGCCameraManager.h"
#include "CameraMetaData.h"

#include <QtCore/QSet>
#include <QtTest/QTest>

// ============================================================================
// Parsing Tests
// ============================================================================

void QGCCameraManagerTest::_testCameraListParsing()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    // Verify parsing succeeded and returned a list
    QVERIFY(!cameraList.isEmpty());

    // Verify each camera was allocated
    for (const CameraMetaData* camera : cameraList) {
        VERIFY_NOT_NULL(camera);
    }

    qDeleteAll(cameraList);
}

void QGCCameraManagerTest::_testCameraListNotEmpty()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();
    QGC_VERIFY_NOT_EMPTY(cameraList);

    // Expect at least a few cameras in the database
    QCOMPARE_GE(cameraList.count(), 5);

    qDeleteAll(cameraList);
}

// ============================================================================
// Validation Tests
// ============================================================================

void QGCCameraManagerTest::_testCameraPropertiesValid()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    for (const CameraMetaData* camera : cameraList) {
        // Each camera must have a canonical name
        QVERIFY2(!camera->canonicalName.isEmpty(),
                 qPrintable(QStringLiteral("Camera with empty canonical name")));

        // Each camera must have a model
        QVERIFY2(!camera->model.isEmpty(),
                 qPrintable(QStringLiteral("Camera %1 has empty model").arg(camera->canonicalName)));
    }

    qDeleteAll(cameraList);
}

void QGCCameraManagerTest::_testCameraSensorDimensions()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    for (const CameraMetaData* camera : cameraList) {
        // Sensor dimensions must be positive (in millimeters)
        QVERIFY2(camera->sensorWidth > 0.0,
                 qPrintable(QStringLiteral("Camera %1 has invalid sensorWidth: %2")
                            .arg(camera->canonicalName).arg(camera->sensorWidth)));

        QVERIFY2(camera->sensorHeight > 0.0,
                 qPrintable(QStringLiteral("Camera %1 has invalid sensorHeight: %2")
                            .arg(camera->canonicalName).arg(camera->sensorHeight)));

        // Sensor dimensions should be reasonable (0.1mm to 200mm typical range)
        // Note: Some thermal cameras like Flir Duo R have larger sensors (~160mm)
        QVERIFY2(camera->sensorWidth < 200.0,
                 qPrintable(QStringLiteral("Camera %1 has unusually large sensorWidth: %2")
                            .arg(camera->canonicalName).arg(camera->sensorWidth)));

        QVERIFY2(camera->sensorHeight < 200.0,
                 qPrintable(QStringLiteral("Camera %1 has unusually large sensorHeight: %2")
                            .arg(camera->canonicalName).arg(camera->sensorHeight)));
    }

    qDeleteAll(cameraList);
}

void QGCCameraManagerTest::_testCameraImageDimensions()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    for (const CameraMetaData* camera : cameraList) {
        // Image dimensions must be positive (in pixels)
        QVERIFY2(camera->imageWidth > 0.0,
                 qPrintable(QStringLiteral("Camera %1 has invalid imageWidth: %2")
                            .arg(camera->canonicalName).arg(camera->imageWidth)));

        QVERIFY2(camera->imageHeight > 0.0,
                 qPrintable(QStringLiteral("Camera %1 has invalid imageHeight: %2")
                            .arg(camera->canonicalName).arg(camera->imageHeight)));

        // Image dimensions should be at least 100 pixels (reasonable minimum)
        QVERIFY2(camera->imageWidth >= 100.0,
                 qPrintable(QStringLiteral("Camera %1 has unusually small imageWidth: %2")
                            .arg(camera->canonicalName).arg(camera->imageWidth)));

        QVERIFY2(camera->imageHeight >= 100.0,
                 qPrintable(QStringLiteral("Camera %1 has unusually small imageHeight: %2")
                            .arg(camera->canonicalName).arg(camera->imageHeight)));
    }

    qDeleteAll(cameraList);
}

void QGCCameraManagerTest::_testCameraFocalLength()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    for (const CameraMetaData* camera : cameraList) {
        // Focal length must be positive (in millimeters)
        QVERIFY2(camera->focalLength > 0.0,
                 qPrintable(QStringLiteral("Camera %1 has invalid focalLength: %2")
                            .arg(camera->canonicalName).arg(camera->focalLength)));

        // Focal length should be reasonable (1mm to 1000mm typical range)
        QVERIFY2(camera->focalLength < 1000.0,
                 qPrintable(QStringLiteral("Camera %1 has unusually large focalLength: %2")
                            .arg(camera->canonicalName).arg(camera->focalLength)));
    }

    qDeleteAll(cameraList);
}

void QGCCameraManagerTest::_testCameraBrandsExist()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();

    QSet<QString> brands;
    for (const CameraMetaData* camera : cameraList) {
        if (!camera->brand.isEmpty()) {
            brands.insert(camera->brand);
        }
    }

    // Should have multiple camera brands
    QCOMPARE_GE(brands.count(), 3);

    qDeleteAll(cameraList);
}
