#include "FlyViewCameraCaptureUITest.h"

#include <QtTest/QTest>

#include "MavlinkCameraControlInterface.h"
#include "MockLink.h"
#include "QGCCameraManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(FlyViewCameraCaptureUITest, TestLabel::Integration)

void FlyViewCameraCaptureUITest::_testTimelapseShutterStopsCapture()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableCamera); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            QGCCameraManager* const cameraManager = vehicle->cameraManager();
            QVERIFY(cameraManager);

            auto findCamera = [cameraManager]() -> MavlinkCameraControlInterface* {
                for (int i = 0; i < cameraManager->cameras()->count(); i++) {
                    auto* const cam = qobject_cast<MavlinkCameraControlInterface*>(cameraManager->cameras()->get(i));
                    if (cam && (cam->compID() == MAV_COMP_ID_CAMERA)) {
                        return cam;
                    }
                }
                return nullptr;
            };
            MavlinkCameraControlInterface* camera = nullptr;
            QVERIFY_TRUE_WAIT((camera = findCamera()) != nullptr, TestTimeout::longMs());
            QVERIFY(camera->capturesPhotos());
            QVERIFY_TRUE_WAIT(camera->capturePhotosState() == MavlinkCameraControlInterface::CapturePhotosStateIdle,
                              TestTimeout::longMs());

            cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
            QCOMPARE(cameraManager->currentCameraInstance(), camera);

            // Mock camera 1 starts in video mode; the shutter button is only shown in photo mode
            camera->setCameraModePhoto();
            QVERIFY_TRUE_WAIT(camera->cameraMode() == MavlinkCameraControlInterface::CAM_MODE_PHOTO,
                              TestTimeout::mediumMs());

            camera->setPhotoCaptureMode(MavlinkCameraControlInterface::PHOTO_CAPTURE_TIMELAPSE);
            camera->setPhotoLapse(1.0);
            camera->setPhotoLapseCount(0);

            const QString buttonName = QStringLiteral("photoVideoControl_photoCaptureButton");
            QVERIFY2(findVisibleItem(_rootItem, buttonName, TestTimeout::mediumMs()),
                     "Photo capture button never became visible");

            auto startCount = [&mockLink] {
                return mockLink->receivedMavCommandCount(MAV_CMD_IMAGE_START_CAPTURE, MAV_COMP_ID_CAMERA);
            };
            auto stopCount = [&mockLink] {
                return mockLink->receivedMavCommandCount(MAV_CMD_IMAGE_STOP_CAPTURE, MAV_COMP_ID_CAMERA);
            };
            QCOMPARE(startCount(), 0);
            QCOMPARE(stopCount(), 0);

            QVERIFY(clickItemFraction(buttonName, 0.5, 0.5));
            QVERIFY_TRUE_WAIT(startCount() == 1, TestTimeout::mediumMs());
            // Camera broadcasts CAMERA_IMAGE_CAPTURED per interval shot, which drives the photo counter
            QVERIFY_TRUE_WAIT(vehicle->cameraTriggerPoints()->count() >= 1, TestTimeout::mediumMs());

            // Second click while the interval capture is running must stop it
            QVERIFY(clickItemFraction(buttonName, 0.5, 0.5));
            QVERIFY_TRUE_WAIT(stopCount() == 1, TestTimeout::mediumMs());
            QVERIFY_TRUE_WAIT(camera->capturePhotosState() == MavlinkCameraControlInterface::CapturePhotosStateIdle,
                              TestTimeout::mediumMs());
        });
}
