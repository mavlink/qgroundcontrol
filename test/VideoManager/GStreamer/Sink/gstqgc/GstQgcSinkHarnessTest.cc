#include "GstQgcSinkHarnessTest.h"

#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QtCore/QCoreApplication>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GstScoped.h"
#include "gstqgc/GstQgcSinkBin.h"

namespace {

/// A GstHarness feeding a sink bin bound to a QVideoSink. The bin's sink doesn't sync, so frames go through on
/// push(); the QVideoSink gets them only when deliver() runs the calls queued to this thread.
class SinkBinHarness
{
public:
    explicit SinkBinHarness(QVideoSink* videoSink)
    {
        _bin = GStreamer::adoptFloating(GstQgc::makeSinkBin({}));
        if (_bin && GStreamer::setupQVideoSinkElement(_bin.get(), videoSink, &_owner)) {
            _harness = gst_harness_new_with_element(_bin.get(), "sink", nullptr);
        }
    }

    ~SinkBinHarness()
    {
        if (_harness) {
            gst_harness_teardown(_harness);
        }
    }

    SinkBinHarness(const SinkBinHarness&) = delete;
    SinkBinHarness& operator=(const SinkBinHarness&) = delete;

    bool isValid() const { return _harness != nullptr; }

    /// Sends stream caps for raw @p format frames of @p width x @p height, then a segment.
    void setCaps(const char* format, int width, int height)
    {
        (void) gst_video_info_set_format(&_info, gst_video_format_from_string(format), static_cast<guint>(width),
                                         static_cast<guint>(height));
        gst_harness_set_src_caps(_harness, gst_video_info_to_caps(&_info));
    }

    /// Pushes a mid-grey frame in the current caps, stamped @p pts.
    GstFlowReturn push(GstClockTime pts)
    {
        GstBuffer* const buffer = gst_harness_create_buffer(_harness, GST_VIDEO_INFO_SIZE(&_info));
        gst_buffer_memset(buffer, 0, 0x80, GST_VIDEO_INFO_SIZE(&_info));
        GST_BUFFER_PTS(buffer) = pts;
        return gst_harness_push(_harness, buffer);
    }

    bool pushEvent(GstEvent* event) { return gst_harness_push_event(_harness, event); }

    /// Starts another stream in the current caps.
    bool restartStream()
    {
        GstSegment segment;
        gst_segment_init(&segment, GST_FORMAT_TIME);
        return pushEvent(gst_event_new_stream_start("qgc-harness-restart")) &&
               pushEvent(gst_event_new_segment(&segment));
    }

    static void deliver(QVideoSink& videoSink) { QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall); }

private:
    QObject _owner;
    GStreamer::GstElementPtr _bin;
    GstHarness* _harness = nullptr;
    GstVideoInfo _info{};
};

}  // namespace

void GstQgcSinkHarnessTest::_testBinRenegotiatesMidStream()
{
    QVideoSink videoSink;
    SinkBinHarness harness(&videoSink);
    QVERIFY(harness.isValid());

    harness.setCaps("I420", 64, 48);
    QCOMPARE(harness.push(GST_SECOND), GST_FLOW_OK);
    SinkBinHarness::deliver(videoSink);
    QCOMPARE(videoSink.videoFrame().size(), QSize(64, 48));
    QCOMPARE(videoSink.videoFrame().pixelFormat(), QVideoFrameFormat::Format_YUV420P);

    // A camera switching resolution and format mid-stream: the converter must renegotiate, not scale to the old size.
    // Same byte size as the first format, so a frame the sink mapped with stale caps would still map and show the old
    // layout.
    harness.setCaps("NV12", 48, 64);
    QCOMPARE(harness.push(2 * GST_SECOND), GST_FLOW_OK);
    SinkBinHarness::deliver(videoSink);
    QCOMPARE(videoSink.videoFrame().size(), QSize(48, 64));
    QCOMPARE(videoSink.videoFrame().pixelFormat(), QVideoFrameFormat::Format_NV12);
}

void GstQgcSinkHarnessTest::_testBinAppliesOrientationTag_data()
{
    QTest::addColumn<QString>("tag");
    QTest::addColumn<bool>("restartStream");
    QTest::addColumn<int>("rotation");
    QTest::addColumn<bool>("mirrored");

    QTest::newRow("rotate-90") << QStringLiteral("rotate-90") << false << int(QtVideo::Rotation::Clockwise90) << false;
    QTest::newRow("rotate-180") << QStringLiteral("rotate-180") << false << int(QtVideo::Rotation::Clockwise180)
                                << false;
    // Flipped, then turned 90 degrees clockwise: Qt turns first, so that's 270 degrees, then mirrored.
    QTest::newRow("flip-rotate-90") << QStringLiteral("flip-rotate-90") << false << int(QtVideo::Rotation::Clockwise270)
                                    << true;
    // The tag belongs to the stream it came with.
    QTest::newRow("new-stream-untagged") << QStringLiteral("rotate-90") << true << int(QtVideo::Rotation::None)
                                         << false;
}

void GstQgcSinkHarnessTest::_testBinAppliesOrientationTag()
{
    QFETCH(QString, tag);
    QFETCH(bool, restartStream);
    QFETCH(int, rotation);
    QFETCH(bool, mirrored);

    QVideoSink videoSink;
    SinkBinHarness harness(&videoSink);
    QVERIFY(harness.isValid());
    harness.setCaps("I420", 64, 48);

    // The tag has to cross the bin's converter to reach the sink.
    QVERIFY(
        harness.pushEvent(gst_event_new_tag(gst_tag_list_new(GST_TAG_IMAGE_ORIENTATION, qPrintable(tag), nullptr))));
    if (restartStream) {
        QVERIFY(harness.restartStream());
    }
    QCOMPARE(harness.push(GST_SECOND), GST_FLOW_OK);
    SinkBinHarness::deliver(videoSink);
    const QVideoFrame frame = videoSink.videoFrame();
    QVERIFY(frame.isValid());
    QCOMPARE(int(frame.rotation()), rotation);
    QCOMPARE(frame.mirrored(), mirrored);
    QCOMPARE(frame.size(), QSize(64, 48));
}

UT_REGISTER_TEST(GstQgcSinkHarnessTest, TestLabel::Unit)
