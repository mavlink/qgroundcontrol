#include "FlyViewCameraZoomUITest.h"

#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "MavlinkCameraControlInterface.h"
#include "MockLink.h"
#include "QGCCameraManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(FlyViewCameraZoomUITest, TestLabel::Integration)

void FlyViewCameraZoomUITest::_testZoomSliderTracksCameraWithoutEcho()
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
            QVERIFY(camera->hasZoom());
            QVERIFY_TRUE_WAIT(qFuzzyCompare(camera->zoomLevel(), 1.0), TestTimeout::longMs());

            // PhotoVideoControl shows the current camera, which is whichever mock camera announced first
            cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
            QCOMPARE(cameraManager->currentCameraInstance(), camera);

            const QString sliderName = QStringLiteral("photoVideoControl_zoomSlider");
            QVERIFY2(findVisibleItem(_rootItem, sliderName, TestTimeout::mediumMs()),
                     "Zoom slider never became visible");
            QVERIFY(verifyProperty(sliderName, "value", 1.0, QStringLiteral("initial zoom")));

            auto zoomCommandCount = [&mockLink] {
                return mockLink->receivedMavCommandCount(MAV_CMD_SET_CAMERA_ZOOM, MAV_COMP_ID_CAMERA);
            };
            const int baseline = zoomCommandCount();

            // Joystick-style step zoom: the camera moves to a level QGC did not choose, and
            // QGC learns about it via the CAMERA_SETTINGS refresh that follows the ack.
            camera->stepZoom(1);
            QVERIFY_TRUE_WAIT(zoomCommandCount() >= baseline + 1, TestTimeout::mediumMs());
            QVERIFY_TRUE_WAIT(camera->zoomLevel() > 1.0, TestTimeout::mediumMs());
            const qreal steppedLevel = camera->zoomLevel();
            QVERIFY(verifyProperty(sliderName, "value", steppedLevel, QStringLiteral("stepped zoom")));

            // The slider following the reported level must not send it back to the camera.
            QVERIFY2(
                !QTest::qWaitFor([&] { return zoomCommandCount() > baseline + 1; }, TestTimeout::shortMs()),
                qPrintable(QStringLiteral(
                               "Slider echoed camera-reported zoom back as SET_CAMERA_ZOOM (%1 commands after step)")
                               .arg(zoomCommandCount() - baseline)));

            // User interaction with the slider must still command the camera: clicking the
            // groove moves the handle there and the camera ends up at the slider's value.
            QVERIFY(clickItemFraction(sliderName, 0.5, 0.5));
            QVERIFY_TRUE_WAIT(zoomCommandCount() == baseline + 2, TestTimeout::mediumMs());
            QQuickItem* const slider = findVisibleItem(_rootItem, sliderName);
            QVERIFY(slider);
            const qreal sliderLevel = slider->property("value").toReal();
            QVERIFY(!qFuzzyCompare(sliderLevel, steppedLevel));
            // Level round-trips through a MAVLink float
            QVERIFY_TRUE_WAIT(qAbs(camera->zoomLevel() - sliderLevel) < 0.001, TestTimeout::mediumMs());
        });
}
