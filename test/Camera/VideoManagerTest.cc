#include "VideoManagerTest.h"

#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "QtMultimediaReceiver.h"
#include "VideoManager.h"
#include "VideoReceiver.h"

namespace {

/// Records the screenshot paths VideoManager asks it to save.
class ScreenshotRecordingReceiver : public VideoReceiver
{
public:
    explicit ScreenshotRecordingReceiver(const QString& name) { setName(name); }

    void start(uint32_t /*timeout*/) override {}

    void stop() override {}

    void startDecoding(VideoSinkHandle /*sink*/) override {}

    void stopDecoding() override {}

    void startRecording(const QString& /*videoFile*/, FILE_FORMAT /*format*/) override {}

    void stopRecording() override {}

    void takeScreenshot(const QString& imageFile) override { requested.append(imageFile); }

    QStringList requested;
};

}  // namespace

void VideoManagerTest::_videoOutputQmlTypeAvailableInUnitTestMode_test()
{
    static constexpr auto envName = "QGC_TEST_ENABLE_GSTREAMER";
    TestFixtures::EnvVarFixture envBackup(envName);
    qunsetenv(envName);

    VideoManager testManager;
    QQmlEngine engine;
    QQmlComponent component(&engine);

    component.setData(R"QML(
import QtQuick
import QtMultimedia

VideoOutput {
    width: 32
    height: 24
}
)QML", QUrl(QStringLiteral("qrc:/VideoManagerTest.qml")));

    QObject *item = component.create();
    QVERIFY2(component.errors().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(item != nullptr);
    QVERIFY(qobject_cast<QQuickItem *>(item) != nullptr);

    delete item;
}

void VideoManagerTest::_qtMultimediaScreenshotSavesDisplayedFrame_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("screenshot.png"));

    QQuickVideoOutput videoOutput;
    const std::unique_ptr<VideoReceiver> receiver(QtMultimediaReceiver::createVideoReceiver(nullptr));
    receiver->setWidget(&videoOutput);
    receiver->startDecoding(videoOutput.videoSink());

    // An unmapped CPU frame, as a decoder delivers it.
    QImage displayed(QSize(64, 48), QImage::Format_RGBA8888);
    displayed.fill(Qt::red);
    videoOutput.videoSink()->setVideoFrame(QVideoFrame(displayed));

    QSignalSpy screenshotSpy(receiver.get(), &VideoReceiver::onTakeScreenshotComplete);
    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    receiver->takeScreenshot(path);
    QCOMPARE(screenshotSpy.count(), 1);
    QCOMPARE(screenshotSpy.constFirst().constFirst().value<VideoReceiver::STATUS>(), VideoReceiver::STATUS_OK);
    QCOMPARE(QImage(path).size(), displayed.size());
}

void VideoManagerTest::_screenshotsNameEachStartedStream_test()
{
    VideoManager videoManager;
    ScreenshotRecordingReceiver visible(QStringLiteral("videoContent"));
    ScreenshotRecordingReceiver thermal(QStringLiteral("thermalVideo"));
    videoManager._videoReceivers = {&visible, &thermal};
    const auto detach = qScopeGuard([&videoManager] { videoManager._videoReceivers.clear(); });

    visible.setStarted(true);
    thermal.setStarted(true);
    videoManager.grabImage(QStringLiteral("/photos/2026-10-16_01.02.03.004.jpg"));
    QCOMPARE(visible.requested, QStringList{QStringLiteral("/photos/2026-10-16_01.02.03.004.jpg")});
    QCOMPARE(thermal.requested, QStringList{QStringLiteral("/photos/2026-10-16_01.02.03.004.thermalVideo.jpg")});

    thermal.setStarted(false);
    videoManager.grabImage(QStringLiteral("/photos/no-extension"));
    QCOMPARE(visible.requested.last(), QStringLiteral("/photos/no-extension"));
    QCOMPARE(thermal.requested.size(), 1);
}

UT_REGISTER_TEST(VideoManagerTest, TestLabel::Unit)
