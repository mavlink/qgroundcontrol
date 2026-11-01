#include "UVCCameraControllerTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QMediaFormat>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "UVCCameraController.h"
#include "UVCStreamMonitor.h"
#include "VideoTestHelpers.h"

using VideoTestHelpers::createVideoOutput;
using VideoTestHelpers::showFrame;

namespace {

constexpr const char* kMissingCamera = "QGC Test Missing Camera";

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

void UVCCameraControllerTest::_testRecordingFormat_data()
{
    QTest::addColumn<QMediaFormat::FileFormat>("container");
    QTest::newRow("mkv") << QMediaFormat::Matroska;
    QTest::newRow("mov") << QMediaFormat::QuickTime;
    QTest::newRow("mp4") << QMediaFormat::MPEG4;
}

void UVCCameraControllerTest::_testRecordingFormat()
{
    QFETCH(QMediaFormat::FileFormat, container);

    // Whatever this platform's Qt encodes, best first: H.264 where Qt counts it encodable, in the container asked for
    // where Qt can write it.
    const QList<QMediaFormat> formats = UVCCameraController::recordingFormats(container);
    QVERIFY(!formats.isEmpty());
    const QMediaFormat::FileFormat used = formats.first().fileFormat();
    for (const QMediaFormat& format : formats) {
        QVERIFY(format.isSupported(QMediaFormat::Encode));
        QCOMPARE(format.fileFormat(), used);
        QCOMPARE(formats.count(format), 1);
    }
    QMediaFormat h264(used);
    h264.setVideoCodec(QMediaFormat::VideoCodec::H264);
    if (h264.isSupported(QMediaFormat::Encode)) {
        QCOMPARE(formats.first().videoCodec(), QMediaFormat::VideoCodec::H264);
    }
    QMediaFormat asked(container);
    asked.setVideoCodec(formats.first().videoCodec());
    if (asked.isSupported(QMediaFormat::Encode)) {
        QCOMPARE(used, container);
    }
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
    QVERIFY(!controller._recorder);
}

UT_REGISTER_TEST(UVCCameraControllerTest, TestLabel::Unit)
