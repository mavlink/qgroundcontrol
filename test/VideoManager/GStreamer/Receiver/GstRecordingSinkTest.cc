#include "GstRecordingSinkTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "RecordingBranch.h"
#include "VideoReceiver.h"

namespace {

struct DemuxPadContext
{
    GstElement* parser = nullptr;
    std::atomic<bool> linked{false};
};

void linkH265DemuxPad([[maybe_unused]] GstElement* demux, GstPad* pad, gpointer userData)
{
    auto* context = static_cast<DemuxPadContext*>(userData);
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    if (!caps || gst_caps_is_any(caps) || gst_caps_is_empty(caps)) {
        gst_clear_caps(&caps);
        return;
    }

    bool isH265 = false;
    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        if (gst_structure_has_name(gst_caps_get_structure(caps, i), "video/x-h265")) {
            isH265 = true;
            break;
        }
    }
    gst_clear_caps(&caps);
    if (!isH265) {
        return;
    }

    GstPad* sinkPad = gst_element_get_static_pad(context->parser, "sink");
    if (sinkPad) {
        context->linked.store(gst_pad_link(pad, sinkPad) == GST_PAD_LINK_OK, std::memory_order_relaxed);
        gst_clear_object(&sinkPad);
    }
}

GstFlowReturn pushAccessUnit(GstElement* appsrc, const QByteArray& bytes, GstClockTime timestamp,
                             GstClockTime duration = GST_SECOND / 4)
{
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(bytes.size()), nullptr);
    if (!buffer) {
        return GST_FLOW_ERROR;
    }
    (void) gst_buffer_fill(buffer, 0, bytes.constData(), static_cast<gsize>(bytes.size()));
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = duration;
    return gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

}  // namespace

void GstRecordingSinkTest::_testRecordingSinkFinalizesMidStreamH265Mp4()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "mp4mux", "qtdemux", "avdec_h265");

    // Synthetic 32x32 Annex-B H.265 access units. Only the first AU contains VPS/SPS/PPS; the
    // later IDRs intentionally contain just a slice. This reproduces a camera that sends codec
    // configuration once at session startup, before the user begins recording.
    const QByteArray initialAu = QByteArray::fromBase64(QByteArrayLiteral(
        "AAAAAUABDAH//wQIAAADAJgoAAADAAAeugJAAAAAAUIBAQQIAAADAJgoAAADAAAekAhBCKUt0lJhf/gACAALUGBgYEAAAAMAQ"
        "AAAAwECAAAAAUQBwHAwYBEgAAAAASgBrC2AE6/7X1g="));
    const QByteArray secondAu = QByteArray::fromBase64(QByteArrayLiteral("AAAAASgBrC2AE6/7X1g="));
    const QByteArray thirdAu = QByteArray::fromBase64(QByteArrayLiteral("AAAAASgBrCyAE6/7X1g="));
    QVERIFY(!initialAu.isEmpty());
    QVERIFY(!secondAu.isEmpty());
    QVERIFY(!thirdAu.isEmpty());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString outputPath = QDir(tempDir.path()).filePath(QStringLiteral("mid-stream.mp4"));

    GstTest::TestPipeline pipeline(gst_pipeline_new("mid-stream-recording-test"));
    QVERIFY(pipeline.get());

    GstElement* source = gst_element_factory_make("appsrc", nullptr);
    GstElement* upstreamParser = gst_element_factory_make("h265parse", nullptr);
    GstElement* tee = gst_element_factory_make("tee", nullptr);
    GstElement* displayQueue = gst_element_factory_make("queue", nullptr);
    GstElement* displaySink = gst_element_factory_make("fakesink", nullptr);
    GstElement* recorderQueue = gst_element_factory_make("queue", nullptr);
    GstElement* recorderValve = gst_element_factory_make("valve", nullptr);
    QVERIFY(source);
    QVERIFY(upstreamParser);
    QVERIFY(tee);
    QVERIFY(displayQueue);
    QVERIFY(displaySink);
    QVERIFY(recorderQueue);
    QVERIFY(recorderValve);

    GstCaps* sourceCaps =
        gst_caps_from_string("video/x-h265,stream-format=byte-stream,alignment=au,width=32,height=32,framerate=4/1");
    QVERIFY(sourceCaps);
    g_object_set(source, "caps", sourceCaps, "format", GST_FORMAT_TIME, nullptr);
    gst_clear_caps(&sourceCaps);
    g_object_set(upstreamParser, "config-interval", -1, nullptr);
    g_object_set(displaySink, "sync", FALSE, "async", FALSE, nullptr);
    g_object_set(recorderValve, "drop", TRUE, nullptr);
    gst_util_set_object_arg(G_OBJECT(recorderValve), "drop-mode", "forward-sticky-events");

    gst_bin_add_many(pipeline.bin(), source, upstreamParser, tee, displayQueue, displaySink, recorderQueue,
                     recorderValve, nullptr);
    QVERIFY(gst_element_link_many(source, upstreamParser, tee, nullptr));
    QVERIFY(gst_element_link_many(tee, displayQueue, displaySink, nullptr));
    QVERIFY(gst_element_link_many(tee, recorderQueue, recorderValve, nullptr));

    std::atomic<int> displayedBuffers{0};
    const GStreamer::GstPadPtr displayPad = GStreamer::adoptPad(gst_element_get_static_pad(displaySink, "sink"));
    QVERIFY(displayPad);
    GStreamer::PadProbe displayProbe;
    QVERIFY(displayProbe.add(displayPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &displayedBuffers));

    QVERIFY(pipeline.play());
    QVERIFY(pushAccessUnit(source, initialAu, 0) == GST_FLOW_OK);
    QTRY_VERIFY_WITH_TIMEOUT(displayedBuffers.load(std::memory_order_relaxed) >= 1, TestTimeout::shortMs());

    const GStreamer::GstPadPtr valveSrcPad = GStreamer::adoptPad(gst_element_get_static_pad(recorderValve, "src"));
    QVERIFY(valveSrcPad);
    const GStreamer::GstCapsPtr recordingCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(valveSrcPad.get()));
    QVERIFY2(recordingCaps, "Closed recording valve did not preserve negotiated caps");

    const GStreamer::GstElementPtr fileSink =
        RecordingBranch::makeFileSink(outputPath, VideoReceiver::FileFormat::Mp4, recordingCaps.get());
    QVERIFY(fileSink);
    QVERIFY(gst_bin_add(pipeline.bin(), fileSink.get()));
    QVERIFY(gst_element_link(recorderValve, fileSink.get()));
    QVERIFY(gst_element_sync_state_with_parent(fileSink.get()));
    g_object_set(recorderValve, "drop", FALSE, nullptr);

    QVERIFY(pushAccessUnit(source, secondAu, GST_SECOND / 4) == GST_FLOW_OK);
    QVERIFY(pushAccessUnit(source, thirdAu, GST_SECOND / 2) == GST_FLOW_OK);
    QVERIFY(gst_app_src_end_of_stream(GST_APP_SRC(source)) == GST_FLOW_OK);

    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL);
    QVERIFY2(QFileInfo(outputPath).size() > 0, "Recording did not produce an MP4 file");

    GstTest::TestPipeline verifyPipeline(gst_pipeline_new("verify-mid-stream-recording"));
    QVERIFY(verifyPipeline.get());

    GstElement* fileSource = gst_element_factory_make("filesrc", nullptr);
    GstElement* demux = gst_element_factory_make("qtdemux", nullptr);
    GstElement* verifyParser = gst_element_factory_make("h265parse", nullptr);
    GstElement* verifyDecoder = gst_element_factory_make("avdec_h265", nullptr);
    GstElement* verifySink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(fileSource);
    QVERIFY(demux);
    QVERIFY(verifyParser);
    QVERIFY(verifyDecoder);
    QVERIFY(verifySink);

    const QByteArray outputPathBytes = QFile::encodeName(outputPath);
    g_object_set(fileSource, "location", outputPathBytes.constData(), nullptr);
    g_object_set(verifySink, "sync", FALSE, "async", FALSE, nullptr);
    gst_bin_add_many(verifyPipeline.bin(), fileSource, demux, verifyParser, verifyDecoder, verifySink, nullptr);
    QVERIFY(gst_element_link(fileSource, demux));
    QVERIFY(gst_element_link_many(verifyParser, verifyDecoder, verifySink, nullptr));

    DemuxPadContext demuxContext{verifyParser};
    const gulong padAddedHandler = g_signal_connect(demux, "pad-added", G_CALLBACK(linkH265DemuxPad), &demuxContext);
    QVERIFY(padAddedHandler != 0);
    const auto padAddedCleanup = qScopeGuard([&] { g_signal_handler_disconnect(demux, padAddedHandler); });

    std::atomic<int> verifiedBuffers{0};
    const GStreamer::GstPadPtr verifyPad = GStreamer::adoptPad(gst_element_get_static_pad(verifySink, "sink"));
    QVERIFY(verifyPad);
    GStreamer::PadProbe verifyProbe;
    QVERIFY(verifyProbe.add(verifyPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &verifiedBuffers));

    QVERIFY(verifyPipeline.play());
    const QString verifyFailure = verifyPipeline.runToEos();
    QVERIFY2(verifyFailure.isEmpty(), qPrintable(verifyFailure));
    QVERIFY2(demuxContext.linked.load(std::memory_order_relaxed), "MP4 has no H.265 video track");
    QVERIFY2(verifiedBuffers.load(std::memory_order_relaxed) >= 2,
             "Finalized MP4 did not decode the two mid-stream H.265 access units");
}

void GstRecordingSinkTest::_testRecordingSinkTimestampsDtsOnlyFrames()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "appsink", "mp4mux", "qtdemux");

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString outputPath = tempDir.filePath(QStringLiteral("dts-only.mp4"));
    constexpr int kFrames = 20;
    QString loadError;
    const QList<GstTest::AccessUnit> units =
        GstTest::loadAccessUnits(QString::fromLatin1(GstTest::kH264Stream), &loadError);
    QVERIFY2(units.size() >= kFrames, qPrintable(loadError));

    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("appsrc name=src format=time "
                                            "caps=video/x-h264,stream-format=byte-stream,alignment=au ! "
                                            "h264parse ! identity name=strip")),
             qPrintable(pipeline.error()));

    // After packet loss the parsers pass some frames on with only a DTS; qtmux alone would fail the recording.
    struct Stripped
    {
        int deltaFrames = 0;  ///< Touched only on the streaming thread
        std::atomic<int> count{0};
    } stripped;

    const GStreamer::GstPadPtr stripPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("strip").get(), "src"));
    QVERIFY(stripPad);
    GStreamer::PadProbe stripProbe;
    QVERIFY(stripProbe.add(
        stripPad.get(), GST_PAD_PROBE_TYPE_BUFFER,
        [](GstPad*, GstPadProbeInfo* info, gpointer data) -> GstPadProbeReturn {
            GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
            auto* const strippedCount = static_cast<Stripped*>(data);
            if (buffer && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT) &&
                GST_BUFFER_DTS_IS_VALID(buffer) && (strippedCount->deltaFrames++ % 3 == 0)) {
                buffer = gst_buffer_make_writable(buffer);
                GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
                GST_PAD_PROBE_INFO_DATA(info) = buffer;
                strippedCount->count.fetch_add(1, std::memory_order_relaxed);
            }
            return GST_PAD_PROBE_OK;
        },
        &stripped));

    const GStreamer::GstCapsPtr recordingCaps =
        GStreamer::adoptCaps(gst_caps_from_string("video/x-h264,stream-format=byte-stream,alignment=au"));
    const GStreamer::GstElementPtr fileSink =
        RecordingBranch::makeFileSink(outputPath, VideoReceiver::FileFormat::Mp4, recordingCaps.get());
    QVERIFY(fileSink);
    QVERIFY(gst_bin_add(pipeline.bin(), fileSink.get()));
    QVERIFY(gst_element_link(pipeline.element("strip").get(), fileSink.get()));

    QVERIFY(pipeline.play());
    const GStreamer::GstElementPtr source = pipeline.element("src");
    constexpr GstClockTime kFrameDuration = GST_SECOND / GstTest::kStreamFrameRate;
    for (int frame = 0; frame < kFrames; ++frame) {
        QCOMPARE(pushAccessUnit(source.get(), units.at(frame).bytes, frame * kFrameDuration, kFrameDuration),
                 GST_FLOW_OK);
    }
    QCOMPARE(gst_app_src_end_of_stream(GST_APP_SRC(source.get())), GST_FLOW_OK);
    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL);
    QVERIFY2(stripped.count.load(std::memory_order_relaxed) > 0, "No frame lost its PTS; the test proves nothing");

    QString playbackFailure;
    const int frames = GstTest::countPlayableFrames(outputPath, playbackFailure);
    QVERIFY2(frames >= 0, qPrintable(playbackFailure));
    QCOMPARE(frames, kFrames);
}

#endif

UT_REGISTER_TEST(GstRecordingSinkTest, TestLabel::Integration)
