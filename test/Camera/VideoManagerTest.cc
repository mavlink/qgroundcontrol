#include "VideoManagerTest.h"

#include <QtCore/QFile>
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
#include "VideoManager.h"
#include "VideoReceiver.h"

namespace {

/// A receiver whose output shows whatever frame the test puts into it.
class FrameReceiver : public VideoReceiver
{
public:
    explicit FrameReceiver(const QString& name)
    {
        setName(name);
        setVideoOutput(&output);
    }

    /// Shows a @p size frame, as a decoder would deliver it.
    void showFrame(QSize size)
    {
        QImage displayed(size, QImage::Format_RGBA8888);
        displayed.fill(Qt::red);
        output.videoSink()->setVideoFrame(QVideoFrame(displayed));
    }

    QQuickVideoOutput output;

protected:
    void onConfigChanged(const Config& /*previous*/) override {}

    void onActiveChanged() override {}

    void onRestartRequested() override {}

    void onVideoOutputChanged() override {}

    void onStartRecording(const QString& /*file*/, FileFormat /*format*/) override {}

    void onStopRecording() override {}
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

void VideoManagerTest::_screenshotSavesDisplayedFrame_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("screenshot.png"));

    FrameReceiver receiver(QStringLiteral("videoContent"));
    receiver.showFrame(QSize(64, 48));

    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(receiver.takeScreenshot(path));
    QCOMPARE(QImage(path).size(), QSize(64, 48));
}

void VideoManagerTest::_screenshotsNameEachStartedStream_test()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    VideoManager videoManager;
    FrameReceiver visible(QStringLiteral("videoContent"));
    FrameReceiver thermal(QStringLiteral("thermalVideo"));
    visible.showFrame(QSize(64, 48));
    thermal.showFrame(QSize(32, 24));
    videoManager._videoReceivers = {&visible, &thermal};
    const auto detach = qScopeGuard([&videoManager] { videoManager._videoReceivers.clear(); });

    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    visible.setActive(true);
    thermal.setActive(true);
    videoManager.grabImage(tempDir.filePath(QStringLiteral("2026-10-16_01.02.03.004.jpg")));
    QCOMPARE(QImage(tempDir.filePath(QStringLiteral("2026-10-16_01.02.03.004.jpg"))).size(), QSize(64, 48));
    QCOMPARE(QImage(tempDir.filePath(QStringLiteral("2026-10-16_01.02.03.004.thermalVideo.jpg"))).size(),
             QSize(32, 24));

    thermal.setActive(false);
    videoManager.grabImage(tempDir.filePath(QStringLiteral("second.png")));
    QVERIFY(QFile::exists(tempDir.filePath(QStringLiteral("second.png"))));
    QVERIFY(!QFile::exists(tempDir.filePath(QStringLiteral("second.thermalVideo.png"))));
}

UT_REGISTER_TEST(VideoManagerTest, TestLabel::Unit)
