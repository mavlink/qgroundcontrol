#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QBuffer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QImage>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include "GstSourceFactory.h"
#include "LocalHttpTestServer.h"
#include "QGCNetworkHelper.h"

namespace {

GstSample* tryPullSampleOrPreroll(GstAppSink* sink)
{
    if (GstSample* sample = gst_app_sink_try_pull_sample(sink, 0)) {
        return sample;
    }
    return gst_app_sink_try_pull_preroll(sink, 0);
}

// Borrowed (bin-owned) first child whose element-factory name matches, or nullptr.
GstElement* findChildByFactoryName(GstElement* bin, const char* factoryName)
{
    GstIterator* it = gst_bin_iterate_elements(GST_BIN(bin));
    GValue item = G_VALUE_INIT;
    GstElement* match = nullptr;
    bool done = false;
    while (!done) {
        switch (gst_iterator_next(it, &item)) {
            case GST_ITERATOR_OK: {
                GstElement* child = GST_ELEMENT(g_value_get_object(&item));
                GstElementFactory* f = gst_element_get_factory(child);
                if (f && (g_strcmp0(GST_OBJECT_NAME(f), factoryName) == 0)) {
                    match = child;
                    done = true;
                }
                g_value_reset(&item);
                break;
            }
            case GST_ITERATOR_RESYNC:
                gst_iterator_resync(it);
                break;
            default:
                done = true;
                break;
        }
    }
    g_value_unset(&item);
    gst_iterator_free(it);
    return match;
}

}  // namespace

void GStreamerTest::_testSourceFactoryUdpRtpJitterBuffer()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    config.latencyMs = 80;
    config.doRetransmission = true;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* jb = findChildByFactoryName(bin, "rtpjitterbuffer");
    QVERIFY2(jb, "static UDP-RTP path must insert an rtpjitterbuffer at NULL state");

    guint latency = 0;
    gboolean doLost = FALSE, doRtx = FALSE, dropOnLatency = FALSE;
    gint rtxDelay = 0, rtxMaxRetries = 0;
    g_object_get(jb, "latency", &latency, "do-lost", &doLost, "do-retransmission", &doRtx, "drop-on-latency",
                 &dropOnLatency, "rtx-delay", &rtxDelay, "rtx-max-retries", &rtxMaxRetries, nullptr);
    QCOMPARE(latency, 80u);
    QCOMPARE(doLost, TRUE);
    QCOMPARE(doRtx, TRUE);
    QCOMPARE(dropOnLatency, TRUE);
    QCOMPARE(rtxDelay, 25);
    QCOMPARE(rtxMaxRetries, 1);
}

void GStreamerTest::_testSourceFactoryJitterBufferNone()
{
    if (!gst_element_factory_find("udpsrc")) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::None;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "JitterBuffer::None must link the source straight to the parser");
}

void GStreamerTest::_testSourceFactoryNoRetransmission()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::Buffered;
    config.doRetransmission = false;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* jb = findChildByFactoryName(bin, "rtpjitterbuffer");
    QVERIFY(jb);

    gboolean doRtx = TRUE, dropOnLatency = TRUE;
    g_object_get(jb, "do-retransmission", &doRtx, "drop-on-latency", &dropOnLatency, nullptr);
    QCOMPARE(doRtx, FALSE);
    QCOMPARE(dropOnLatency, FALSE);
}

void GStreamerTest::_testSourceFactoryRtspExcludesStaticJitterBuffer()
{
    if (!gst_element_factory_find("rtspsrc")) {
        QSKIP("rtspsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "rtspsrc owns its internal jitterbuffer; the factory must not add a second one");
}

void GStreamerTest::_testSourceFactoryHttpMjpeg()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse")) {
        QSKIP("souphttpsrc/multipartdemux/jpegparse plugins unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.timeoutS = 9;
    GstElement* bin = GStreamer::SourceFactory::create(
        QStringLiteral("https://video.example.test:8443/camera.mjpg?quality=80#local-view"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* source = findChildByFactoryName(bin, "souphttpsrc");
    GstElement* demux = findChildByFactoryName(bin, "multipartdemux");
    GstElement* parser = findChildByFactoryName(bin, "jpegparse");
    QVERIFY(source);
    QVERIFY(demux);
    QVERIFY(parser);

    gchar* location = nullptr;
    gchar* method = nullptr;
    gboolean isLive = FALSE;
    gboolean doTimestamp = FALSE;
    gboolean keepAlive = FALSE;
    gboolean compress = TRUE;
    gboolean iradioMode = TRUE;
    gboolean automaticRedirect = TRUE;
    gboolean sslStrict = FALSE;
    gboolean singleStream = FALSE;
    gint retries = -1;
    guint timeout = 0;
    gint httpLogLevel = -1;
    gchar* userAgent = nullptr;
    g_object_get(source, "location", &location, "method", &method, "is-live", &isLive, "do-timestamp", &doTimestamp,
                 "keep-alive", &keepAlive, "compress", &compress, "iradio-mode", &iradioMode, "automatic-redirect",
                 &automaticRedirect, "retries", &retries, "timeout", &timeout, "ssl-strict", &sslStrict,
                 "http-log-level", &httpLogLevel, "user-agent", &userAgent, nullptr);
    const auto stringsCleanup = qScopeGuard([&] {
        g_free(location);
        g_free(method);
        g_free(userAgent);
    });
    g_object_get(demux, "single-stream", &singleStream, nullptr);

    QCOMPARE(QString::fromUtf8(location), QStringLiteral("https://video.example.test:8443/camera.mjpg?quality=80"));
    QCOMPARE(QString::fromUtf8(method), QStringLiteral("GET"));
    QCOMPARE(isLive, TRUE);
    QCOMPARE(doTimestamp, TRUE);
    QCOMPARE(keepAlive, TRUE);
    QCOMPARE(compress, FALSE);
    QCOMPARE(iradioMode, FALSE);
    QCOMPARE(automaticRedirect, FALSE);
    QCOMPARE(retries, 0);
    QCOMPARE(timeout, 9u);
    QCOMPARE(sslStrict, TRUE);
    QCOMPARE(httpLogLevel, 0);
    QCOMPARE(QString::fromUtf8(userAgent), QGCNetworkHelper::defaultUserAgent());
    QCOMPARE(singleStream, TRUE);

    static GstStaticPadTemplate jpegPadTemplate =
        GST_STATIC_PAD_TEMPLATE("src_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS("image/jpeg"));
    GstPad* jpegPad = gst_pad_new_from_static_template(&jpegPadTemplate, "src_0");
    QVERIFY(jpegPad);
    QVERIFY2(gst_element_add_pad(demux, jpegPad), "multipartdemux test pad must be accepted");

    GstPad* parserSink = gst_element_get_static_pad(parser, "sink");
    QVERIFY(parserSink);
    const auto parserSinkCleanup = qScopeGuard([&] { gst_object_unref(parserSink); });
    QVERIFY2(gst_pad_is_linked(parserSink), "multipart JPEG pad-added must link to jpegparse");
    GstPad* peer = gst_pad_get_peer(parserSink);
    QVERIFY(peer);
    QCOMPARE(peer, jpegPad);
    gst_object_unref(peer);
    QVERIFY(gst_element_remove_pad(demux, jpegPad));

    GstPad* srcPad = gst_element_get_static_pad(bin, "src");
    QVERIFY2(srcPad, "HTTP MJPEG source bin must expose a static parsed-JPEG source pad");
    gst_object_unref(srcPad);
}

void GStreamerTest::_testSourceFactoryHttpMjpegDelivery()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse") || !gst_element_factory_find("appsink")) {
        QSKIP("souphttpsrc/multipartdemux/jpegparse/appsink plugins unavailable");
    }

    QByteArray jpeg;
    QBuffer jpegBuffer(&jpeg);
    QVERIFY(jpegBuffer.open(QIODevice::WriteOnly));
    QImage image(16, 16, QImage::Format_RGB32);
    image.fill(Qt::green);
    QVERIFY2(image.save(&jpegBuffer, "JPEG"), "Qt JPEG encoder unavailable");

    const QByteArray boundary("qgc-test-boundary");
    const QByteArray framePart = "--" + boundary +
                                 "\r\n"
                                 "Content-Type: image/jpeg\r\n"
                                 "Content-Length: " +
                                 QByteArray::number(jpeg.size()) + "\r\n\r\n" + jpeg + "\r\n";
    // A multipart video stream contains repeated images. Supplying two also
    // verifies delivery after multipartdemux exposes and links its dynamic pad.
    const QByteArray body = framePart + framePart + "--" + boundary + "--\r\n";
    const QByteArray response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=" +
        boundary +
        "\r\n"
        "Connection: close\r\n"
        "Content-Length: " +
        QByteArray::number(body.size()) + "\r\n\r\n" + body;

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local MJPEG test server");

    GStreamer::SourceFactory::Config config;
    config.timeoutS = 5;
    GstElement* source = GStreamer::SourceFactory::create(server.url(QStringLiteral("/video_feed")), config);
    GstElement* sink = gst_element_factory_make("appsink", "sink");
    GstElement* pipeline = gst_pipeline_new("http-mjpeg-delivery-test");
    if (!source || !sink || !pipeline) {
        gst_clear_object(&source);
        gst_clear_object(&sink);
        gst_clear_object(&pipeline);
        QFAIL("Could not create HTTP MJPEG delivery pipeline");
    }
    const auto pipelineCleanup = qScopeGuard([&] {
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    });

    g_object_set(sink, "sync", FALSE, "max-buffers", 1u, "drop", TRUE, nullptr);
    gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
    QVERIFY2(gst_element_link(source, sink), "Could not link HTTP MJPEG source to appsink");
    QVERIFY2(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
             "HTTP MJPEG delivery pipeline failed to start");

    QTcpSocket* client = server.waitForConnection(TestTimeout::mediumMs());
    QVERIFY2(client, "HTTP MJPEG source did not connect to the local test server");
    const auto clientCleanup = qScopeGuard([&] {
        client->disconnectFromHost();
        client->deleteLater();
    });
    if (client->bytesAvailable() == 0) {
        QVERIFY2(client->waitForReadyRead(TestTimeout::mediumMs()), "HTTP MJPEG source did not send a request");
    }
    const QByteArray request = client->readAll();
    QVERIFY2(request.startsWith("GET /video_feed HTTP/1.1\r\n"), "HTTP MJPEG source sent an unexpected request");
    QCOMPARE(client->write(response), response.size());
    QVERIFY2(client->waitForBytesWritten(TestTimeout::mediumMs()), "Could not deliver the local MJPEG response");

    GstSample* sample = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((sample = tryPullSampleOrPreroll(GST_APP_SINK(sink))) != nullptr, TestTimeout::mediumMs());
    const auto sampleCleanup = qScopeGuard([&] { gst_sample_unref(sample); });
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    QVERIFY(buffer);
    QVERIFY(gst_buffer_get_size(buffer) > 0);
    GstCaps* caps = gst_sample_get_caps(sample);
    QVERIFY(caps);
    QCOMPARE(QString::fromUtf8(gst_structure_get_name(gst_caps_get_structure(caps, 0))), QStringLiteral("image/jpeg"));
}

void GStreamerTest::_testSourceFactoryRejectsUnsafeHttpMjpegUrl()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid HTTP MJPEG URL")));
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("HTTP MJPEG credentials in URLs are not supported")));

    GStreamer::SourceFactory::Config config;
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("http:///camera.mjpg"), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("http://video.example.test:0/camera.mjpg"), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("https://operator:secret@video.example.test/camera.mjpg"),
                                              config));
}

void GStreamerTest::_testSourceFactoryRejectsBadUri()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("URI is not specified|Invalid UDP port")));
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unsupported URI scheme")));

    GStreamer::SourceFactory::Config config;
    QVERIFY(!GStreamer::SourceFactory::create(QString(), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("ftp://127.0.0.1/x"), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:0"), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:99999"), config));
}

void GStreamerTest::_testSourceFactoryTcpMpegTs()
{
    if (!gst_element_factory_find("tcpclientsrc") || !gst_element_factory_find("tsdemux")) {
        QSKIP("tcpclientsrc/tsdemux plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("tcp://192.168.1.50:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "tcpclientsrc");
    QVERIFY2(src, "tcp:// must build a tcpclientsrc");
    gchar* host = nullptr;
    gint port = 0;
    g_object_get(src, "host", &host, "port", &port, nullptr);
    const auto hostCleanup = qScopeGuard([&] { g_free(host); });
    QCOMPARE(QString::fromUtf8(host), QStringLiteral("192.168.1.50"));
    QCOMPARE(port, 5600);

    QVERIFY2(findChildByFactoryName(bin, "tsdemux"), "MPEG-TS over TCP must insert tsdemux explicitly");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactoryRejectsBadTcpUri()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("Invalid TCP port|Missing host in TCP URI")));

    GStreamer::SourceFactory::Config config;
    QVERIFY2(!GStreamer::SourceFactory::create(QStringLiteral("tcp://192.168.1.50"), config),
             "tcp:// without a port must be rejected");
    QVERIFY2(!GStreamer::SourceFactory::create(QStringLiteral("tcp://:5600"), config),
             "tcp:// without a host must be rejected");
    QVERIFY2(!GStreamer::SourceFactory::create(QStringLiteral("tcp://192.168.1.50:0"), config),
             "tcp:// with port 0 must be rejected");
}

void GStreamerTest::_testSourceFactoryUdp265Caps()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp265://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "udpsrc");
    QVERIFY2(src, "udp265:// must build a udpsrc");

    GstCaps* caps = nullptr;
    g_object_get(src, "caps", &caps, nullptr);
    QVERIFY2(caps, "udp265:// must set H265 RTP caps on udpsrc");
    const GstStructure* st = gst_caps_get_structure(caps, 0);
    const gchar* encName = gst_structure_get_string(st, "encoding-name");
    QCOMPARE(QString::fromUtf8(encName), QStringLiteral("H265"));
    gst_caps_unref(caps);

    QVERIFY2(findChildByFactoryName(bin, "rtpjitterbuffer"), "udp265 RTP path must insert a jitterbuffer");
    QVERIFY2(!findChildByFactoryName(bin, "tsdemux"), "udp265 is RTP, not MPEG-TS; no tsdemux");
}

void GStreamerTest::_testSourceFactoryUdp265UsesExplicitDepayAndParser()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtph265depay") ||
        !gst_element_factory_find("h265parse")) {
        QSKIP("udp265 RTP/H265 plugins unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp265://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(findChildByFactoryName(bin, "rtph265depay"), "udp265:// must depayload H265 RTP explicitly");

    GstElement* parser = findChildByFactoryName(bin, "h265parse");
    QVERIFY2(parser, "udp265:// must parse H265 explicitly");

    gint configInterval = 0;
    g_object_get(parser, "config-interval", &configInterval, nullptr);
    QCOMPARE(configInterval, -1);
}

void GStreamerTest::_testSourceFactoryUdpH264Caps()
{
    if (!gst_element_factory_find("udpsrc")) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "udpsrc");
    QVERIFY(src);

    GstCaps* caps = nullptr;
    g_object_get(src, "caps", &caps, nullptr);
    QVERIFY2(caps, "udp:// must set H264 RTP caps");
    const GstStructure* st = gst_caps_get_structure(caps, 0);
    const gchar* media = gst_structure_get_string(st, "media");
    const gchar* encName = gst_structure_get_string(st, "encoding-name");
    QCOMPARE(QString::fromUtf8(media), QStringLiteral("video"));
    QCOMPARE(QString::fromUtf8(encName), QStringLiteral("H264"));
    gst_caps_unref(caps);
}

void GStreamerTest::_testSourceFactoryUdpMpegTs()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("tsdemux")) {
        QSKIP("udpsrc/tsdemux plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "udpsrc");
    QVERIFY2(src, "mpegts:// must build a udpsrc");

    GstCaps* caps = nullptr;
    g_object_get(src, "caps", &caps, nullptr);
    if (caps) {
        gst_caps_unref(caps);
    }
    QVERIFY2(!caps, "raw MPEG-TS over UDP must not force RTP caps on udpsrc");

    QVERIFY2(findChildByFactoryName(bin, "tsdemux"), "mpegts:// must insert tsdemux");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactorySchemeCaseInsensitive()
{
    if (!gst_element_factory_find("udpsrc")) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("UDP://127.0.0.1:5600"), config);
    QVERIFY2(bin, "uppercase scheme must be accepted (scheme is lower-cased before matching)");
    gst_object_unref(bin);
}

void GStreamerTest::_testSourceFactoryNegativeLatencyClamped()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.latencyMs = -100;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* jb = findChildByFactoryName(bin, "rtpjitterbuffer");
    QVERIFY(jb);
    guint latency = 99999;
    g_object_get(jb, "latency", &latency, nullptr);
    QCOMPARE(latency, 0u);
}

void GStreamerTest::_testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("tsdemux") ||
        !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/tsdemux/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* tsdemux = findChildByFactoryName(bin, "tsdemux");
    QVERIFY(tsdemux);
    GstElement* parser = findChildByFactoryName(bin, "parsebin");
    QVERIFY(parser);

    GstPad* parserSink = gst_element_get_static_pad(parser, "sink");
    QVERIFY(parserSink);
    const auto parserSinkCleanup = qScopeGuard([&] { gst_object_unref(parserSink); });
    QVERIFY(!gst_pad_is_linked(parserSink));

    static GstStaticPadTemplate sRtpPadTemplate = GST_STATIC_PAD_TEMPLATE(
        "stray_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
        GST_STATIC_CAPS("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264"));

    GstPad* strayPad = gst_pad_new_from_static_template(&sRtpPadTemplate, "stray_rtp");
    QVERIFY(strayPad);
    const auto padCleanup = qScopeGuard([&] { gst_object_unref(strayPad); });

    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gst_element_link_pads\\(\\) failed")));
    g_signal_emit_by_name(tsdemux, "pad-added", strayPad);

    QVERIFY2(!gst_pad_is_linked(parserSink),
             "a failed dynamic RTP pad link must leave parsebin available for the next valid pad");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "a failed dynamic RTP pad link must remove its temporary jitterbuffer");
}

#endif
