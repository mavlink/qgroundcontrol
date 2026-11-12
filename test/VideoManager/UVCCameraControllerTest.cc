#include "UVCCameraControllerTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

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

/// Selects the camera of description @p description, as VideoManager does.
void selectCamera(UVCCameraController& controller, const QString& description)
{
    VideoReceiver::Config config;
    config.uri = description;
    controller.setConfig(config);
    controller.setActive(!description.isEmpty());
}

}  // namespace

void UVCCameraControllerTest::_testScreenshotFromVideoOutput()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString shot = tempDir.filePath(QStringLiteral("uvc.png"));
    const QRegularExpression noOutput(QStringLiteral("^No video output to take a screenshot from"));

    UVCCameraController controller;
    expectLogMessage("Video.VideoReceiver", QtWarningMsg, noOutput);
    QVERIFY(!controller.takeScreenshot(shot));
    verifyExpectedLogMessage();

    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());
    QCOMPARE(controller.videoOutput(), output.get());

    QImage frame(QSize(64, 48), QImage::Format_RGBA8888);
    frame.fill(Qt::red);
    showFrame(output.get(), frame);
    // Without a camera nothing streams, whatever the output shows: VideoManager takes no screenshot of it.
    QVERIFY(!controller.streaming());
    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(controller.takeScreenshot(shot));
    QTRY_COMPARE_WITH_TIMEOUT(QImage(shot).size(), frame.size(), TestTimeout::mediumMs());

    // Another output's going away leaves the registered one alone.
    const std::unique_ptr<QQuickItem> otherOutput = createVideoOutput(engine);
    controller.unregisterVideoOutput(otherOutput.get());
    QCOMPARE(controller.videoOutput(), output.get());

    // An output that leaves as another one registers hands the camera over, which never stops for it.
    controller.unregisterVideoOutput(output.get());
    QCOMPARE(controller.videoOutput(), output.get());
    controller.registerVideoOutput(otherOutput.get());
    QCoreApplication::sendPostedEvents(&controller, QEvent::MetaCall);
    QCOMPARE(controller.videoOutput(), otherOutput.get());

    // With no other output by the next pass, the camera has none.
    controller.unregisterVideoOutput(otherOutput.get());
    QCoreApplication::sendPostedEvents(&controller, QEvent::MetaCall);
    expectLogMessage("Video.VideoReceiver", QtWarningMsg, noOutput);
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
    QCOMPARE(controller.videoOutput(), output.get());
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
    QTRY_VERIFY_WITH_TIMEOUT(showsTurnedMirroredQuadrant(QImage(shot)), TestTimeout::mediumMs());
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
    QTRY_VERIFY_WITH_TIMEOUT(showsTurnedMirroredQuadrant(QImage(photo)), TestTimeout::mediumMs());

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
    QSignalSpy errorSpy(&controller, &VideoReceiver::errorOccurred);
    QVERIFY(controller.takeScreenshot(photo));
    QVERIFY(controller._photoFiles.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(QImage(photo).size(), QSize(64, 48), TestTimeout::mediumMs());

    // A failed capture falls back the same way.
    const QString failed = tempDir.filePath(QStringLiteral("failed.png"));
    controller._photoFiles.insert(9, {failed, {}});
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Photo capture failed")));
    controller._onImageCaptureError(9, QStringLiteral("Camera busy"));
    verifyExpectedLogMessage();
    QTRY_COMPARE_WITH_TIMEOUT(QImage(failed).size(), QSize(64, 48), TestTimeout::mediumMs());
    QCOMPARE(errorSpy.count(), 0);

    // A captured photo that can't be saved is reported. A debug build of Qt, which a Windows Debug build links, warns
    // of the failed write as well.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("^QImage::save: failed to write")));
    expectLogMessage("Video.VideoScreenshot", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to save screenshot")));
    controller._photoFiles.insert(10, {tempDir.filePath(QStringLiteral("missing/unwritable.png")), {}});
    controller._onImageAvailable(10, QVideoFrame(redTopLeftQuadrant()));
    QTRY_COMPARE_WITH_TIMEOUT(errorSpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(errorSpy.takeFirst().at(0).toString().contains(QStringLiteral("unwritable.png")));
    verifyExpectedLogMessage();

    // So is a failed capture once there is no output to take the photo from instead.
    controller.unregisterVideoOutput(output.get());
    QCoreApplication::sendPostedEvents(&controller, QEvent::MetaCall);
    controller._photoFiles.insert(11, {tempDir.filePath(QStringLiteral("none.png")), {}});
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Photo capture failed")));
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No video on display to take the photo from")));
    controller._onImageCaptureError(11, QStringLiteral("Camera busy"));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.takeFirst().at(0).toString().contains(QStringLiteral("no video is on display")));
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
    selectCamera(controller, QString::fromLatin1(kMissingCamera));
    verifyExpectedLogMessage();
    QCOMPARE(controller.device(), QString::fromLatin1(kMissingCamera));
    QVERIFY(!controller._camera);
    QVERIFY(!controller._captureSession);
    QCOMPARE(controller.state(), VideoReceiver::State::Stopped);
    QVERIFY(!controller.stats().valid);

    selectCamera(controller, QString());
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
    selectCamera(controller, QString::fromLatin1(kMissingCamera));
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
    selectCamera(controller, QString::fromLatin1(kMissingCamera));
    const QString error = QStringLiteral("Camera is in use");

    // The first error of a run is reported, and the camera is recreated later, not from its own signal.
    expectAppMessage(
        QRegularExpression(QStringLiteral("USB camera QGC Test Missing Camera: Camera is in use. Retrying.")));
    controller._onCameraError(error);
    verifyExpectedLogMessage();
    QVERIFY(controller._retryTimer->isActive());
    QCOMPARE(controller._retryTimer->intervalAsDuration(),
             std::chrono::milliseconds(UVCCameraController::kRetryInterval));
    QCOMPARE(controller.state(), VideoReceiver::State::Reconnecting);

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
    QCOMPARE(controller.state(), VideoReceiver::State::Stopped);

    // A restart tries the camera again, in a new run.
    controller.restart();
    QCOMPARE(controller._retries, 0);
    expectAppMessage(QRegularExpression(QStringLiteral("Camera is in use. Retrying.")));
    controller._onCameraError(error);
    verifyExpectedLogMessage();
    QCOMPARE(controller._retries, 1);

    // So does selecting a camera.
    selectCamera(controller, QString());
    QCOMPARE(controller._retries, 0);
    QVERIFY(!controller._retryTimer->isActive());
}

void UVCCameraControllerTest::_testRecordingNeedsRunningCamera()
{
    UVCCameraController controller;
    QSignalSpy finishedSpy(&controller, &UVCCameraController::recordingFinished);
    QSignalSpy recordingSpy(&controller, &UVCCameraController::recordingChanged);
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    selectCamera(controller, QString::fromLatin1(kMissingCamera));

    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("Can't record")));
    controller.startRecording(QStringLiteral("/tmp/uvc.mp4"), VideoReceiver::FileFormat::Mp4);
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().at(0).toString(), QStringLiteral("/tmp/uvc.mp4"));
    QVERIFY(!finishedSpy.first().at(1).toString().isEmpty());
    QCOMPARE(recordingSpy.count(), 0);
    QVERIFY(!controller.recording());
    QVERIFY(!controller._recorder->active());
}

UT_REGISTER_TEST(UVCCameraControllerTest, TestLabel::Unit)
