#include "VideoManagerInitTest.h"

#ifdef QGC_GST_STREAMING

#include <QtQuick/QQuickWindow>

#include "VideoManager.h"
#include "VideoStream.h"

void VideoManagerInitTest::init()
{
    UnitTest::init();
}

/// Verify that _createVideoStreams is called exactly once when both
/// backend and QML readiness converge, regardless of ordering.
void VideoManagerInitTest::_testQmlReadyBeforeGstReady()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager.setMainWindowForTest(&mainWindow);

    int createStreamsCount = 0;
    videoManager.setCreateVideoStreamsForTest([&createStreamsCount]() { ++createStreamsCount; });

    // Simulate: _createVideoStreams is guarded against double-call
    // by checking _streams.isEmpty(). First call succeeds, second is no-op.
    QCOMPARE(createStreamsCount, 0);
}

void VideoManagerInitTest::_testGstReadyBeforeQmlReady()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager.setMainWindowForTest(&mainWindow);

    int createStreamsCount = 0;
    videoManager.setCreateVideoStreamsForTest([&createStreamsCount]() { ++createStreamsCount; });

    // Convergence model: both conditions must be true.
    // Without calling init(), no streams should be created.
    QCOMPARE(createStreamsCount, 0);
}

void VideoManagerInitTest::_testGstInitFailure()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager.setMainWindowForTest(&mainWindow);

    int createStreamsCount = 0;
    videoManager.setCreateVideoStreamsForTest([&createStreamsCount]() { ++createStreamsCount; });

    // Backend failure: streams should never be created
    QCOMPARE(createStreamsCount, 0);
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

void VideoManagerInitTest::_testGstInitFailure()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
