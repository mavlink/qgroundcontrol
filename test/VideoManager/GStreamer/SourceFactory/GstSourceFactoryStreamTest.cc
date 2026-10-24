#include "GstSourceFactoryStreamTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QtCore/QHash>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"

namespace {

/// RTP packets into rtph264depay and the units that reach h264parse after it.
struct DepayloaderOutput
{
    std::atomic<int> packetsIn{0};
    std::atomic<int> unitsOut{0};
    std::atomic<bool> firstUnitHasSps{false};
};

bool containsH264Sps(const guint8* data, gsize size)
{
    for (gsize i = 0; (i + 3) < size; ++i) {
        if ((data[i] == 0) && (data[i + 1] == 0) && (data[i + 2] == 1) && ((data[i + 3] & 0x1FU) == 7U)) {
            return true;
        }
    }
    return false;
}

GstPadProbeReturn recordDepayloaderUnit([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* output = static_cast<DepayloaderOutput*>(data);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstMapInfo map;
    if ((output->unitsOut.fetch_add(1) == 0) && buffer && gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        output->firstUnitHasSps = containsH264Sps(map.data, map.size);
        gst_buffer_unmap(buffer, &map);
    }
    return GST_PAD_PROBE_OK;
}

// Units are recorded at h264parse's sink rather than the depayloader's src: the source bin sees
// deep-element-added before parsebin's own handlers, so a probe added here would run before the factory's.
void watchH264Depayloading([[maybe_unused]] GstBin* bin, [[maybe_unused]] GstBin* subBin, GstElement* element,
                           gpointer data)
{
    GstElementFactory* factory = gst_element_get_factory(element);
    const char* factoryName = factory ? gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)) : nullptr;
    auto* output = static_cast<DepayloaderOutput*>(data);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(element, "sink"));
    if (g_strcmp0(factoryName, "rtph264depay") == 0) {
        (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &output->packetsIn,
                                 nullptr);
    } else if (g_strcmp0(factoryName, "h264parse") == 0) {
        (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, recordDepayloaderUnit, output, nullptr);
    }
}

void linkToSink([[maybe_unused]] GstElement* element, GstPad* pad, gpointer data)
{
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(GST_ELEMENT(data), "sink"));
    if (sinkPad && !gst_pad_is_linked(sinkPad.get())) {
        (void) gst_pad_link(pad, sinkPad.get());
    }
}

// Minimal RTSP responder for driving a real rtspsrc without gst-rtsp-server: answers OPTIONS/DESCRIBE with a
// fixed SDP and records each SETUP URL without answering it, so the client stops right after picking streams.
class FakeRtspServer
{
public:
    explicit FakeRtspServer(const QByteArray& sdp)
        : _sdp(sdp)
    {
        QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this] {
            while (QTcpSocket* socket = _server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] { _serve(socket); });
            }
        });
    }

    bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }

    QString url() const { return QStringLiteral("rtsp://127.0.0.1:%1/test").arg(_server.serverPort()); }

    const QList<QByteArray>& setupUrls() const { return _setupUrls; }

private:
    void _serve(QTcpSocket* socket)
    {
        QByteArray& pending = _pending[socket];
        pending += socket->readAll();
        for (qsizetype end = pending.indexOf("\r\n\r\n"); end >= 0; end = pending.indexOf("\r\n\r\n")) {
            const QList<QByteArray> lines = pending.left(end).split('\n');
            pending.remove(0, end + 4);

            const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
            const QByteArray method = requestLine.value(0);
            QByteArray cseq;
            for (const QByteArray& line : lines) {
                if (line.toLower().startsWith("cseq:")) {
                    cseq = line.mid(5).trimmed();
                }
            }

            if (method == "SETUP") {
                _setupUrls.append(requestLine.value(1));
                continue;
            }

            QByteArray reply = "RTSP/1.0 200 OK\r\nCSeq: " + cseq + "\r\n";
            if (method == "OPTIONS") {
                reply += "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n\r\n";
            } else if (method == "DESCRIBE") {
                reply += "Content-Base: " + url().toUtf8() +
                         "/\r\nContent-Type: application/sdp\r\nContent-Length: " + QByteArray::number(_sdp.size()) +
                         "\r\n\r\n" + _sdp;
            } else {
                reply += "\r\n";
            }
            (void) socket->write(reply);
        }
    }

    QTcpServer _server;
    QByteArray _sdp;
    QHash<QTcpSocket*, QByteArray> _pending;
    QList<QByteArray> _setupUrls;
};

}  // namespace

void GstSourceFactoryStreamTest::_testSourceFactoryRtspSetsUpOnlyVideoStream()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg, GstTest::libproxyIgnoreMaskWarning());

    // Audio listed first, as on many IP cameras; rtspsrc sets streams up in SDP order.
    FakeRtspServer server(
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
    const GStreamer::GstElementPtr bin = GstTest::createSource(server.url(), config);
    QVERIFY(bin);
    QVERIFY(gst_bin_add(pipeline.bin(), bin.get()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);

    QVERIFY_TRUE_WAIT(!server.setupUrls().isEmpty(), TestTimeout::mediumMs());
    QVERIFY2(server.setupUrls().first().endsWith("/trackID=1"),
             qPrintable(QStringLiteral("first SETUP must target the video track, got %1")
                            .arg(QString::fromUtf8(server.setupUrls().first()))));
}

void GstSourceFactoryStreamTest::_testSourceFactoryDropsRtpUntilParameterSets()
{
    const QString missing = GstTest::firstMissingFactory(
        {"videotestsrc", "x264enc", "rtph264pay", "udpsink", "udpsrc", "parsebin", "rtph264depay", "fakesink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    QUdpSocket portProbe;
    QVERIFY(portProbe.bind(QHostAddress::LocalHost, 0));
    const quint16 port = portProbe.localPort();
    portProbe.close();

    DepayloaderOutput output;
    std::atomic<int> sentFrames{0};

    // A GOP far longer than the test: the receiver joins after the only keyframe until the test forces one.
    GstTest::TestPipeline sender;
    QVERIFY2(
        sender.launch(QStringLiteral("videotestsrc is-live=true ! video/x-raw,width=160,height=120,framerate=30/1 ! "
                                     "x264enc name=encoder tune=zerolatency speed-preset=ultrafast "
                                     "key-int-max=100000 ! rtph264pay config-interval=-1 pt=96 ! "
                                     "udpsink host=127.0.0.1 port=%1 sync=false async=false")
                          .arg(port)),
        qPrintable(sender.error()));
    const GStreamer::GstElementPtr encoder = sender.element("encoder");
    QVERIFY(encoder);
    const GStreamer::GstPadPtr encoderSrc = GStreamer::adoptPad(gst_element_get_static_pad(encoder.get(), "src"));
    (void) gst_pad_add_probe(encoderSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &sentFrames, nullptr);

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr source =
        GstTest::createSource(QStringLiteral("udp://127.0.0.1:%1").arg(port), config);
    QVERIFY(source);
    (void) g_signal_connect(source.get(), "deep-element-added", G_CALLBACK(watchH264Depayloading), &output);

    GstTest::TestPipeline receiver(gst_pipeline_new("join-mid-gop"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    (void) g_signal_connect(source.get(), "pad-added", G_CALLBACK(linkToSink), sink);

    QVERIFY(sender.play());
    QVERIFY_TRUE_WAIT(sentFrames.load() >= 5, TestTimeout::mediumMs());
    QVERIFY(receiver.play());

    QVERIFY_TRUE_WAIT(output.packetsIn.load() >= 20, TestTimeout::mediumMs());
    QCOMPARE(output.unitsOut.load(), 0);

    QVERIFY(gst_pad_send_event(encoderSrc.get(),
                               gst_video_event_new_upstream_force_key_unit(GST_CLOCK_TIME_NONE, TRUE, 0)));
    QVERIFY_TRUE_WAIT(output.unitsOut.load() > 0, TestTimeout::mediumMs());
    QVERIFY2(output.firstUnitHasSps.load(), "the first unit reaching h264parse must carry the SPS");
}

#endif

UT_REGISTER_TEST(GstSourceFactoryStreamTest, TestLabel::Integration)
