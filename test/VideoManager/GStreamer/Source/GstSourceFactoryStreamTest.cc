#include "GstSourceFactoryStreamTest.h"

#include <atomic>
#include <gst/gst.h>
#include <thread>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"
#include "PipelineStatsProbe.h"
#include "VideoStreamStats.h"

namespace {

/// RTP packets into rtph264depay and the units that reach h264parse after it.
struct DepayloaderOutput
{
    std::atomic<int> packetsIn{0};
    std::atomic<int> unitsOut{0};
    std::atomic<bool> firstUnitHasSps{false};
};

GstPadProbeReturn recordDepayloaderUnit([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* output = static_cast<DepayloaderOutput*>(data);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstMapInfo map;
    if ((output->unitsOut.fetch_add(1) == 0) && buffer && gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        const QByteArray unit = QByteArray::fromRawData(reinterpret_cast<const char*>(map.data), qsizetype(map.size));
        output->firstUnitHasSps = GstTest::nalTypes(unit, /*h265=*/false).contains(7);
        gst_buffer_unmap(buffer, &map);
    }
    return GST_PAD_PROBE_OK;
}

/// Counts the RTP packets into @p source's rtph264depay and records the units that reach its h264parse: at the
/// parser's sink rather than the depayloader's src, where the factory's own probe holds units back.
bool watchH264Depayloading(GstElement* source, DepayloaderOutput* output)
{
    const GStreamer::GstElementPtr depayloader = GstTest::findByFactory(source, "rtph264depay");
    const GStreamer::GstElementPtr parser = GstTest::findByFactory(source, "h264parse");
    const GStreamer::GstPadPtr depayloaderSink =
        GStreamer::adoptPad(depayloader ? gst_element_get_static_pad(depayloader.get(), "sink") : nullptr);
    const GStreamer::GstPadPtr parserSink =
        GStreamer::adoptPad(parser ? gst_element_get_static_pad(parser.get(), "sink") : nullptr);
    return depayloaderSink && parserSink &&
           gst_pad_add_probe(depayloaderSink.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers,
                             &output->packetsIn, nullptr) &&
           gst_pad_add_probe(parserSink.get(), GST_PAD_PROBE_TYPE_BUFFER, recordDepayloaderUnit, output, nullptr);
}

/// An RTP packet (payload type 96) carrying a slice of an H.264 picture that isn't a keyframe.
QByteArray makeH264RtpPacket(quint16 sequence, quint32 timestamp)
{
    QByteArray packet(16, '\0');
    auto* const bytes = reinterpret_cast<guint8*>(packet.data());
    bytes[0] = 0x80;
    bytes[1] = 96;
    GST_WRITE_UINT16_BE(bytes + 2, sequence);
    GST_WRITE_UINT32_BE(bytes + 4, timestamp);
    GST_WRITE_UINT32_BE(bytes + 8, 0x1234ABCD);
    bytes[12] = 0x41;
    return packet;
}

/// Links a source bin's pad, once it has one, to the sink given as user data.
void linkSourcePadToSink([[maybe_unused]] GstElement* source, GstPad* pad, gpointer sink)
{
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(GST_ELEMENT(sink), "sink"));
    if (sinkPad && !gst_pad_is_linked(sinkPad.get())) {
        (void) gst_pad_link(pad, sinkPad.get());
    }
}

/// An RTSP server on a free loopback port that takes a client through to PLAY of one H.264 stream, interleaved in the
/// RTSP connection, answers whatever it asks next, and records the methods it asked for. It sends no video.
class PlayingRtspServer
{
public:
    PlayingRtspServer()
    {
        QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this] {
            while (QTcpSocket* const socket = _server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] { _serve(socket); });
            }
        });
    }

    [[nodiscard]] bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }

    /// Over TCP only: no UDP ports to agree on.
    QString uri() const { return QStringLiteral("rtspt://127.0.0.1:%1/test").arg(_server.serverPort()); }

    const QList<QByteArray>& methods() const { return _methods; }

private:
    void _serve(QTcpSocket* socket)
    {
        QByteArray& pending = _pending[socket];
        pending += socket->readAll();
        while (!pending.isEmpty()) {
            // The client's RTCP, interleaved: '$', the channel, and the length in two bytes.
            if (pending.startsWith('$')) {
                if (pending.size() < 4) {
                    return;
                }
                const qsizetype length =
                    4 + GST_READ_UINT16_BE(reinterpret_cast<const guint8*>(pending.constData()) + 2);
                if (pending.size() < length) {
                    return;
                }
                pending.remove(0, length);
                continue;
            }
            const qsizetype end = pending.indexOf("\r\n\r\n");
            if (end < 0) {
                return;
            }
            const QList<QByteArray> lines = pending.left(end).split('\n');
            pending.remove(0, end + 4);

            const QByteArray method = lines.first().trimmed().split(' ').value(0);
            _methods.append(method);
            QByteArray reply = "RTSP/1.0 200 OK\r\n";
            for (const QByteArray& line : lines) {
                // The transport is the one the client proposes.
                if (line.toLower().startsWith("cseq:") || line.toLower().startsWith("transport:")) {
                    reply += line.trimmed() + "\r\n";
                }
            }
            if (method == "OPTIONS") {
                reply += "Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN\r\n\r\n";
            } else if (method == "DESCRIBE") {
                const QByteArray sdp =
                    "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\ns=QGC\r\nc=IN IP4 127.0.0.1\r\nt=0 0\r\n"
                    "m=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=control:trackID=1\r\n";
                reply += "Content-Base: " + uri().toUtf8() +
                         "/\r\nContent-Type: application/sdp\r\nContent-Length: " + QByteArray::number(sdp.size()) +
                         "\r\n\r\n" + sdp;
            } else {
                reply += "Session: 12345678\r\n\r\n";
            }
            (void) socket->write(reply);
        }
    }

    QTcpServer _server;
    QHash<QTcpSocket*, QByteArray> _pending;
    QList<QByteArray> _methods;
};

/// An mpegts:// source bin listening on a free loopback port, with the buffers it hands on counted at a fakesink, and
/// a sender that muxes each buffer it is given into a PES packet. The sender stays up after the last one, as nothing
/// tells a UDP receiver that a stream is over.
class MpegTsLoopback
{
public:
    /// Why the receiver doesn't play or the sender of @p mediaType isn't ready; empty when they are.
    QString start(const char* mediaType = "video/x-h264")
    {
        source = GstTest::createSource(QStringLiteral("mpegts://127.0.0.1:0"), GStreamer::SourceFactory::Config{});
        GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
        if (!source || !sink) {
            return QStringLiteral("no mpegts:// source bin or no fakesink");
        }
        g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
        if (!gst_bin_add(_receiver.bin(), source.get()) || !gst_bin_add(_receiver.bin(), sink)) {
            return QStringLiteral("the receiver's elements don't go into its pipeline");
        }
        (void) g_signal_connect(source.get(), "pad-added", G_CALLBACK(linkSourcePadToSink), sink);
        sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(sink, "sink"));
        if (!gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &frames, nullptr) ||
            !_receiver.play()) {
            return QStringLiteral("the receiver doesn't play");
        }
        gint port = 0;
        g_object_get(GstTest::findByFactory(source.get(), "udpsrc").get(), "port", &port, nullptr);
        if (port <= 0) {
            return QStringLiteral("the source listens on no port");
        }
        if (!_sender.launch(QStringLiteral("appsrc name=src format=time "
                                           "caps=%2,stream-format=byte-stream,alignment=au ! "
                                           "mpegtsmux ! udpsink host=127.0.0.1 port=%1 sync=false async=false")
                                .arg(port)
                                .arg(QLatin1StringView(mediaType))) ||
            !_sender.play()) {
            return QStringLiteral("the sender doesn't play: %1").arg(_sender.error());
        }
        _appSource = _sender.element("src");
        return {};
    }

    /// Sends @p payload as the PES packet of the picture @p frame, in the test streams' frame rate.
    bool send(const QByteArray& payload, qsizetype frame) const
    {
        GstBuffer* const buffer = gst_buffer_new_memdup(payload.constData(), static_cast<gsize>(payload.size()));
        GST_BUFFER_PTS(buffer) = frame * (GST_SECOND / GstTest::kStreamFrameRate);
        GstFlowReturn pushed = GST_FLOW_ERROR;
        g_signal_emit_by_name(_appSource.get(), "push-buffer", buffer, &pushed);
        gst_buffer_unref(buffer);
        return pushed == GST_FLOW_OK;
    }

    /// The "alignment" the source bin's @p parserFactory is told its input has; empty when told none, or without
    /// that parser.
    QByteArray parserInputAlignment(const char* parserFactory = "h264parse") const
    {
        const GStreamer::GstElementPtr parser = GstTest::findByFactory(source.get(), parserFactory);
        const GStreamer::GstPadPtr parserSink =
            GStreamer::adoptPad(parser ? gst_element_get_static_pad(parser.get(), "sink") : nullptr);
        const GStreamer::GstCapsPtr caps =
            GStreamer::adoptCaps(parserSink ? gst_pad_get_current_caps(parserSink.get()) : nullptr);
        return caps ? QByteArray(gst_structure_get_string(gst_caps_get_structure(caps.get(), 0), "alignment"))
                    : QByteArray();
    }

    GStreamer::GstElementPtr source;
    GStreamer::GstPadPtr sinkPad;
    std::atomic<int> frames{0};

private:
    GstTest::TestPipeline _receiver{gst_pipeline_new("mpegts-loopback")};
    GstTest::TestPipeline _sender;
    GStreamer::GstElementPtr _appSource;
};

}  // namespace

void GstSourceFactoryStreamTest::_testSourceFactoryRtspSetsUpOnlyVideoStream()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");

    // Audio listed first, as on many IP cameras; rtspsrc sets streams up in SDP order.
    GstTest::FakeRtspServer server(
        QByteArrayLiteral("v=0\r\n"
                          "o=- 0 0 IN IP4 127.0.0.1\r\n"
                          "s=QGC select-stream test\r\n"
                          "c=IN IP4 127.0.0.1\r\n"
                          "t=0 0\r\n"
                          "m=audio 0 RTP/AVP 97\r\n"
                          "a=rtpmap:97 OPUS/48000/2\r\n"
                          "a=control:trackID=0\r\n"
                          "m=video 0 RTP/AVP 96\r\n"
                          "a=rtpmap:96 H264/90000\r\n"
                          "a=control:trackID=1\r\n"));
    QVERIFY(server.listen());

    GstTest::TestPipeline pipeline(gst_pipeline_new("rtsp-select-stream-test"));
    QVERIFY(pipeline.get());

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(server.uri(), config);
    QVERIFY(bin);
    QVERIFY(gst_bin_add(pipeline.bin(), bin.get()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);

    QVERIFY_TRUE_WAIT(!server.setupUrls().isEmpty(), TestTimeout::mediumMs());
    QVERIFY2(server.setupUrls().first().endsWith("/trackID=1"),
             qPrintable(QStringLiteral("first SETUP must target the video track, got %1")
                            .arg(QString::fromUtf8(server.setupUrls().first()))));
}

void GstSourceFactoryStreamTest::_testSourceFactoryRtspEndsSessionWithoutPause()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");

    PlayingRtspServer server;
    QVERIFY(server.listen());

    GstTest::TestPipeline pipeline(gst_pipeline_new("rtsp-teardown-test"));
    const GStreamer::GstElementPtr bin = GstTest::createSource(server.uri(), GStreamer::SourceFactory::Config{});
    QVERIFY(bin);
    QVERIFY(gst_bin_add(pipeline.bin(), bin.get()));

    // Long enough for a TEARDOWN to be answered over a slow link, where rtspsrc's 100 ms isn't, and well within the
    // 2 s a receiver that is destroyed gives its pipeline to stop.
    guint64 teardownTimeout = 0;
    g_object_get(GstTest::findByFactory(bin.get(), "rtspsrc").get(), "teardown-timeout", &teardownTimeout, nullptr);
    QVERIFY2((teardownTimeout >= 500 * GST_MSECOND) && (teardownTimeout <= GST_SECOND),
             qPrintable(QString::number(teardownTimeout)));

    // rtspsrc tells in a PROGRESS message when it is done with a PLAY or PAUSE request.
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(pipeline.get()));
    int completed = 0;
    const auto requestsCompleted = [&bus, &completed]() {
        while (const GStreamer::GstMessagePtr message =
                   GStreamer::adoptMessage(gst_bus_pop_filtered(bus.get(), GST_MESSAGE_PROGRESS))) {
            GstProgressType type = GST_PROGRESS_TYPE_START;
            gchar* rawCode = nullptr;
            gchar* rawText = nullptr;
            gst_message_parse_progress(message.get(), &type, &rawCode, &rawText);
            const GStreamer::GCharPtr code = GStreamer::adoptGChar(rawCode);
            const GStreamer::GCharPtr text = GStreamer::adoptGChar(rawText);
            if ((type == GST_PROGRESS_TYPE_COMPLETE) && (g_strcmp0(code.get(), "request") == 0)) {
                ++completed;
            }
        }
        return completed;
    };
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    QVERIFY_TRUE_WAIT(requestsCompleted() == 1, TestTimeout::mediumMs());
    QVERIFY(server.methods().contains("PLAY"));

    // A pipeline stops by way of PAUSED, where rtspsrc sends a PAUSE, which a live camera may answer with an error,
    // unless the pipeline is on to READY before it gets to it. A receiver's pipeline takes longer than that; this one
    // has only the source, and waits in PAUSED instead.
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
    QVERIFY_TRUE_WAIT(requestsCompleted() == 2, TestTimeout::mediumMs());

    // Stopped on another thread, as the receiver's worker stops it, while the server here goes on answering.
    std::atomic<bool> stopped{false};
    std::thread stopper([&pipeline, &stopped] {
        (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL);
        stopped = true;
    });
    const bool stoppedInTime = QTest::qWaitFor([&stopped] { return stopped.load(); }, TestTimeout::mediumMs());
    stopper.join();
    QVERIFY(stoppedInTime);

    QVERIFY_TRUE_WAIT(server.methods().contains("TEARDOWN"), TestTimeout::mediumMs());
    QVERIFY2(!server.methods().contains("PAUSE"), server.methods().join(' ').constData());
}

void GstSourceFactoryStreamTest::_testSourceFactoryDropsRtpUntilParameterSets()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "appsink");

    DepayloaderOutput output;
    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr source = GstTest::createSource(QStringLiteral("udp://127.0.0.1:0"), config);
    QVERIFY(source);
    QVERIFY(watchH264Depayloading(source.get(), &output));

    GstTest::TestPipeline receiver(gst_pipeline_new("join-mid-gop"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    QVERIFY(gst_element_link(source.get(), sink));
    QVERIFY(receiver.play());
    gint port = 0;
    g_object_get(GstTest::findByFactory(source.get(), "udpsrc").get(), "port", &port, nullptr);
    QVERIFY(port > 0);

    // The receiver joins a stream whose keyframes, and the parameter sets sent with them, it missed.
    GstTest::FixtureRtpSender sender;
    QVERIFY2(sender.error().isEmpty(), qPrintable(sender.error()));
    sender.holdKeyframes(true);
    QVERIFY(sender.start(static_cast<quint16>(port)));

    QVERIFY_TRUE_WAIT(output.packetsIn.load() >= 20, TestTimeout::mediumMs());
    QCOMPARE(output.unitsOut.load(), 0);

    sender.holdKeyframes(false);
    QVERIFY_TRUE_WAIT(output.unitsOut.load() > 0, TestTimeout::mediumMs());
    QVERIFY2(output.firstUnitHasSps.load(), "the first unit reaching h264parse must carry the SPS");
}

void GstSourceFactoryStreamTest::_testStatsCountPacketsDroppedOnLatency()
{
    // The RTP timestamps of the packets are a second apart, so the third one is the jitter buffer's latency after the
    // first.
    constexpr quint32 kPacketSpacing = 90000;

    GStreamer::SourceFactory::Config config;
    config.latencyMs = 2000;
    const GStreamer::GstElementPtr source = GstTest::createSource(QStringLiteral("udp://127.0.0.1:0"), config);
    QVERIFY(source);
    const GStreamer::GstElementPtr jitterBuffer = GstTest::findByFactory(source.get(), "rtpjitterbuffer");
    QVERIFY(jitterBuffer);

    GstTest::TestPipeline receiver(gst_pipeline_new("drop-on-latency"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    QVERIFY(gst_element_link(source.get(), sink));
    QVERIFY(receiver.play());
    gint port = 0;
    g_object_get(GstTest::findByFactory(source.get(), "udpsrc").get(), "port", &port, nullptr);
    QVERIFY(port > 0);

    QUdpSocket sender;
    int sent = 0;
    const auto sendPacket = [&sender, &sent, port]() {
        const QByteArray packet =
            makeH264RtpPacket(static_cast<quint16>(100 + sent), static_cast<quint32>(sent) * kPacketSpacing);
        ++sent;
        return sender.writeDatagram(packet, QHostAddress::LocalHost, static_cast<quint16>(port)) == packet.size();
    };

    // As PipelineSession does with its bus.
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(receiver.get()));
    PipelineStatsProbe probe;
    const auto packetsLost = [&]() {
        while (const GStreamer::GstMessagePtr message =
                   GStreamer::adoptMessage(gst_bus_pop_filtered(bus.get(), GST_MESSAGE_ELEMENT))) {
            PipelineStatsProbe::noteElementMessage(message.get());
        }
        VideoStreamStatsMeter::Counters counters;
        VideoStreamStats stats;
        probe.sample(nullptr, source.get(), nullptr, counters, stats);
        return counters.rtp ? counters.rtp->packetsLost : 0;
    };
    // A burst, as after a stalled link: the packets of many seconds arrive within the two that the jitter buffer holds
    // the stream's first packets for. None is missing, but it keeps no more than its latency and drops the oldest ones.
    // The packets go on until it has: it drops none while the events that start the stream are still ahead of them.
    QVERIFY_TRUE_WAIT(sendPacket() && (packetsLost() > 0), TestTimeout::mediumMs());

    // Its own count of lost packets is of those that never arrived.
    GstStructure* rawStats = nullptr;
    g_object_get(jitterBuffer.get(), "stats", &rawStats, nullptr);
    const GStreamer::GstStructurePtr stats = GStreamer::adoptStructure(rawStats);
    guint64 neverArrived = 1;
    QVERIFY(stats && gst_structure_get_uint64(stats.get(), "num-lost", &neverArrived));
    QCOMPARE(neverArrived, guint64(0));
}

void GstSourceFactoryStreamTest::_testMpegTsStreamObjectCarriesParsedCaps()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "mpegtsmux", "tsdemux");

    MpegTsLoopback loopback;
    const QString startError = loopback.start();
    QVERIFY2(startError.isEmpty(), qPrintable(startError));

    // The test stream in MPEG-TS, all of it at once.
    QString loadError;
    const QList<GstTest::AccessUnit> units =
        GstTest::loadAccessUnits(QString::fromLatin1(GstTest::kH264Stream), &loadError);
    QVERIFY2(!units.isEmpty(), qPrintable(loadError));
    for (qsizetype i = 0; i < units.size(); ++i) {
        QVERIFY(loopback.send(units.at(i).bytes, i));
    }
    QVERIFY_TRUE_WAIT(loopback.frames.load() > 0, TestTimeout::mediumMs());

    // decodebin3 picks the decoder by the caps on the stream object. tsdemux's has those of the unparsed stream,
    // which no decoder takes.
    const GStreamer::GstEventPtr streamStart =
        GStreamer::adoptEvent(gst_pad_get_sticky_event(loopback.sinkPad.get(), GST_EVENT_STREAM_START, 0));
    QVERIFY(streamStart);
    GstStream* rawStream = nullptr;
    gst_event_parse_stream(streamStart.get(), &rawStream);
    const GStreamer::GstObjectPtr stream = GStreamer::adoptObject(GST_OBJECT_CAST(rawStream));
    QVERIFY2(stream, "tsdemux sent no stream object");
    const GStreamer::GstCapsPtr streamCaps = GStreamer::adoptCaps(gst_stream_get_caps(GST_STREAM_CAST(stream.get())));
    const GStreamer::GstCapsPtr padCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(loopback.sinkPad.get()));
    QVERIFY(streamCaps && padCaps);
    const GStreamer::GCharPtr streamCapsText = GStreamer::adoptGChar(gst_caps_to_string(streamCaps.get()));
    QVERIFY2(gst_caps_is_equal(streamCaps.get(), padCaps.get()), streamCapsText.get());
}

void GstSourceFactoryStreamTest::_testMpegTsFrameLeavesBeforeTheNextArrives_data()
{
    QTest::addColumn<QString>("stream");
    QTest::addColumn<QByteArray>("mediaType");
    QTest::addColumn<QByteArray>("parser");

    QTest::newRow("h264") << QString::fromLatin1(GstTest::kH264Stream) << QByteArrayLiteral("video/x-h264")
                          << QByteArrayLiteral("h264parse");
    QTest::newRow("h265") << QString::fromLatin1(GstTest::kH265Stream) << QByteArrayLiteral("video/x-h265")
                          << QByteArrayLiteral("h265parse");
}

void GstSourceFactoryStreamTest::_testMpegTsFrameLeavesBeforeTheNextArrives()
{
    QFETCH(QString, stream);
    QFETCH(QByteArray, mediaType);
    QFETCH(QByteArray, parser);
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "mpegtsmux", "tsdemux", parser.constData());

    MpegTsLoopback loopback;
    const QString startError = loopback.start(mediaType.constData());
    QVERIFY2(startError.isEmpty(), qPrintable(startError));

    QString loadError;
    const QList<GstTest::AccessUnit> units = GstTest::loadAccessUnits(stream, &loadError);
    QVERIFY2(units.size() > 2, qPrintable(loadError));
    for (qsizetype i = 0; i < units.size(); ++i) {
        QVERIFY(loopback.send(units.at(i).bytes, i));
    }

    // The last frame sent arrives too. tsdemux hands on a PES packet that tells its length, as these do, once it has
    // it all, and the parser keeps none back: told that each packet holds whole frames, it needn't see the next frame
    // to end one.
    QCOMPARE_TRUE_WAIT(loopback.frames.load(), int(units.size()), TestTimeout::mediumMs());
    QCOMPARE(loopback.parserInputAlignment(parser.constData()), QByteArrayLiteral("au"));
}

void GstSourceFactoryStreamTest::_testMpegTsPacketWithinFrameEndsAlignment()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "mpegtsmux", "tsdemux", "h264parse");

    MpegTsLoopback loopback;
    const QString startError = loopback.start();
    QVERIFY2(startError.isEmpty(), qPrintable(startError));

    QString loadError;
    const QList<GstTest::AccessUnit> units =
        GstTest::loadAccessUnits(QString::fromLatin1(GstTest::kH264Stream), &loadError);
    QVERIFY2(units.size() > 3, qPrintable(loadError));

    // A sender that cuts a frame in two PES packets: the second one starts within the frame.
    expectLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("starts within a frame")));
    const QByteArray& cut = units.at(1).bytes;
    QVERIFY(loopback.send(units.at(0).bytes, 0));
    QVERIFY(loopback.send(cut.left(cut.size() / 2), 1));
    QVERIFY(loopback.send(cut.mid(cut.size() / 2), 1));
    for (qsizetype i = 2; i < units.size(); ++i) {
        QVERIFY(loopback.send(units.at(i).bytes, i));
    }

    // The parser is no longer told that each packet holds whole frames, so it finds their ends itself.
    QVERIFY_TRUE_WAIT((loopback.frames.load() > 2) && loopback.parserInputAlignment().isEmpty(),
                      TestTimeout::mediumMs());
    verifyExpectedLogMessage();
}

void GstSourceFactoryStreamTest::_testUdpSourceRefusesPortInUse()
{
    const GStreamer::SourceFactory::Config config;
    const GstTest::TestPipeline listener(GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:0"), config));
    QVERIFY(listener.get());
    // udpsrc binds its socket on the way to READY.
    QCOMPARE(gst_element_set_state(listener.get(), GST_STATE_READY), GST_STATE_CHANGE_SUCCESS);
    gint port = 0;
    g_object_get(GstTest::findByFactory(listener.get(), "udpsrc").get(), "port", &port, nullptr);
    QVERIFY(port > 0);

    // A second listener on the port would get some streams' packets, the first one the others', and neither an error.
    const GstTest::TestPipeline second(
        GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:%1").arg(port), config));
    QVERIFY(second.get());
    QCOMPARE(gst_element_set_state(second.get(), GST_STATE_READY), GST_STATE_CHANGE_FAILURE);
}

void GstSourceFactoryStreamTest::_testUdpSourceStopsRightAfterPlaying()
{
    // An unfixed udpsrc corrupts the heap in most runs of a few thousand cycles; each takes well under a millisecond.
    constexpr int kImmediateStopCycles = 10000;

    // The factory's UDP sources are udpsrc. Before gst-plugins-good 1.24.10, leaving PLAYING while its streaming thread
    // still clears the previous unlock cancels a freed GCancellable and corrupts the heap; GStreamer::completeInit()
    // serializes the two. Downstream elements delay the source's own state change and hide the race, so the source
    // runs bare.
    for (int cycle = 0; cycle < kImmediateStopCycles; ++cycle) {
        GstTest::TestPipeline pipeline;
        QVERIFY2(pipeline.launch(QStringLiteral("udpsrc address=127.0.0.1 port=0 ! fakesink")),
                 qPrintable(pipeline.error()));
        QVERIFY(pipeline.play());
        QCOMPARE(gst_element_set_state(pipeline.get(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
}

UT_REGISTER_TEST(GstSourceFactoryStreamTest, TestLabel::Integration)
