#include "VideoManagerInitTest.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>

#include "GStreamerTestHelpers.h"
#include "QtMultimediaVideoReceiver.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

namespace {

/// Counts the receivers VideoManager creates.
auto countingFactory(int& count)
{
    return [&count](QObject* parent) {
        ++count;
        return new FakeVideoReceiver(parent);
    };
}

}  // namespace

void VideoManagerInitTest::init()
{
    UnitTest::init();

#ifdef QGC_GST_STREAMING
    // Completing the init prunes the decoders from the GStreamer registry, which these tests don't initialize.
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
    videoManager._initState = VideoManager::InitState::Pending;

    if (windowFirst) {
        videoManager._onMainWindowReady();
        QCOMPARE(receiverCount, 0);
        videoManager._onBackendInitComplete(true);
    } else {
        videoManager._onBackendInitComplete(true);
        QCOMPARE(receiverCount, 0);
        videoManager._onMainWindowReady();
    }
    QCOMPARE(videoManager._initState, VideoManager::InitState::Ready);
    QCOMPARE(receiverCount, 2);
    QVERIFY(videoManager.primaryStream()->receiver());
    QVERIFY(videoManager.thermalStream()->receiver());

    // A late second completion (the queued continuation after waitForVideoBackendReady()) changes nothing.
    videoManager._onBackendInitComplete(true);
    QCOMPARE(receiverCount, 2);
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
    videoManager._initState = VideoManager::InitState::Pending;
    if (windowFirst) {
        videoManager._onMainWindowReady();
    }

    expectLogMessage("Video.VideoManager", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("video initialization failed")));
    expectAppMessage(QRegularExpression(QStringLiteral("failed to initialize")));
    videoManager._onBackendInitComplete(false);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(videoManager._initState, VideoManager::InitState::Failed);
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
    videoManager._initState = VideoManager::InitState::Pending;
    videoManager._onMainWindowReady();

    // The application exits while the backend still initializes: neither its completion nor a setting that replaces
    // the receivers creates one afterwards.
    videoManager.cleanup();
    videoManager._onBackendInitComplete(true);
    QCOMPARE(receiverCount, 0);
    restorer.set(backend, VideoSettings::VideoBackendQtMultimedia);
    QCOMPARE(receiverCount, 0);
    QVERIFY(!videoManager.primaryStream()->receiver());
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
    videoManager._initState = VideoManager::InitState::Pending;
    videoManager._onMainWindowReady();
    videoManager._watchBackendInit(QtConcurrent::run([] { return true; }));

    QVERIFY(videoManager.waitForVideoBackendReady());
    QCOMPARE(videoManager._initState, VideoManager::InitState::Ready);
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
    videoManager._initState = VideoManager::InitState::Pending;
    videoManager._onMainWindowReady();
    videoManager._onBackendInitComplete(true);
    QCOMPARE(receiverCount, 2);
    const QPointer<VideoReceiver> primary = videoManager.primaryStream()->receiver();
    const QPointer<VideoReceiver> thermal = videoManager.thermalStream()->receiver();

    // Another backend: each stream gets a new receiver.
    restorer.set(backend, VideoSettings::VideoBackendQtMultimedia);
    QCOMPARE(receiverCount, 4);
    QVERIFY(primary.isNull());
    QVERIFY(thermal.isNull());
    QVERIFY(videoManager.primaryStream()->receiver());
    QVERIFY(videoManager.thermalStream()->receiver());
}

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
