#include "VideoManagerInitTest.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QRegularExpression>

#include "GStreamerTestHelpers.h"
#include "VideoManager.h"
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

void VideoManagerInitTest::_testBackendInitFailure()
{
    VideoManager videoManager;
    int receiverCount = 0;
    videoManager._receiverFactory = countingFactory(receiverCount);
    videoManager._initState = VideoManager::InitState::Pending;
    videoManager._onMainWindowReady();

    expectLogMessage("Video.VideoManager", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("video initialization failed")));
    videoManager._onBackendInitComplete(false);
    verifyExpectedLogMessage();

    QCOMPARE(videoManager._initState, VideoManager::InitState::Failed);
    QVERIFY(!videoManager.waitForVideoBackendReady());
    QCOMPARE(receiverCount, 0);
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

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
