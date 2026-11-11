#include "UVCCameraControllerTest.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "LogManager.h"
#include "UVCCameraController.h"
#include "UVCStreamMonitor.h"
#include "VideoTestHelpers.h"

using VideoTestHelpers::createVideoOutput;
using VideoTestHelpers::redTopLeftQuadrant;
using VideoTestHelpers::showFrame;

namespace {

constexpr const char* kMissingCamera = "QGC Test Missing Camera";

constexpr const char* kUvcVideoOutputQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

UVCVideoOutput {
    width: 64
    height: 48
}
)QML";

/// Whether @p image is redTopLeftQuadrant() turned clockwise, which puts the red quadrant top right, and mirrored,
/// which brings it back to the top left.
bool showsTurnedMirroredQuadrant(const QImage& image)
{
    return (image.size() == QSize(48, 64)) && (VideoTestHelpers::redCorner(image) == Qt::TopLeftCorner);
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

void UVCCameraControllerTest::_testOutputFollowsTransform()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString shot = tempDir.filePath(QStringLiteral("uvc.png"));

    UVCCameraController controller;
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kUvcVideoOutputQml, QUrl(QStringLiteral("qrc:/UVCCameraControllerTest/UVCVideoOutput.qml")));
    const std::unique_ptr<QQuickItem> output(qobject_cast<QQuickItem*>(
        component.createWithInitialProperties({{QStringLiteral("camera"), QVariant::fromValue(&controller)}})));
    QVERIFY2(output, qPrintable(component.errorString()));
    QCOMPARE(controller._videoOutput, output.get());
    QCOMPARE(output->property("orientation").toInt(), 0);
    QVERIFY(!output->property("mirrored").toBool());

    // The output turns and mirrors the picture. The camera's frames, which its recorder gets too, stay as they are.
    controller.setTransform(QtVideo::Rotation::Clockwise90, true);
    QCOMPARE(output->property("orientation").toInt(), 90);
    QVERIFY(output->property("mirrored").toBool());
    showFrame(output.get(), redTopLeftQuadrant());
    auto* const sink = output->property("videoSink").value<QVideoSink*>();
    QVERIFY(sink);
    QCOMPARE(sink->videoFrame().rotation(), QtVideo::Rotation::None);
    QVERIFY(!sink->videoFrame().mirrored());

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

    // A photo that can't be saved is reported.
    expectLogMessage("Video.VideoScreenshot", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to save screenshot")));
    expectAppMessage(QRegularExpression(QStringLiteral("Unable to save the photo unwritable\\.png")));
    const auto appMessages = [] {
        return LogManager::capturedMessages(QStringLiteral("API.QGCApplication.AppMessage")).size();
    };
    const qsizetype appMessagesBefore = appMessages();
    controller.capturePhoto(tempDir.filePath(QStringLiteral("missing/unwritable.png")));
    QTRY_COMPARE_WITH_TIMEOUT(appMessages(), appMessagesBefore + 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // So is one there is no output to take it from.
    controller.unregisterVideoOutput(output.get());
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No video output to take a photo from")));
    expectAppMessage(QRegularExpression(QStringLiteral("no video is on display")));
    controller.capturePhoto(tempDir.filePath(QStringLiteral("none.png")));
    verifyExpectedLogMessage();
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
    QCOMPARE(controller._retryTimer->intervalAsDuration(),
             std::chrono::milliseconds(UVCCameraController::kRetryInterval));

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
