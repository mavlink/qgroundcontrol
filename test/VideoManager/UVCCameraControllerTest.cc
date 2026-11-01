#include "UVCCameraControllerTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
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

    // Without a selected camera, a change of cameras is only announced.
    controller._onVideoInputsChanged();
    QCOMPARE(camerasSpy.count(), 1);

    // A selected camera that is no longer attached was unplugged.
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    controller.setDevice(QString::fromLatin1(kMissingCamera));
    const QRegularExpression message(QStringLiteral("USB camera QGC Test Missing Camera was disconnected"));
    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("Camera unplugged")));
    expectAppMessage(message);
    controller._onVideoInputsChanged();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(camerasSpy.count(), 2);
}

void UVCCameraControllerTest::_testCameraErrorReported()
{
    UVCCameraController controller;
    ignoreLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("No camera named")));
    controller.setDevice(QString::fromLatin1(kMissingCamera));

    expectLogMessage("Video.UVCCameraController", QtWarningMsg, QRegularExpression(QStringLiteral("Camera error")));
    expectAppMessage(QRegularExpression(QStringLiteral("USB camera QGC Test Missing Camera: Camera is in use")));
    controller._onCameraError(QStringLiteral("Camera is in use"));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(UVCCameraControllerTest, TestLabel::Unit)
