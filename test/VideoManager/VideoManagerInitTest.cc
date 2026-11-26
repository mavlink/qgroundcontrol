#include "VideoManagerInitTest.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QPointer>
#include <QtCore/QPromise>
#include <QtCore/QRegularExpression>

#include "QtMultimediaVideoReceiver.h"
#include "SettingsManager.h"
#include "UVCCameraController.h"
#include "VideoBackendInit.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

#ifdef QGC_GST_STREAMING
#include "GStreamerTestHelpers.h"
#endif

namespace {

/// Counts the receivers VideoManager creates.
auto countingFactory(int& count)
{
    return [&count](QObject* parent) {
        ++count;
        return new FakeVideoReceiver(parent);
    };
}

/// A backend initialization the test finishes itself: the manager's stays pending until then.
class PendingBackendInit
{
public:
    explicit PendingBackendInit(VideoBackendInit* backendInit)
    {
        _initialization.start();
        backendInit->start(_initialization.future());
    }

    void finish(bool ready)
    {
        _initialization.addResult(ready);
        _initialization.finish();
        // The manager is told from the event loop.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    }

private:
    QPromise<bool> _initialization;
};

}  // namespace

void VideoManagerInitTest::init()
{
    UnitTest::init();

#ifdef QGC_GST_STREAMING
    // Completing the init looks the decoders up in the GStreamer registry, which these tests don't initialize.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtCriticalMsg, GstTest::glibTypeRegistrationCriticals());
#endif
}

void VideoManagerInitTest::_testReceiversNeedBackendAndWindow_data()
{
    QTest::addColumn<bool>("windowFirst");

    QTest::newRow("window first") << true;
    QTest::newRow("backend first") << false;
}

void VideoManagerInitTest::_testReceiversNeedBackendAndWindow()
{
    QFETCH(bool, windowFirst);

    VideoManager videoManager;
    int receiverCount = 0;
    videoManager._receiverFactory = countingFactory(receiverCount);
    PendingBackendInit backendInit(videoManager._backendInit);

    if (windowFirst) {
        videoManager._onMainWindowReady();
        QCOMPARE(receiverCount, 0);
        backendInit.finish(true);
    } else {
        backendInit.finish(true);
        QCOMPARE(receiverCount, 0);
        videoManager._onMainWindowReady();
    }
    QCOMPARE(videoManager._backendInit->state(), VideoBackendInit::State::Ready);
    QCOMPARE(receiverCount, 2);
    QVERIFY(videoManager.primaryStream()->receiver());
    QVERIFY(videoManager.thermalStream()->receiver());
}

void VideoManagerInitTest::_testBackendInitFailure_data()
{
    QTest::addColumn<bool>("windowFirst");

    QTest::newRow("window first") << true;
    QTest::newRow("backend first") << false;
}

void VideoManagerInitTest::_testBackendInitFailure()
{
    QFETCH(bool, windowFirst);

    Fact* const backend = SettingsManager::instance()->videoSettings()->videoBackend();
    VideoTestHelpers::FactRestorer restorer;
    restorer.set(backend, VideoSettings::VideoBackendGStreamer);

    VideoManager videoManager;
    int receiverCount = 0;
    videoManager._receiverFactory = countingFactory(receiverCount);
    PendingBackendInit backendInit(videoManager._backendInit);
    if (windowFirst) {
        videoManager._onMainWindowReady();
    }

    expectLogMessage("Video.VideoManager", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("video initialization failed")));
    expectAppMessage(QRegularExpression(QStringLiteral("failed to initialize")));
    backendInit.finish(false);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(videoManager._backendInit->state(), VideoBackendInit::State::Failed);
    QVERIFY(!videoManager.waitForVideoBackendReady());

    // The streams still get receivers, which the backend makes without its initialization.
    if (!windowFirst) {
        QCOMPARE(receiverCount, 0);
        videoManager._onMainWindowReady();
    }
    QCOMPARE(receiverCount, 2);
    QVERIFY(videoManager.primaryStream()->receiver());
    QVERIFY(videoManager.thermalStream()->receiver());

    // So does a change of the backend setting.
    restorer.set(backend, VideoSettings::VideoBackendQtMultimedia);
    QCOMPARE(receiverCount, 4);
}

void VideoManagerInitTest::_testNothingStartsAfterCleanup()
{
    Fact* const backend = SettingsManager::instance()->videoSettings()->videoBackend();
    VideoTestHelpers::FactRestorer restorer;
    restorer.set(backend, VideoSettings::VideoBackendGStreamer);

    VideoManager videoManager;
    int receiverCount = 0;
    videoManager._receiverFactory = countingFactory(receiverCount);
    PendingBackendInit backendInit(videoManager._backendInit);
    videoManager._onMainWindowReady();

    // The application exits while the backend still initializes: neither its completion nor a setting that replaces
    // the receivers creates one afterwards.
    videoManager.cleanup();
    backendInit.finish(true);
    QCOMPARE(receiverCount, 0);
    restorer.set(backend, VideoSettings::VideoBackendQtMultimedia);
    QCOMPARE(receiverCount, 0);
    QVERIFY(!videoManager.primaryStream()->receiver());
    // The USB camera, which QML refers to, is stopped rather than deleted.
    QCOMPARE(videoManager._cameraSource->receiver(), videoManager.uvcCamera());
}

void VideoManagerInitTest::_testWaitForBackendReady()
{
    // Unit tests run without the video backend's global init.
    {
        VideoManager videoManager;
        QVERIFY(videoManager.waitForVideoBackendReady());
    }

    VideoManager videoManager;
    int receiverCount = 0;
    videoManager._receiverFactory = countingFactory(receiverCount);
    videoManager._backendInit->start(QtConcurrent::run([] { return true; }));
    videoManager._onMainWindowReady();

    QVERIFY(videoManager.waitForVideoBackendReady());
    QCOMPARE(videoManager._backendInit->state(), VideoBackendInit::State::Ready);
    QCOMPARE(receiverCount, 2);

    // The continuation queued for the GUI thread finds the init complete.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(receiverCount, 2);
}

void VideoManagerInitTest::_testBackendSettingReplacesReceivers()
{
    Fact* const backend = SettingsManager::instance()->videoSettings()->videoBackend();
    VideoTestHelpers::FactRestorer restorer;
    restorer.set(backend, VideoSettings::VideoBackendGStreamer);

    VideoManager videoManager;
    // The default factory makes Qt Multimedia's receiver once it is chosen.
    restorer.set(backend, VideoSettings::VideoBackendQtMultimedia);
    QObject owner;
    QVERIFY(qobject_cast<QtMultimediaVideoReceiver*>(videoManager._receiverFactory(&owner)));
    restorer.set(backend, VideoSettings::VideoBackendGStreamer);

    int receiverCount = 0;
    videoManager._receiverFactory = countingFactory(receiverCount);
    PendingBackendInit backendInit(videoManager._backendInit);
    videoManager._onMainWindowReady();
    backendInit.finish(true);
    QCOMPARE(receiverCount, 2);
    const QPointer<VideoReceiver> primary = videoManager.primaryStream()->receiver();
    const QPointer<VideoReceiver> thermal = videoManager.thermalStream()->receiver();

    // A stream shown in a tile, once the others have their receivers, gets one as well.
    videoManager.setTileRoom(1);
    const auto current = VideoTestHelpers::makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5601");
    const auto other = VideoTestHelpers::makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    videoManager._setAdvertisedStreams({current.get(), nullptr, QString()}, nullptr,
                                       {{other.get(), nullptr, QString()}});
    QCOMPARE(videoManager._sources.size(), 4);
    QCOMPARE(receiverCount, 3);
    const VideoStreamController* const tileStream = videoManager._sources.last();
    const QPointer<VideoReceiver> tile = tileStream->receiver();
    QVERIFY(tile);

    // Another backend: each stream gets a new receiver.
    restorer.set(backend, VideoSettings::VideoBackendQtMultimedia);
    QCOMPARE(receiverCount, 6);
    QVERIFY(primary.isNull());
    QVERIFY(thermal.isNull());
    QVERIFY(tile.isNull());
    QVERIFY(videoManager.primaryStream()->receiver());
    QVERIFY(videoManager.thermalStream()->receiver());
    QVERIFY(tileStream->receiver());
    // The USB camera's receiver is no backend's: it stays.
    QCOMPARE(videoManager._cameraSource->receiver(), videoManager.uvcCamera());
}

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
