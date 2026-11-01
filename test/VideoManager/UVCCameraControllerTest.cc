#include "UVCCameraControllerTest.h"

#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QVideoFrame>
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

/// A VideoOutput turned 90° clockwise and mirrored, as UVCVideoOutput sets it from the rotation and mirror settings.
std::unique_ptr<QQuickItem> turnedMirroredOutput(QQmlEngine& engine)
{
    std::unique_ptr<QQuickItem> output = createVideoOutput(engine);
    if (output && !(output->setProperty("orientation", 90) && output->setProperty("mirrored", true))) {
        output.reset();
    }
    return output;
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

void UVCCameraControllerTest::_testScreenshotFollowsOutputTransform()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString shot = tempDir.filePath(QStringLiteral("uvc.png"));

    UVCCameraController controller;
    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = turnedMirroredOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());

    showFrame(output.get(), redTopLeftQuadrant());
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(controller.takeScreenshot(shot));
    QVERIFY(showsTurnedMirroredQuadrant(QImage(shot)));
}

void UVCCameraControllerTest::_testPhotoFollowsOutputTransform()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString photo = tempDir.filePath(QStringLiteral("photo.png"));

    UVCCameraController controller;
    QQmlEngine engine;
    const std::unique_ptr<QQuickItem> output = turnedMirroredOutput(engine);
    QVERIFY(output);
    controller.registerVideoOutput(output.get());

    // A captured frame, as QImageCapture hands it over for request 7, is saved as displayed.
    controller._photoFiles.insert(7, photo);
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

    // No camera captures, so the frame on display is saved, right away.
    controller.capturePhoto(photo);
    QCOMPARE(QImage(photo).size(), QSize(64, 48));
    QVERIFY(controller._photoFiles.isEmpty());

    // A failed capture falls back the same way.
    const QString failed = tempDir.filePath(QStringLiteral("failed.png"));
    controller._photoFiles.insert(9, failed);
    expectLogMessage("Video.UVCCameraController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Photo capture failed")));
    controller._onImageCaptureError(9, QStringLiteral("Camera busy"));
    verifyExpectedLogMessage();
    QCOMPARE(QImage(failed).size(), QSize(64, 48));
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

void UVCCameraControllerTest::_testRecordingFormatTriesCodecFirst()
{
    const QStringList codecs = UVCCameraController::recordingCodecs();
    QVERIFY(!codecs.isEmpty());
    const QList<QMediaFormat> fallbacks = UVCCameraController::recordingFormats(QMediaFormat::Matroska);
    for (const QString& key : codecs) {
        const QMediaFormat::VideoCodec codec = UVCCameraController::recordingCodec(key);
        const QList<QMediaFormat> formats = UVCCameraController::recordingFormats(QMediaFormat::Matroska, codec);
        QVERIFY2(!formats.isEmpty(), qPrintable(key));
        QCOMPARE(formats.first().videoCodec(), codec);
        QVERIFY(formats.first().isSupported(QMediaFormat::Encode));
        // The fallbacks follow, in their order, without the codec's own format again.
        QList<QMediaFormat> rest = fallbacks;
        rest.removeAll(formats.first());
        QCOMPARE(formats.mid(1), rest);
    }
    QCOMPARE(UVCCameraController::recordingFormats(QMediaFormat::Matroska, QMediaFormat::VideoCodec::Unspecified),
             fallbacks);
}

void UVCCameraControllerTest::_testRecordingCodecs()
{
    // Each container's fallbacks appear in the list in the order a recording tries them. The list covers every
    // container, so it can hold codecs a given container can't.
    const QStringList codecs = UVCCameraController::recordingCodecs();
    for (const QMediaFormat::FileFormat container :
         {QMediaFormat::MPEG4, QMediaFormat::Matroska, QMediaFormat::QuickTime}) {
        qsizetype previous = -1;
        for (const QMediaFormat& format : UVCCameraController::recordingFormats(container)) {
            const qsizetype index = codecs.indexOf(QString::fromLatin1(
                QMetaEnum::fromType<QMediaFormat::VideoCodec>().valueToKey(static_cast<int>(format.videoCodec()))));
            QVERIFY(index > previous);
            previous = index;
        }
    }
    for (const QString& key : codecs) {
        QCOMPARE(codecs.count(key), 1);
        QVERIFY(!UVCCameraController::recordingCodecName(key).isEmpty());
    }
    QCOMPARE(UVCCameraController::recordingCodec(QStringLiteral("H264")), QMediaFormat::VideoCodec::H264);
    QCOMPARE(UVCCameraController::recordingCodec(QString()), QMediaFormat::VideoCodec::Unspecified);
    QCOMPARE(UVCCameraController::recordingCodec(QStringLiteral("NoSuchCodec")), QMediaFormat::VideoCodec::Unspecified);
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
