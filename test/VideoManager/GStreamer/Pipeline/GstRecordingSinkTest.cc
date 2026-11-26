#include "GstRecordingSinkTest.h"

#include <atomic>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QMediaPlayer>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "RecordingSink.h"
#include "VideoReceiver.h"

namespace {

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

/// The H.264 test stream's first frames through h264parse into the recording sink.
struct SinkRecording
{
    /// Builds appsrc ! h264parse name=parse ! the file sink for @p path. Why not, or an empty string.
    QString build(const QString& path, VideoReceiver::FileFormat format)
    {
        if (!pipeline.launch(QStringLiteral("appsrc name=src format=time "
                                            "caps=video/x-h264,stream-format=byte-stream,alignment=au ! "
                                            "h264parse name=parse"))) {
            return pipeline.error();
        }
        const GStreamer::GstCapsPtr recordingCaps =
            GStreamer::adoptCaps(gst_caps_from_string("video/x-h264,stream-format=byte-stream,alignment=au"));
        sinkBin = GStreamer::RecordingSink::create(path, format, recordingCaps.get());
        if (!sinkBin || !gst_bin_add(pipeline.bin(), sinkBin.get()) ||
            !gst_element_link(pipeline.element("parse").get(), sinkBin.get())) {
            return QStringLiteral("%1: no file sink linked to the parser").arg(path);
        }
        return {};
    }

    /// Plays the first @p frames frames into the file, to EOS, and stops the pipeline. Why not, or an empty string.
    QString record(int frames)
    {
        QString failure;
        const QList<GstTest::AccessUnit> units =
            GstTest::loadAccessUnits(QString::fromLatin1(GstTest::kH264Stream), &failure);
        if (units.size() < frames) {
            return QStringLiteral("%1 of %2 frames loaded %3").arg(units.size()).arg(frames).arg(failure);
        }
        if (!pipeline.play()) {
            return QStringLiteral("the recording pipeline did not start");
        }
        const GStreamer::GstElementPtr source = pipeline.element("src");
        constexpr GstClockTime kFrameDuration = GST_SECOND / GstTest::kStreamFrameRate;
        for (int frame = 0; frame < frames; ++frame) {
            if (pushAccessUnit(source.get(), units.at(frame).bytes, frame * kFrameDuration, kFrameDuration) !=
                GST_FLOW_OK) {
                return QStringLiteral("frame %1 was not taken").arg(frame);
            }
        }
        if (gst_app_src_end_of_stream(GST_APP_SRC(source.get())) != GST_FLOW_OK) {
            return QStringLiteral("the end of the stream was not taken");
        }
        failure = pipeline.runToEos();
        (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL);
        return failure;
    }

    GstTest::TestPipeline pipeline;
    GStreamer::GstElementPtr sinkBin;
};

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

    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral(
                 "appsrc name=src format=time "
                 "caps=video/x-h265,stream-format=byte-stream,alignment=au,width=32,height=32,framerate=4/1 ! "
                 "h265parse config-interval=-1 ! tee name=tee ! queue ! fakesink name=display sync=false async=false "
                 "tee. ! queue ! valve name=valve drop=true drop-mode=forward-sticky-events")),
             qPrintable(pipeline.error()));
    const GStreamer::GstElementPtr source = pipeline.element("src");
    const GStreamer::GstElementPtr recorderValve = pipeline.element("valve");

    std::atomic<int> displayedBuffers{0};
    const GStreamer::GstPadPtr displayPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("display").get(), "sink"));
    QVERIFY(displayPad);
    GStreamer::PadProbe displayProbe;
    QVERIFY(displayProbe.add(displayPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &displayedBuffers));

    QVERIFY(pipeline.play());
    QVERIFY(pushAccessUnit(source.get(), initialAu, 0) == GST_FLOW_OK);
    QTRY_VERIFY_WITH_TIMEOUT(displayedBuffers.load(std::memory_order_relaxed) >= 1, TestTimeout::shortMs());

    const GStreamer::GstPadPtr valveSrcPad =
        GStreamer::adoptPad(gst_element_get_static_pad(recorderValve.get(), "src"));
    QVERIFY(valveSrcPad);
    const GStreamer::GstCapsPtr recordingCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(valveSrcPad.get()));
    QVERIFY2(recordingCaps, "Closed recording valve did not preserve negotiated caps");

    const GStreamer::GstElementPtr fileSink =
        GStreamer::RecordingSink::create(outputPath, VideoReceiver::FileFormat::Mp4, recordingCaps.get());
    QVERIFY(fileSink);
    QVERIFY(gst_bin_add(pipeline.bin(), fileSink.get()));
    QVERIFY(gst_element_link(recorderValve.get(), fileSink.get()));
    QVERIFY(gst_element_sync_state_with_parent(fileSink.get()));
    g_object_set(recorderValve.get(), "drop", FALSE, nullptr);

    QVERIFY(pushAccessUnit(source.get(), secondAu, GST_SECOND / 4) == GST_FLOW_OK);
    QVERIFY(pushAccessUnit(source.get(), thirdAu, GST_SECOND / 2) == GST_FLOW_OK);
    QVERIFY(gst_app_src_end_of_stream(GST_APP_SRC(source.get())) == GST_FLOW_OK);

    const QString failure = pipeline.runToEos();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL);
    QVERIFY2(QFileInfo(outputPath).size() > 0, "Recording did not produce an MP4 file");

    QString verifyFailure;
    const int decoded = GstTest::countPlayableFrames(outputPath, verifyFailure, "qtdemux ! h265parse ! avdec_h265");
    QVERIFY2(decoded >= 2, qPrintable(QStringLiteral("%1 frames decoded %2").arg(decoded).arg(verifyFailure)));
}

void GstRecordingSinkTest::_testRecordingSinkTimestampsDtsOnlyFrames()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "appsink", "mp4mux", "qtdemux");

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString outputPath = tempDir.filePath(QStringLiteral("dts-only.mp4"));
    constexpr int kFrames = 20;
    SinkRecording recording;
    QString failure = recording.build(outputPath, VideoReceiver::FileFormat::Mp4);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    // After packet loss the parsers pass some frames on with only a DTS; qtmux alone would fail the recording.
    struct Stripped
    {
        int deltaFrames = 0;  ///< Touched only on the streaming thread
        std::atomic<int> count{0};
    } stripped;

    const GStreamer::GstPadPtr stripPad =
        GStreamer::adoptPad(gst_element_get_static_pad(recording.pipeline.element("parse").get(), "src"));
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

    failure = recording.record(kFrames);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QVERIFY2(stripped.count.load(std::memory_order_relaxed) > 0, "No frame lost its PTS; the test proves nothing");

    QString playbackFailure;
    const int frames = GstTest::countPlayableFrames(outputPath, playbackFailure);
    QVERIFY2(frames >= 0, qPrintable(playbackFailure));
    QCOMPARE(frames, kFrames);
}

void GstRecordingSinkTest::_testRecordingSinkTagsFile_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("muxer");
    // matroskamux writes the comment as COMMENTS, which FFmpeg passes on as no comment of its own.
    QTest::addColumn<bool>("commentReadable");

    QTest::newRow("mkv") << int(VideoReceiver::FileFormat::Mkv) << QStringLiteral("matroskamux") << false;
    QTest::newRow("mov") << int(VideoReceiver::FileFormat::Mov) << QStringLiteral("qtmux") << true;
    QTest::newRow("mp4") << int(VideoReceiver::FileFormat::Mp4) << QStringLiteral("mp4mux") << true;
}

void GstRecordingSinkTest::_testRecordingSinkTagsFile()
{
    QFETCH(int, format);
    QFETCH(QString, muxer);
    QFETCH(bool, commentReadable);
    const QByteArray muxerFactory = muxer.toLatin1();
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "h264parse", muxerFactory.constData());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto fileFormat = static_cast<VideoReceiver::FileFormat>(format);
    const QString outputPath = tempDir.filePath(QStringLiteral("tagged.") + VideoReceiver::fileExtension(fileFormat));
    const QDateTime started = QDateTime::currentDateTimeUtc();
    SinkRecording recording;
    QString failure = recording.build(outputPath, fileFormat);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    failure = recording.record(/*frames=*/10);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    // As other players (FFmpeg's) read them.
    QMediaPlayer player;
    player.setSource(QUrl::fromLocalFile(outputPath));
    QTRY_VERIFY_WITH_TIMEOUT(player.mediaStatus() == QMediaPlayer::LoadedMedia, TestTimeout::mediumMs());
    const QMediaMetaData metaData = player.metaData();
    if (commentReadable) {
        QCOMPARE(metaData.value(QMediaMetaData::Comment).toString(), VideoReceiver::recordingComment());
    }
    const QDateTime date = metaData.value(QMediaMetaData::Date).toDateTime();
    QVERIFY2(date.isValid(), qPrintable(metaData.stringValue(QMediaMetaData::Date)));
    QVERIFY2(qAbs(date.secsTo(started)) <= 60, qPrintable(date.toString(Qt::ISODate)));
}

void GstRecordingSinkTest::_testMkvRecordingIsSynced()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "h264parse", "matroskamux");

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    SinkRecording recording;
    QString failure = recording.build(tempDir.filePath(QStringLiteral("synced.mkv")), VideoReceiver::FileFormat::Mkv);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    // What filesink is given, after the bin's own probe. matroskamux asks for no sync: without one, storage that
    // caches writes never reports a full disk, and the recording is lost without an error.
    struct Written
    {
        std::atomic<int> buffers{0};
        std::atomic<int> synced{0};
        std::atomic<bool> firstSynced{false};
    } written;

    const GStreamer::GstElementPtr fileSink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(recording.sinkBin.get()), "filesink"));
    QVERIFY(fileSink);
    const GStreamer::GstPadPtr fileSinkPad = GStreamer::adoptPad(gst_element_get_static_pad(fileSink.get(), "sink"));
    GStreamer::PadProbe writtenProbe;
    QVERIFY(writtenProbe.add(
        fileSinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER,
        [](GstPad*, GstPadProbeInfo* info, gpointer data) -> GstPadProbeReturn {
            auto* const seen = static_cast<Written*>(data);
            const bool synced = GST_BUFFER_FLAG_IS_SET(GST_PAD_PROBE_INFO_BUFFER(info), GST_BUFFER_FLAG_SYNC_AFTER);
            if (seen->buffers.fetch_add(1, std::memory_order_relaxed) == 0) {
                seen->firstSynced.store(synced, std::memory_order_relaxed);
            }
            if (synced) {
                seen->synced.fetch_add(1, std::memory_order_relaxed);
            }
            return GST_PAD_PROBE_OK;
        },
        &written));

    failure = recording.record(/*frames=*/10);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    // From the first buffer, so that a disk already full is reported at once, and then now and again: not each one.
    QVERIFY(written.firstSynced.load(std::memory_order_relaxed));
    QVERIFY2(written.synced.load(std::memory_order_relaxed) < written.buffers.load(std::memory_order_relaxed),
             qPrintable(QStringLiteral("%1 of %2 buffers synced")
                            .arg(written.synced.load(std::memory_order_relaxed))
                            .arg(written.buffers.load(std::memory_order_relaxed))));
}

void GstRecordingSinkTest::_testRecordingIsSyncedAtItsEnd_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("muxer");

    QTest::newRow("mkv") << int(VideoReceiver::FileFormat::Mkv) << QStringLiteral("matroskamux");
    // qtmux asks for this sync itself, with the last thing it writes: the row tells when it no longer does.
    QTest::newRow("mp4") << int(VideoReceiver::FileFormat::Mp4) << QStringLiteral("mp4mux");
}

void GstRecordingSinkTest::_testRecordingIsSyncedAtItsEnd()
{
    QFETCH(int, format);
    QFETCH(QString, muxer);
    const QByteArray muxerFactory = muxer.toLatin1();
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "h264parse", muxerFactory.constData());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto fileFormat = static_cast<VideoReceiver::FileFormat>(format);

    // Whether the last buffer filesink is given asks for a sync, and what the file holds once the EOS that closes it
    // follows. A muxer finalizes its file at the EOS: without a sync after that, a power cut takes the recording's
    // end, and a disk that was full for it is never reported. Read once the pipeline, and its streaming threads with
    // it, has stopped.
    struct Ending
    {
        QString path;
        bool lastSynced = false;
        QByteArray written;
    } ending;

    ending.path = tempDir.filePath(QStringLiteral("ended.") + VideoReceiver::fileExtension(fileFormat));

    SinkRecording recording;
    QString failure = recording.build(ending.path, fileFormat);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    const GStreamer::GstElementPtr fileSink =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(recording.sinkBin.get()), "filesink"));
    QVERIFY(fileSink);
    const GStreamer::GstPadPtr fileSinkPad = GStreamer::adoptPad(gst_element_get_static_pad(fileSink.get(), "sink"));
    GStreamer::PadProbe endingProbe;
    QVERIFY(endingProbe.add(
        fileSinkPad.get(),
        static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
        [](GstPad*, GstPadProbeInfo* info, gpointer data) -> GstPadProbeReturn {
            auto* const seen = static_cast<Ending*>(data);
            if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
                seen->lastSynced = GST_BUFFER_FLAG_IS_SET(GST_PAD_PROBE_INFO_BUFFER(info), GST_BUFFER_FLAG_SYNC_AFTER);
            } else if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_EVENT(info)) == GST_EVENT_EOS) {
                QFile file(seen->path);
                if (file.open(QIODevice::ReadOnly)) {
                    seen->written = file.readAll();
                }
            }
            return GST_PAD_PROBE_OK;
        },
        &ending));

    failure = recording.record(/*frames=*/10);
    QVERIFY2(failure.isEmpty(), qPrintable(failure));

    QVERIFY2(ending.lastSynced, "The last buffer of the file asks for no sync");
    // filesink holds back what it is given, up to 64 KiB: a sync covers only what it has written.
    QFile file(ending.path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray finished = file.readAll();
    QVERIFY(!finished.isEmpty());
    QVERIFY2(ending.written == finished, "The file changed after its last sync");
}

UT_REGISTER_TEST(GstRecordingSinkTest, TestLabel::Integration)
