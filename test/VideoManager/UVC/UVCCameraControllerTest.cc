#include "UVCCameraControllerTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoFrameInput>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "QtVideoRecorder.h"
#include "UVCCameraController.h"
#include "VideoStreamStatsMeter.h"
#include "VideoTestHelpers.h"

namespace {

using VideoTestHelpers::createVideoOutput;
using VideoTestHelpers::redTopLeftQuadrant;
using VideoTestHelpers::showFrame;

constexpr const char* kMissingCamera = "QGC Test Missing Camera";

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
    controller.setVideoOutput(output.get());

    QImage frame(QSize(64, 48), QImage::Format_RGBA8888);
    frame.fill(Qt::red);
    showFrame(output.get(), frame);
    // Without a camera nothing streams, whatever the output shows: VideoManager takes no screenshot of it.
    QVERIFY(!controller.streaming());
    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(controller.takeScreenshot(shot));
    QTRY_COMPARE_WITH_TIMEOUT(QImage(shot).size(), frame.size(), TestTimeout::mediumMs());

    controller.setVideoOutput(nullptr);
    expectLogMessage("Video.VideoReceiver", QtWarningMsg, noOutput);
    QVERIFY(!controller.takeScreenshot(shot));
    verifyExpectedLogMessage();
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
    controller.setVideoOutput(output.get());

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
    controller.setVideoOutput(output.get());
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
    controller.setVideoOutput(nullptr);
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
    controller.setVideoOutput(output.get());

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

void UVCCameraControllerTest::_testStartWhileRecordingIsRefused()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));

    UVCCameraController controller;
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    selectCamera(controller, QString::fromLatin1(kMissingCamera));
    // The camera's recorder waits for its first frame, as while a camera is recorded. No camera is attached here: a
    // frame input stands in for its capture session.
    ignoreLogMessage("qt.multimedia.ffmpeg.encoder", QtInfoMsg,
                     QRegularExpression(QStringLiteral("^Recording new media")));
    QMediaCaptureSession session;
    QVideoFrameInput input;
    session.setVideoFrameInput(&input);
    session.setRecorder(controller._recorder->mediaRecorder());
    controller._recorder->start(tempDir.filePath(QStringLiteral("first.mp4")), QMediaFormat::MPEG4);
    QVERIFY(controller._recorder->active());

    // Whoever asks for another recording meanwhile is told that its file isn't recorded: the first one goes on.
    QSignalSpy finishedSpy(&controller, &UVCCameraController::recordingFinished);
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("recording already")));
    controller.startRecording(second, VideoReceiver::FileFormat::Mp4);
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().at(0).toString(), second);
    QVERIFY(!finishedSpy.first().at(1).toString().isEmpty());
    QVERIFY(controller._recorder->active());
    QVERIFY(controller.recording());
    QVERIFY(!QFile::exists(second));

    controller.stopRecording();
    QTRY_VERIFY_WITH_TIMEOUT(!controller._recorder->active(), TestTimeout::mediumMs());
    QVERIFY(!controller.recording());
}

void UVCCameraControllerTest::_countCameraFrames(UVCCameraController& controller, int count)
{
    QVideoFrameFormat format(QSize(640, 480), QVideoFrameFormat::Format_Jpeg);
    format.setStreamFrameRate(30.0);
    for (int i = 0; i < count; ++i) {
        controller._frameCounters.count(QVideoFrame(format));
    }
}

void UVCCameraControllerTest::_measureSecond(UVCCameraController& controller, qint64& nowMs)
{
    _countCameraFrames(controller, 1);
    controller._sample();
    nowMs += 1000;
    _countCameraFrames(controller, 30);
    controller._sample();
}

void UVCCameraControllerTest::_testFramesBecomeStats()
{
    qint64 nowMs = 1000;
    UVCCameraController controller(nullptr, [&nowMs] { return nowMs; });
    QSignalSpy statsSpy(&controller, &VideoReceiver::statsChanged);

    // Nothing to measure before the first frame.
    nowMs += 1000;
    controller._sample();
    QVERIFY(!controller.stats().valid);

    // The first sample with frames only sets the baseline.
    _countCameraFrames(controller, 1);
    controller._sample();
    QVERIFY(!controller.stats().valid);
    QCOMPARE(statsSpy.count(), 0);

    controller._retries = 2;
    nowMs += 1000;
    _countCameraFrames(controller, 30);
    controller._sample();
    QCOMPARE(statsSpy.count(), 1);
    // Frames flowing again end a run of retries.
    QCOMPARE(controller._retries, 0);
    const VideoStreamStats stats = controller.stats();
    QVERIFY(stats.valid);
    QCOMPARE(stats.fps, 30.0);
    QCOMPARE(stats.codec, QStringLiteral("MJPEG"));
    QCOMPARE(stats.gpuPath, QStringLiteral("CPU"));
    QCOMPARE(stats.uptimeMs, qint64(1000));
    // The camera hands over frames, not the bytes it sent.
    QCOMPARE(stats.bitrateKbps, -1.0);
    QVERIFY(!stats.hasRtpStats());
    // Nor does anything count the frames that never reach the output.
    QCOMPARE(stats.framesDropped, qint64(-1));
    // The frame rate the camera is set up for, as its frames tell.
    QCOMPARE(stats.cameraFps, 30.0);

    // A camera that stalls reads 0 fps once its frames drop out of the rate window.
    for (int i = 0; i < VideoStreamStatsMeter::kRateWindowIntervals; ++i) {
        nowMs += 1000;
        controller._sample();
    }
    QVERIFY(controller.stats().valid);
    QCOMPARE(controller.stats().fps, 0.0);
    QCOMPARE(controller.stats().uptimeMs, qint64(4000));
}

void UVCCameraControllerTest::_testStatsRestartWithoutOutput()
{
    qint64 nowMs = 1000;
    UVCCameraController controller(nullptr, [&nowMs] { return nowMs; });
    QQmlEngine engine;
    std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    QVERIFY(output);
    controller.setVideoOutput(output.get());
    _measureSecond(controller, nowMs);
    QVERIFY(controller.stats().valid);

    // The output set again, as one that moved to another window is, is the one connected: the stream goes on.
    QSignalSpy statsSpy(&controller, &VideoReceiver::statsChanged);
    controller.setVideoOutput(output.get());
    QCOMPARE(statsSpy.count(), 0);
    QVERIFY(controller.stats().valid);

    // The output went away, and the stream with it.
    output.reset();
    QCOMPARE(statsSpy.count(), 1);
    QVERIFY(!controller.stats().valid);
    nowMs += 1000;
    controller._sample();
    QVERIFY(!controller.stats().valid);

    // The next output starts a new stream, whose uptime counts from its own first frame.
    output = createVideoOutput(engine);
    QVERIFY(output);
    controller.setVideoOutput(output.get());
    _measureSecond(controller, nowMs);
    QVERIFY(controller.stats().valid);
    QCOMPARE(controller.stats().uptimeMs, qint64(1000));
}

UT_REGISTER_TEST(UVCCameraControllerTest, TestLabel::Unit)
