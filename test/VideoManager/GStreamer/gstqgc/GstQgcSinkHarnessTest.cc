#include "GstQgcSinkHarnessTest.h"

#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QtCore/QCoreApplication>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "GStreamer.h"
#include "GstScoped.h"

namespace {

/// A GstHarness feeding a qgcvideosinkbin bound to a QVideoSink. The bin's sink doesn't sync, so frames go through on
/// push(); the QVideoSink gets them only when deliver() runs the calls queued to this thread.
class SinkBinHarness
{
public:
    explicit SinkBinHarness(QVideoSink* videoSink)
    {
        GstElement* const bin = gst_element_factory_make("qgcvideosinkbin", nullptr);
        _bin = GStreamer::adoptElement(bin ? GST_ELEMENT(gst_object_ref_sink(bin)) : nullptr);
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

    GstElement* bin() const { return _bin.get(); }

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

    static void deliver(QVideoSink& videoSink) { QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall); }

private:
    QObject _owner;
    GStreamer::GstElementPtr _bin;
    GstHarness* _harness = nullptr;
    GstVideoInfo _info{};
};

}  // namespace

void GstQgcSinkHarnessTest::_testBinDeliversLatestFrameOnly()
{
    int emissions = 0;
    qint64 lastStartUs = -1;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame& frame) {
        ++emissions;
        lastStartUs = frame.startTime();
    });
    SinkBinHarness harness(&videoSink);
    QVERIFY(harness.isValid());
    harness.setCaps("I420", 64, 48);

    // Frames the converter hands on while the GUI thread is busy: only the newest reaches the QVideoSink.
    for (int i = 1; i <= 3; ++i) {
        QCOMPARE(harness.push(i * GST_SECOND), GST_FLOW_OK);
    }
    SinkBinHarness::deliver(videoSink);
    QCOMPARE(emissions, 1);
    QCOMPARE(lastStartUs, qint64(3 * GST_SECOND / GST_USECOND));
}

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
    harness.setCaps("NV12", 32, 16);
    QCOMPARE(harness.push(2 * GST_SECOND), GST_FLOW_OK);
    SinkBinHarness::deliver(videoSink);
    QCOMPARE(videoSink.videoFrame().size(), QSize(32, 16));
    QCOMPARE(videoSink.videoFrame().pixelFormat(), QVideoFrameFormat::Format_NV12);
}

void GstQgcSinkHarnessTest::_testBinAppliesOrientationTag_data()
{
    QTest::addColumn<QString>("tag");
    QTest::addColumn<int>("rotation");
    QTest::addColumn<bool>("mirrored");

    QTest::newRow("rotate-90") << QStringLiteral("rotate-90") << int(QtVideo::Rotation::Clockwise90) << false;
    QTest::newRow("rotate-180") << QStringLiteral("rotate-180") << int(QtVideo::Rotation::Clockwise180) << false;
    // Flipped, then turned 90 degrees clockwise: Qt turns first, so that's 270 degrees, then mirrored.
    QTest::newRow("flip-rotate-90") << QStringLiteral("flip-rotate-90") << int(QtVideo::Rotation::Clockwise270) << true;
}

void GstQgcSinkHarnessTest::_testBinAppliesOrientationTag()
{
    QFETCH(QString, tag);
    QFETCH(int, rotation);
    QFETCH(bool, mirrored);

    QVideoSink videoSink;
    SinkBinHarness harness(&videoSink);
    QVERIFY(harness.isValid());
    harness.setCaps("I420", 64, 48);

    // The tag has to cross the bin's converter to reach the sink.
    QVERIFY(
        harness.pushEvent(gst_event_new_tag(gst_tag_list_new(GST_TAG_IMAGE_ORIENTATION, qPrintable(tag), nullptr))));
    QCOMPARE(harness.push(GST_SECOND), GST_FLOW_OK);
    SinkBinHarness::deliver(videoSink);
    const QVideoFrame frame = videoSink.videoFrame();
    QVERIFY(frame.isValid());
    QCOMPARE(int(frame.rotation()), rotation);
    QCOMPARE(frame.mirrored(), mirrored);
    QCOMPARE(frame.size(), QSize(64, 48));
}

void GstQgcSinkHarnessTest::_testBinDropsUndeliveredFrame_data()
{
    QTest::addColumn<bool>("viaFlush");
    QTest::newRow("flush-start") << true;
    QTest::newRow("stop") << false;
}

void GstQgcSinkHarnessTest::_testBinDropsUndeliveredFrame()
{
    QFETCH(bool, viaFlush);

    int emissions = 0;
    QVideoSink videoSink;
    QObject::connect(&videoSink, &QVideoSink::videoFrameChanged, &videoSink, [&](const QVideoFrame&) { ++emissions; });
    SinkBinHarness harness(&videoSink);
    QVERIFY(harness.isValid());
    harness.setCaps("I420", 64, 48);

    QCOMPARE(harness.push(GST_SECOND), GST_FLOW_OK);
    if (viaFlush) {
        QVERIFY(harness.pushEvent(gst_event_new_flush_start()));
    } else {
        QCOMPARE(gst_element_set_state(harness.bin(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
    SinkBinHarness::deliver(videoSink);
    QCOMPARE(emissions, 0);
    QVERIFY(!videoSink.videoFrame().isValid());
}

UT_REGISTER_TEST(GstQgcSinkHarnessTest, TestLabel::Unit)
