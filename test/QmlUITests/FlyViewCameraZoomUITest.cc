#include "FlyViewCameraZoomUITest.h"

#include <QtCore/QScopeGuard>
#include <QtGui/QPointingDevice>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "Fact.h"
#include "MavlinkCameraControlInterface.h"
#include "MockLink.h"
#include "QGCCameraManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoSettings.h"

UT_REGISTER_TEST(FlyViewCameraZoomUITest, TestLabel::Integration)

namespace {

/// The virtual joysticks' thumb pads that are shown under @p item, in tree order.
void collectVirtualJoystickPads(QQuickItem* item, QList<QQuickItem*>& pads)
{
    if (!item->isVisible()) {
        return;
    }
    if (item->property("touchActive").isValid()) {
        pads.append(item);
        return;
    }
    for (QQuickItem* const child : item->childItems()) {
        collectVirtualJoystickPads(child, pads);
    }
}

}  // namespace

void FlyViewCameraZoomUITest::_selectZoomCamera(Vehicle* vehicle, MavlinkCameraControlInterface*& camera)
{
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
    QVERIFY_TRUE_WAIT((camera = findCamera()) != nullptr, TestTimeout::longMs());
    QVERIFY(camera->hasZoom());
    QVERIFY_TRUE_WAIT(qFuzzyCompare(camera->zoomLevel(), 1.0), TestTimeout::longMs());

    // The Fly View follows the current camera, which is whichever mock camera announced first
    cameraManager->setCurrentCamera(cameraManager->cameras()->indexOf(camera));
    QCOMPARE(cameraManager->currentCameraInstance(), camera);
}

void FlyViewCameraZoomUITest::_testZoomSliderTracksCameraWithoutEcho()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableCamera); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            MavlinkCameraControlInterface* camera = nullptr;
            _selectZoomCamera(vehicle, camera);
            if (QTest::currentTestFailed()) {
                return;
            }

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

void FlyViewCameraZoomUITest::_testPinchOnFullSizeVideoStepsZoom()
{
    // A configured video stream is what makes the Fly View offer the video
    Fact* const videoSource = SettingsManager::instance()->videoSettings()->videoSource();
    const auto restoreVideoSource =
        qScopeGuard([videoSource, saved = videoSource->rawValue()] { videoSource->setRawValue(saved); });
    videoSource->setRawValue(QString::fromLatin1(VideoSettings::videoSourceUDPH264));
    Fact* const virtualJoystick = SettingsManager::instance()->appSettings()->virtualJoystick();
    const auto restoreVirtualJoystick =
        qScopeGuard([virtualJoystick, saved = virtualJoystick->rawValue()] { virtualJoystick->setRawValue(saved); });
    virtualJoystick->setRawValue(true);

    runWithMockLink(
        [] { return MockLink::startPX4MockLink(MockConfiguration::OptionEnableCamera); },
        [this](QPointer<MockLink> mockLink, Vehicle* vehicle) {
            MavlinkCameraControlInterface* camera = nullptr;
            _selectZoomCamera(vehicle, camera);
            if (QTest::currentTestFailed()) {
                return;
            }

            // The video's mouse area handles presses only while the video has the full view
            const QString pipName = QStringLiteral("flyViewPipView");
            QQuickItem* const pipView = findVisibleItem(_rootItem, pipName, TestTimeout::mediumMs());
            QVERIFY2(pipView, "PiP view not visible");
            QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
            QQuickItem* const mouseArea = findItem(_rootItem, QStringLiteral("flyViewVideoMouseArea"));
            QVERIFY(mouseArea);
            QTRY_VERIFY(mouseArea->property("enabled").toBool());

            QObject* const clickTimer = mouseArea->findChild<QObject*>(QStringLiteral("flyViewVideoClickTimer"));
            QVERIFY(clickTimer);

            QPointingDevice* const touchScreen = QTest::createTouchDevice();
            // A real window merges touch moves until the next frame's sync; render two
            // frames per move so a sync is sure to follow the commit
            const auto commitTouchMove = [this](QTest::QTouchEventSequence& touch) {
                touch.commit();
                for (int frame = 0; frame < 2; frame++) {
                    QSignalSpy frameSpy(_window, &QQuickWindow::frameSwapped);
                    _window->update();
                    QVERIFY_SIGNAL_WAIT(frameSpy, TestTimeout::shortMs());
                }
            };
            const QPoint center(_window->width() / 2, _window->height() / 2);
            auto zoomCommandCount = [&mockLink] {
                return mockLink->receivedMavCommandCount(MAV_CMD_SET_CAMERA_ZOOM, MAV_COMP_ID_CAMERA);
            };
            const int baseline = zoomCommandCount();

            // The first finger down is a press of the mouse area's. The second one makes the two a pinch: the first
            // finger's movement is no drag, and lifting it no click
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchScreen, false);
                touch.press(0, center + QPoint(-50, 0)).commit();
                QTRY_VERIFY(mouseArea->property("pressed").toBool());
                touch.stationary(0).press(1, center + QPoint(50, 0)).commit();
                QTRY_VERIFY(mouseArea->property("_pinched").toBool());
                for (int i = 1; i <= 10; i++) {
                    commitTouchMove(
                        touch.move(0, center + QPoint(-50 - (i * 10), 0)).move(1, center + QPoint(50 + (i * 10), 0)));
                    if (QTest::currentTestFailed()) {
                        return;
                    }
                    QVERIFY(!mouseArea->property("_dragging").toBool());
                }
                touch.release(0, center + QPoint(-150, 0)).release(1, center + QPoint(150, 0)).commit();
            }
            QTRY_VERIFY(!mouseArea->property("pressed").toBool());
            QVERIFY(!clickTimer->property("running").toBool());
            QVERIFY_TRUE_WAIT(zoomCommandCount() > baseline, TestTimeout::mediumMs());
            QVERIFY_TRUE_WAIT(camera->zoomLevel() > 1.0, TestTimeout::mediumMs());

            // One finger is still the mouse area's: its movement is a drag
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchScreen, false);
                touch.press(0, center).commit();
                commitTouchMove(touch.move(0, center + QPoint(40, 0)));
                if (QTest::currentTestFailed()) {
                    return;
                }
                QTRY_VERIFY(mouseArea->property("_dragging").toBool());
                touch.release(0, center + QPoint(40, 0)).commit();
            }
            QTRY_VERIFY(!mouseArea->property("_dragging").toBool());
            QVERIFY(!clickTimer->property("running").toBool());

            // ... and its tap a click
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchScreen, false);
                touch.press(0, center + QPoint(0, 60)).commit();
                touch.release(0, center + QPoint(0, 60)).commit();
            }
            QTRY_VERIFY(clickTimer->property("running").toBool());

            // A touch that another item has is none of the pinch's. With a thumb on each virtual joystick, a finger on
            // the video is the mouse area's press, and each joystick still gets the release of its thumb
            QList<QQuickItem*> pads;
            collectVirtualJoystickPads(_rootItem, pads);
            QCOMPARE(pads.size(), 2);
            const auto padCenter = [](const QQuickItem* pad) {
                return pad->mapToScene(QPointF(pad->width() / 2, pad->height() / 2)).toPoint();
            };
            const QPoint thumbs[] = {padCenter(pads.at(0)), padCenter(pads.at(1))};
            const QPoint finger = center + QPoint(0, -80);
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchScreen, false);
                touch.press(0, thumbs[0]).commit();
                QTRY_VERIFY(pads.at(0)->property("touchActive").toBool());
                touch.stationary(0).press(1, thumbs[1]).commit();
                QTRY_VERIFY(pads.at(1)->property("touchActive").toBool());
                touch.stationary(0).stationary(1).press(2, finger).commit();
                QTRY_VERIFY(mouseArea->property("pressed").toBool());
                QVERIFY(!mouseArea->property("_pinched").toBool());
                touch.release(0, thumbs[0]).stationary(1).stationary(2).commit();
                QTRY_VERIFY(!pads.at(0)->property("touchActive").toBool());
                touch.release(1, thumbs[1]).stationary(2).commit();
                QTRY_VERIFY(!pads.at(1)->property("touchActive").toBool());
                touch.release(2, finger).commit();
            }
            QTRY_VERIFY(!mouseArea->property("pressed").toBool());
        });
}
