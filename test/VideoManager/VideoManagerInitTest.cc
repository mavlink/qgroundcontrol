#include "VideoManagerInitTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QRegularExpression>
#include <QtQuick/QQuickWindow>

#include "VideoManager.h"
#include "VideoSettings.h"

void VideoManagerInitTest::init()
{
    UnitTest::init();

    static const QRegularExpression sGStreamerCriticalRe(
        QStringLiteral("cannot register existing type|"
                       "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                       "g_once_init_leave.*result != 0|"
                       "GStreamer initialization failed"));
    // These backend startup diagnostics are platform/runtime dependent and not
    // tied to a single deterministic call site in this fixture.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtCriticalMsg, sGStreamerCriticalRe);
}

void VideoManagerInitTest::_testQmlReadyBeforeGstReady()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager._mainWindow = &mainWindow;

    int createReceiversCount = 0;
    videoManager._createVideoReceiversForTest = [&createReceiversCount]() {
        ++createReceiversCount;
    };

    videoManager._initState = VideoManager::InitState::Pending;

    videoManager._initAfterQmlIsReady();
    QCOMPARE(videoManager._initState, VideoManager::InitState::QmlReady);
    QCOMPARE(createReceiversCount, 0);

    videoManager._onGstInitComplete(true);
    QCOMPARE(videoManager._initState, VideoManager::InitState::Running);
    QCOMPARE(createReceiversCount, 1);

    expectLogMessage("Video.VideoManager", QtWarningMsg, QRegularExpression(QStringLiteral("_onGstInitComplete: unexpected state")));
    videoManager._onGstInitComplete(true);
    verifyExpectedLogMessage();
    QCOMPARE(createReceiversCount, 1);
}

void VideoManagerInitTest::_testGstReadyBeforeQmlReady()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager._mainWindow = &mainWindow;

    int createReceiversCount = 0;
    videoManager._createVideoReceiversForTest = [&createReceiversCount]() {
        ++createReceiversCount;
    };

    videoManager._initState = VideoManager::InitState::Pending;

    videoManager._onGstInitComplete(true);
    QCOMPARE(videoManager._initState, VideoManager::InitState::GstReady);
    QCOMPARE(createReceiversCount, 0);

    videoManager._initAfterQmlIsReady();
    QCOMPARE(videoManager._initState, VideoManager::InitState::Running);
    QCOMPARE(createReceiversCount, 1);

    expectLogMessage("Video.VideoManager", QtWarningMsg, QRegularExpression(QStringLiteral("_initAfterQmlIsReady: unexpected state")));
    videoManager._initAfterQmlIsReady();
    verifyExpectedLogMessage();
    QCOMPARE(createReceiversCount, 1);
}

void VideoManagerInitTest::_testGstInitFailure()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager._mainWindow = &mainWindow;

    int createReceiversCount = 0;
    videoManager._createVideoReceiversForTest = [&createReceiversCount]() {
        ++createReceiversCount;
    };

    videoManager._initState = VideoManager::InitState::Pending;

    expectLogMessage("Video.VideoManager", QtCriticalMsg, QRegularExpression(QStringLiteral("GStreamer initialization failed")));
    videoManager._onGstInitComplete(false);
    verifyExpectedLogMessage();
    QCOMPARE(videoManager._initState, VideoManager::InitState::Failed);
    QCOMPARE(createReceiversCount, 0);

    expectLogMessage("Video.VideoManager", QtWarningMsg, QRegularExpression(QStringLiteral("QML ready but GStreamer init failed")));
    videoManager._initAfterQmlIsReady();
    verifyExpectedLogMessage();
    QCOMPARE(videoManager._initState, VideoManager::InitState::Failed);
    QCOMPARE(createReceiversCount, 0);
}

void VideoManagerInitTest::_testNetworkJpegRecordingFormatPolicy()
{
    for (const QString source : {
             QString::fromLatin1(VideoSettings::videoSourceHTTPMJPEG),
             QString::fromLatin1(VideoSettings::videoSourceWebSocketJPEG),
         }) {
        QVERIFY(VideoManager::_recordingFormatSupportedForSource(source, VideoReceiver::FILE_FORMAT_MKV));
        QVERIFY(VideoManager::_recordingFormatSupportedForSource(source, VideoReceiver::FILE_FORMAT_MOV));
        QVERIFY(!VideoManager::_recordingFormatSupportedForSource(source, VideoReceiver::FILE_FORMAT_MP4));
    }

    QVERIFY(VideoManager::_recordingFormatSupportedForSource(QString::fromLatin1(VideoSettings::videoSourceRTSP),
                                                             VideoReceiver::FILE_FORMAT_MP4));
}

#else

void VideoManagerInitTest::init() { UnitTest::init(); QSKIP("GStreamer not enabled"); }
void VideoManagerInitTest::_testQmlReadyBeforeGstReady() { QSKIP("GStreamer not enabled"); }
void VideoManagerInitTest::_testGstReadyBeforeQmlReady() { QSKIP("GStreamer not enabled"); }
void VideoManagerInitTest::_testGstInitFailure() { QSKIP("GStreamer not enabled"); }
void VideoManagerInitTest::_testNetworkJpegRecordingFormatPolicy()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
