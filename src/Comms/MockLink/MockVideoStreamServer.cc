#include <gst/gst.h>

#ifdef QGC_GST_RTSP_SERVER
#include <glib.h>
// gdbusintrospection.h, which rtsp-server.h pulls in, has a `signals` member that collides with Qt's `signals`
// keyword macro (the app target's precompiled header defines it first).
#pragma push_macro("signals")
#undef signals
#include <gst/rtsp-server/rtsp-server.h>
#pragma pop_macro("signals")
#endif

#include "MockVideoStreamServer.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MockVideoStreamServerLog, "VideoManager.MockVideoStreamServer")

namespace {

// Shared test pattern source. videotestsrc's "ball" pattern makes motion obvious so a
// stalled pipeline is easy to spot by eye.
constexpr const char* kTestSource =
    "videotestsrc is-live=true pattern=ball ! "
    "video/x-raw,width=1280,height=720,framerate=30/1 ! "
    "videoconvert";

// Receivers join mid-stream and wait for the next keyframe, so the encoders key every second. Their fastest settings
// keep a 720p encode real-time on a loaded CI runner; x265's medium preset and 250-frame default GOP do not.
// H.264 prefers openh264enc, which is BSD-licensed and in the plugin allowlist, so every platform ships it; x264enc and
// x265enc are GPL, so only a desktop GStreamer that installs them provides them, and mobile builds never bundle them.
constexpr const char* kOpenH264Encoder = "openh264enc complexity=low rate-control=bitrate bitrate=2000000 gop-size=30";
constexpr const char* kX264Encoder = "x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=30";
constexpr const char* kH265Encoder = "x265enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=30";

bool factoryExists(const char* name)
{
    GstElementFactory* factory = gst_element_factory_find(name);
    if (factory) {
        gst_object_unref(factory);
        return true;
    }
    return false;
}

// The H.264 encoder description for gst_parse_launch, or nullptr when no H.264 encoder is installed.
const char* h264Encoder()
{
    if (factoryExists("openh264enc")) {
        return kOpenH264Encoder;
    }
    if (factoryExists("x264enc")) {
        return kX264Encoder;
    }
    return nullptr;
}

// GStreamer init is owned by QGC's central VideoBackend/GStreamer::initialize() path
// (environment prep, static plugin registration, logging). Never call gst_init() here —
// running it first would bypass that setup and turn the central init into a no-op.
bool gstInitialized()
{
    if (!gst_is_initialized()) {
        qCWarning(MockVideoStreamServerLog)
            << "GStreamer not initialized; QGC's central GStreamer init must run before serving a mock stream";
        return false;
    }
    return true;
}

}  // namespace

MockVideoStreamServer::~MockVideoStreamServer()
{
    stop();
}

bool MockVideoStreamServer::isStreamTypeAvailable(StreamType type)
{
    if (!gstInitialized()) {
        return false;
    }

    switch (type) {
        case StreamType::RtpUdpH264:
            return (h264Encoder() != nullptr) && factoryExists("videotestsrc");
        case StreamType::MpegTsUdp:
        case StreamType::MpegTsTcp:
            // mpegtsmux ships in gst-plugins-bad and can be absent even when the encoder is present.
            return (h264Encoder() != nullptr) && factoryExists("videotestsrc") && factoryExists("mpegtsmux");
        case StreamType::RtpUdpH265:
            return factoryExists("x265enc") && factoryExists("videotestsrc");
        case StreamType::RtspH264:
#ifdef QGC_GST_RTSP_SERVER
            return (h264Encoder() != nullptr) && factoryExists("videotestsrc");
#else
            return false;
#endif
    }

    return false;
}

bool MockVideoStreamServer::start(StreamType type, const QString& host, quint16 port)
{
    if (_running) {
        stop();
    }

    if (!isStreamTypeAvailable(type)) {
        qCWarning(MockVideoStreamServerLog) << "Requested stream type is not available in this GStreamer install";
        return false;
    }

    if (type == StreamType::RtspH264) {
#ifdef QGC_GST_RTSP_SERVER
        const int boundPort = _startRtsp(host, port);
        if (boundPort <= 0) {
            return false;
        }
        _servedUri = QStringLiteral("rtsp://%1:%2/test").arg(host, QString::number(boundPort));
        _running = true;
        qCDebug(MockVideoStreamServerLog) << "Serving" << _servedUri;
        return true;
#else
        qCWarning(MockVideoStreamServerLog) << "RTSP requested but gst-rtsp-server is not available";
        return false;
#endif
    }

    QString launch;
    QString uri;
    const QString portString = QString::number(port);
    switch (type) {
        case StreamType::RtpUdpH264:
            launch = QStringLiteral("%1 ! %2 ! rtph264pay config-interval=1 pt=96 ! udpsink host=%3 port=%4")
                         .arg(kTestSource, h264Encoder(), host, portString);
            uri = QStringLiteral("udp://%1:%2").arg(host, portString);
            break;
        case StreamType::RtpUdpH265:
            launch = QStringLiteral("%1 ! %2 ! rtph265pay config-interval=1 pt=96 ! udpsink host=%3 port=%4")
                         .arg(kTestSource, kH265Encoder, host, portString);
            uri = QStringLiteral("udp265://%1:%2").arg(host, portString);
            break;
        case StreamType::MpegTsUdp:
            launch = QStringLiteral("%1 ! %2 ! mpegtsmux ! udpsink host=%3 port=%4")
                         .arg(kTestSource, h264Encoder(), host, portString);
            uri = QStringLiteral("mpegts://%1:%2").arg(host, portString);
            break;
        case StreamType::MpegTsTcp:
            launch = QStringLiteral("%1 ! %2 ! mpegtsmux ! tcpserversink name=server host=%3 port=%4")
                         .arg(kTestSource, h264Encoder(), host, portString);
            uri = QStringLiteral("tcp://%1:%2").arg(host, portString);
            break;
        case StreamType::RtspH264:
            return false;  // handled above
    }

    if (!_startPipeline(launch)) {
        return false;
    }

    if ((type == StreamType::MpegTsTcp) && (port == 0)) {
        GstElement* const server = gst_bin_get_by_name(GST_BIN(_pipeline), "server");
        gint boundPort = 0;
        g_object_get(server, "current-port", &boundPort, nullptr);
        gst_object_unref(server);
        uri = QStringLiteral("tcp://%1:%2").arg(host, QString::number(boundPort));
    }

    _servedUri = uri;
    _running = true;
    qCDebug(MockVideoStreamServerLog) << "Serving" << _servedUri << "via" << launch;
    return true;
}

bool MockVideoStreamServer::_startPipeline(const QString& launch)
{
    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(launch.toUtf8().constData(), &error);
    if (error) {
        qCWarning(MockVideoStreamServerLog) << "Failed to build pipeline:" << error->message;
        g_error_free(error);
        if (pipeline) {
            gst_object_unref(pipeline);
        }
        return false;
    }

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qCWarning(MockVideoStreamServerLog) << "Failed to set pipeline to PLAYING";
        gst_object_unref(pipeline);
        return false;
    }

    _pipeline = pipeline;
    return true;
}

void MockVideoStreamServer::stop()
{
    if (_pipeline) {
        gst_element_set_state(_pipeline, GST_STATE_NULL);
        gst_object_unref(_pipeline);
        _pipeline = nullptr;
    }

#ifdef QGC_GST_RTSP_SERVER
    _stopRtsp();
#endif

    _running = false;
    _servedUri.clear();
}

#ifdef QGC_GST_RTSP_SERVER
int MockVideoStreamServer::_startRtsp(const QString& host, quint16 port)
{
    _rtspContext = g_main_context_new();

    GstRTSPServer* server = gst_rtsp_server_new();
    const QByteArray address = host.toUtf8();
    const QByteArray service = QByteArray::number(port);
    gst_rtsp_server_set_address(server, address.constData());
    gst_rtsp_server_set_service(server, service.constData());

    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    const QString launch = QStringLiteral("( %1 ! %2 ! rtph264pay name=pay0 pt=96 )").arg(kTestSource, h264Encoder());
    gst_rtsp_media_factory_set_launch(factory, launch.toUtf8().constData());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    if (gst_rtsp_server_attach(server, _rtspContext) == 0) {
        qCWarning(MockVideoStreamServerLog) << "Failed to attach RTSP server on port" << port;
        gst_object_unref(server);
        g_main_context_unref(_rtspContext);
        _rtspContext = nullptr;
        return 0;
    }
    const int boundPort = gst_rtsp_server_get_bound_port(server);

    _rtspServer = server;
    _rtspLoop = g_main_loop_new(_rtspContext, FALSE);

    _rtspThread = std::thread([this]() {
        g_main_context_push_thread_default(_rtspContext);
        g_main_loop_run(_rtspLoop);
        g_main_context_pop_thread_default(_rtspContext);
    });

    return boundPort;
}

void MockVideoStreamServer::_stopRtsp()
{
    if (_rtspLoop) {
        // Quit from inside the loop's context: g_main_loop_run() sets the running flag on entry, so a quit
        // issued before the thread enters it would be lost and join() would hang.
        g_main_context_invoke(
            _rtspContext,
            [](gpointer loop) -> gboolean {
                g_main_loop_quit(static_cast<GMainLoop*>(loop));
                return G_SOURCE_REMOVE;
            },
            _rtspLoop);
    }
    if (_rtspThread.joinable()) {
        _rtspThread.join();
    }
    if (_rtspLoop) {
        g_main_loop_unref(_rtspLoop);
        _rtspLoop = nullptr;
    }
    if (_rtspServer) {
        gst_object_unref(_rtspServer);
        _rtspServer = nullptr;
    }
    if (_rtspContext) {
        g_main_context_unref(_rtspContext);
        _rtspContext = nullptr;
    }
}
#endif  // QGC_GST_RTSP_SERVER
