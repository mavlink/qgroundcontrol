#include "VideoManagerInitTest.h"

#ifdef QGC_GST_STREAMING

#include "VideoManager.h"

#include <QtQuick/QQuickWindow>

void VideoManagerInitTest::init()
{
    UnitTest::init();
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

    // Repeated completion notifications must not create duplicate receivers.
    videoManager._onGstInitComplete(true);
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

    // Repeated QML-ready notifications must not create duplicate receivers.
    videoManager._initAfterQmlIsReady();
    QCOMPARE(createReceiversCount, 1);
}

#else

void VideoManagerInitTest::init()
{
    UnitTest::init();
    QSKIP("GStreamer not enabled");
}

void VideoManagerInitTest::_testQmlReadyBeforeGstReady()
{
    QSKIP("GStreamer not enabled");
}

void VideoManagerInitTest::_testGstReadyBeforeQmlReady()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
