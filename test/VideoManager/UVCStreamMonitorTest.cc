#include "UVCStreamMonitorTest.h"

#include <memory>

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

#include "UVCStreamMonitor.h"
#include "VideoTestHelpers.h"

using VideoTestHelpers::showsText;

namespace {

constexpr const char* kStatsGroupQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

UVCStatsGroup {
}
)QML";

/// Hands @p sink @p count frames of a 640 × 480 MJPEG camera set up for 30 fps.
void showCameraFrames(QVideoSink& sink, int count)
{
    QVideoFrameFormat format(QSize(640, 480), QVideoFrameFormat::Format_Jpeg);
    format.setStreamFrameRate(30.0);
    for (int i = 0; i < count; ++i) {
        sink.setVideoFrame(QVideoFrame(format));
    }
}

}  // namespace

void UVCStreamMonitorTest::_measureSecond(UVCStreamMonitor& monitor, QVideoSink& sink, qint64& nowMs)
{
    showCameraFrames(sink, 1);
    monitor._sample();
    nowMs += 1000;
    showCameraFrames(sink, 30);
    monitor._sample();
}

void UVCStreamMonitorTest::_testFramesBecomeStats()
{
    qint64 nowMs = 1000;
    UVCStreamMonitor monitor(nullptr, [&nowMs] { return nowMs; });
    QVideoSink sink;
    QSignalSpy statsSpy(&monitor, &UVCStreamMonitor::statsChanged);
    monitor.attach(nullptr, &sink);

    // Nothing to measure before the first frame.
    nowMs += 1000;
    monitor._sample();
    QVERIFY(!monitor.stats().valid);

    // The first sample with frames only sets the baseline. The invalid frame clearing the display isn't one.
    showCameraFrames(sink, 1);
    sink.setVideoFrame(QVideoFrame());
    monitor._sample();
    QVERIFY(!monitor.stats().valid);
    QCOMPARE(statsSpy.count(), 0);

    nowMs += 1000;
    showCameraFrames(sink, 30);
    monitor._sample();
    QCOMPARE(statsSpy.count(), 1);
    const VideoStreamStats stats = monitor.stats();
    QVERIFY(stats.valid);
    QCOMPARE(stats.fps, 30.0);
    QCOMPARE(stats.codec, QStringLiteral("MJPEG"));
    QCOMPARE(stats.gpuPath, QStringLiteral("CPU"));
    QCOMPARE(stats.uptimeMs, qint64(1000));
    QCOMPARE(stats.bitrateKbps, 0.0);
    QVERIFY(!stats.hasRtpStats());
    QCOMPARE(monitor.videoSize(), QSize(640, 480));
    QCOMPARE(monitor.cameraFrameRate(), 30.0);
    QVERIFY(monitor.cameraName().isEmpty());

    // A camera that stalls reads 0 fps once its frames drop out of the rate window.
    for (int i = 0; i < VideoStreamStatsMeter::kRateWindowIntervals; ++i) {
        nowMs += 1000;
        monitor._sample();
    }
    QVERIFY(monitor.stats().valid);
    QCOMPARE(monitor.stats().fps, 0.0);
    QCOMPARE(monitor.stats().uptimeMs, qint64(4000));
}

void UVCStreamMonitorTest::_testDetachClearsStats()
{
    qint64 nowMs = 1000;
    UVCStreamMonitor monitor(nullptr, [&nowMs] { return nowMs; });
    QVideoSink sink;
    QVideoSink otherSink;
    monitor.attach(nullptr, &sink);
    _measureSecond(monitor, sink, nowMs);
    QVERIFY(monitor.stats().valid);

    QSignalSpy statsSpy(&monitor, &UVCStreamMonitor::statsChanged);
    monitor.detach();
    QCOMPARE(statsSpy.count(), 1);
    QVERIFY(!monitor.stats().valid);
    QCOMPARE(monitor.videoSize(), QSize());
    QCOMPARE(monitor.cameraFrameRate(), 0.0);

    // Frames after the detach count for nothing.
    showCameraFrames(sink, 30);
    nowMs += 1000;
    monitor._sample();
    QVERIFY(!monitor.stats().valid);

    // Attaching starts a new stream, whose uptime counts from its own first frame.
    monitor.attach(nullptr, &otherSink);
    _measureSecond(monitor, otherSink, nowMs);
    QVERIFY(monitor.stats().valid);
    QCOMPARE(monitor.stats().uptimeMs, qint64(1000));
}

void UVCStreamMonitorTest::_testDestroyedSinkClearsStats()
{
    qint64 nowMs = 1000;
    UVCStreamMonitor monitor(nullptr, [&nowMs] { return nowMs; });
    auto sink = std::make_unique<QVideoSink>();
    monitor.attach(nullptr, sink.get());
    _measureSecond(monitor, *sink, nowMs);
    QVERIFY(monitor.stats().valid);

    sink.reset();
    nowMs += 1000;
    monitor._sample();
    QVERIFY(!monitor.stats().valid);
}

void UVCStreamMonitorTest::_testStatsGroupShowsCamera()
{
    qint64 nowMs = 1000;
    UVCStreamMonitor monitor(nullptr, [&nowMs] { return nowMs; });
    QVideoSink sink;
    monitor.attach(nullptr, &sink);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(kStatsGroupQml, QUrl(QStringLiteral("qrc:/UVCStreamMonitorTest/StatsGroup.qml")));
    const std::unique_ptr<QQuickItem> group(qobject_cast<QQuickItem*>(
        component.createWithInitialProperties({{QStringLiteral("monitor"), QVariant::fromValue(&monitor)}})));
    QVERIFY2(group, qPrintable(component.errorString()));
    // Items outside a scene are never visible.
    QQuickWindow window;
    group->setParentItem(window.contentItem());
    QVERIFY(!group->isVisible());

    _measureSecond(monitor, sink, nowMs);
    QVERIFY(group->isVisible());
    QVERIFY(showsText(group.get(), QStringLiteral("30.0 fps (camera 30 fps)")));
    QVERIFY(showsText(group.get(), QStringLiteral("640 × 480 MJPEG")));
    QVERIFY(showsText(group.get(), QStringLiteral("CPU")));
    QVERIFY(showsText(group.get(), QStringLiteral("0:01")));
    // Without a camera there is no name to show.
    QVERIFY(!showsText(group.get(), QStringLiteral("Camera")));
}

UT_REGISTER_TEST(UVCStreamMonitorTest, TestLabel::Unit)
