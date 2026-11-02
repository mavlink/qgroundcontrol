#include "UVCCameraControllerTest.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "UVCCameraController.h"
#include "UVCFrameRelay.h"
#include "UVCStreamMonitor.h"
#include "VideoTestHelpers.h"

using VideoTestHelpers::createVideoOutput;
using VideoTestHelpers::showFrame;

namespace {

constexpr const char* kMissingCamera = "QGC Test Missing Camera";

/// 64 × 48, red in the top-left quadrant only.
QImage redTopLeftQuadrant()
{
    QImage image(QSize(64, 48), QImage::Format_RGBA8888);
    image.fill(Qt::blue);
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 32; ++x) {
            image.setPixelColor(x, y, Qt::red);
        }
    }
    return image;
}

/// Whether @p image is redTopLeftQuadrant() turned clockwise, which puts the red quadrant top right, and mirrored,
/// which brings it back to the top left.
bool showsTurnedMirroredQuadrant(const QImage& image)
{
    if (image.size() != QSize(48, 64)) {
        return false;
    }
    const auto isRed = [&image](int x, int y) {
        const QColor color = image.pixelColor(x, y);
        return (color.red() > 200) && (color.blue() < 50);
    };
    return isRed(10, 10) && !isRed(40, 10) && !isRed(10, 50) && !isRed(40, 50);
}

}  // namespace

void UVCCameraControllerTest::_testScreenshotFromVideoOutput()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString shot = tempDir.filePath(QStringLiteral("uvc.png"));
    const QRegularExpression noOutput(QStringLiteral("^No video output to take a screenshot from"));

    UVCCameraController controller;
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, noOutput);
    QVERIFY(!controller.takeScreenshot(shot));
    verifyExpectedLogMessage();

    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());
    // Without a camera nothing runs, so VideoManager takes its screenshots elsewhere.
    QVERIFY(!controller.canTakeScreenshot());

    QImage frame(QSize(64, 48), QImage::Format_RGBA8888);
    frame.fill(Qt::red);
    showFrame(output.get(), frame);
    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(controller.takeScreenshot(shot));
    QCOMPARE(QImage(shot).size(), frame.size());

    // Another output's going away leaves the registered one alone.
    const std::unique_ptr<QQuickItem> otherOutput = createVideoOutput(engine);
    controller.unregisterVideoOutput(otherOutput.get());
    QVERIFY(controller.takeScreenshot(shot));

    controller.unregisterVideoOutput(output.get());
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, noOutput);
    QVERIFY(!controller.takeScreenshot(shot));
    verifyExpectedLogMessage();
}

void UVCCameraControllerTest::_testFramesFollowTransform()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString shot = tempDir.filePath(QStringLiteral("uvc.png"));

    UVCCameraController controller;
    controller.setTransform(QtVideo::Rotation::Clockwise90, true);
    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());

    // The camera's frame reaches the output turned and mirrored, and the output keeps its own orientation, so what Qt
    // draws over the frame stays upright.
    controller._frameRelay->input()->setVideoFrame(QVideoFrame(redTopLeftQuadrant()));
    auto* const sink = output->property("videoSink").value<QVideoSink*>();
    QVERIFY(sink);
    const QVideoFrame shown = sink->videoFrame();
    QCOMPARE(shown.rotation(), QtVideo::Rotation::Clockwise90);
    QVERIFY(shown.mirrored());
    QCOMPARE(output->property("orientation").toInt(), 0);
    QVERIFY(!output->property("mirrored").toBool());

    // Screenshots save it as displayed.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(controller.takeScreenshot(shot));
    QVERIFY(showsTurnedMirroredQuadrant(QImage(shot)));
}

void UVCCameraControllerTest::_testPhotoFollowsTransform()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString photo = tempDir.filePath(QStringLiteral("photo.png"));

    UVCCameraController controller;
    controller.setTransform(QtVideo::Rotation::Clockwise90, true);
    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());

    // A captured frame, as QImageCapture hands it over for request 7, is saved as displayed.
    controller._photoFiles.insert(7, {photo, {}});
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    controller._onImageAvailable(7, QVideoFrame(redTopLeftQuadrant()));
    QVERIFY(controller._photoFiles.isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo(photo).size() > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(showsTurnedMirroredQuadrant(QImage(photo)), 5000);

    // Frames of requests this controller didn't make are ignored.
    controller._onImageAvailable(8, QVideoFrame(redTopLeftQuadrant()));
}

void UVCCameraControllerTest::_testPhotoWithoutCameraIsScreenshot()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString photo = tempDir.filePath(QStringLiteral("photo.png"));

    UVCCameraController controller;
    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());
    showFrame(output.get(), redTopLeftQuadrant());
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));

    // No camera captures, so the frame on display is saved, off the GUI thread.
    controller.capturePhoto(photo);
    QVERIFY(controller._photoFiles.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(QImage(photo).size(), QSize(64, 48), 5000);

    // A failed capture falls back the same way.
    const QString failed = tempDir.filePath(QStringLiteral("failed.png"));
    controller._photoFiles.insert(9, {failed, {}});
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Photo capture failed")));
    controller._onImageCaptureError(9, QStringLiteral("Camera busy"));
    verifyExpectedLogMessage();
    QTRY_COMPARE_WITH_TIMEOUT(QImage(failed).size(), QSize(64, 48), 5000);

    // Without an output there is nothing to save.
    controller.unregisterVideoOutput(output.get());
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No video output to take a photo from")));
    controller.capturePhoto(tempDir.filePath(QStringLiteral("none.png")));
    verifyExpectedLogMessage();
}

void UVCCameraControllerTest::_testControlsWithoutCamera()
{
    UVCCameraController controller;
    QVERIFY(!controller.whiteBalanceAdjustable());
    QCOMPARE(controller.minimumZoom(), 1.0);
    QCOMPARE(controller.maximumZoom(), 1.0);
    QVERIFY(!controller.exposureAdjustable());
    QVERIFY(!controller.focusAdjustable());

    // Kept for the camera to come.
    const UVCCameraController::Controls controls{.whiteBalance = QCamera::WhiteBalanceSunlight, .zoom = 2.f};
    controller.setControls(controls);
    QCOMPARE(controller.controls(), controls);
}

void UVCCameraControllerTest::_testMissingCameraNeverRuns()
{
    UVCCameraController controller;
    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());

    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    controller.setDevice(QString::fromLatin1(kMissingCamera));
    verifyExpectedLogMessage();
    QCOMPARE(controller.device(), QString::fromLatin1(kMissingCamera));
    QVERIFY(!controller._camera);
    QVERIFY(!controller._captureSession);
    QVERIFY(!controller.canTakeScreenshot());
    QVERIFY(!controller.monitor()->stats().valid);

    controller.setDevice(QString());
    QVERIFY(controller.device().isEmpty());
}

void UVCCameraControllerTest::_testUnpluggedCameraReported()
{
    UVCCameraController controller;
    QSignalSpy camerasSpy(&controller, &UVCCameraController::camerasChanged);

    // Without a camera in use, a change of cameras is only announced.
    controller._onVideoInputsChanged();
    QCOMPARE(camerasSpy.count(), 1);

    // The camera in use, identified by its device id, is no longer attached: it was unplugged.
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    controller.setDevice(QString::fromLatin1(kMissingCamera));
    controller._cameraId = QByteArrayLiteral("qgc-test-unplugged");
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("Camera unplugged")));
    expectAppMessage(QRegularExpression(QStringLiteral("USB camera QGC Test Missing Camera was disconnected")));
    controller._onVideoInputsChanged();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(camerasSpy.count(), 2);
    QVERIFY(controller._cameraId.isEmpty());

    // Reported once: the camera is gone now.
    controller._onVideoInputsChanged();
    QCOMPARE(camerasSpy.count(), 3);
}

void UVCCameraControllerTest::_testCameraErrorRetries()
{
    UVCCameraController controller;
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("^Camera error")));
    controller.setDevice(QString::fromLatin1(kMissingCamera));
    const QString error = QStringLiteral("Camera is in use");

    // The first error of a run is reported, and the camera is recreated later, not from its own signal.
    expectAppMessage(
        QRegularExpression(QStringLiteral("USB camera QGC Test Missing Camera: Camera is in use. Retrying.")));
    controller._onCameraError(error);
    verifyExpectedLogMessage();
    QVERIFY(controller._retryTimer->isActive());
    QCOMPARE(controller._retryTimer->interval(), UVCCameraController::kRetryIntervalMs);

    // Later errors of the run retry silently, up to kMaxRetries.
    for (int retry = 2; retry <= UVCCameraController::kMaxRetries; ++retry) {
        controller._retryTimer->stop();
        controller._recreateCamera();
        controller._onCameraError(error);
        QCOMPARE(controller._retries, retry);
        QVERIFY(controller._retryTimer->isActive());
    }

    // Then the controller gives up and says so.
    controller._retryTimer->stop();
    expectAppMessage(
        QRegularExpression(QStringLiteral(R"(USB camera QGC Test Missing Camera: Camera is in use(?!\. Retrying))")));
    controller._onCameraError(error);
    verifyExpectedLogMessage();
    QVERIFY(!controller._retryTimer->isActive());

    // Selecting a camera starts a new run.
    controller.setDevice(QString());
    QCOMPARE(controller._retries, 0);
}

void UVCCameraControllerTest::_testRecordingNeedsRunningCamera()
{
    UVCCameraController controller;
    QSignalSpy finishedSpy(&controller, &UVCCameraController::recordingFinished);
    QSignalSpy recordingSpy(&controller, &UVCCameraController::recordingChanged);

    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("Can't record")));
    controller.startRecording(QStringLiteral("/tmp/uvc.mp4"), QMediaFormat::MPEG4);
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().at(0).toString(), QStringLiteral("/tmp/uvc.mp4"));
    QVERIFY(!finishedSpy.first().at(1).toString().isEmpty());
    QCOMPARE(recordingSpy.count(), 0);
    QVERIFY(!controller.recording());
    QVERIFY(!controller._recorder->active());
}

UT_REGISTER_TEST(UVCCameraControllerTest, TestLabel::Unit)
