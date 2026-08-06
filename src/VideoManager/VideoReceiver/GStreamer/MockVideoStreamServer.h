#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

typedef struct _GstElement GstElement;

#ifdef QGC_GST_RTSP_SERVER
#include <thread>
typedef struct _GstRTSPServer GstRTSPServer;
typedef struct _GMainLoop GMainLoop;
typedef struct _GMainContext GMainContext;
#endif

Q_DECLARE_LOGGING_CATEGORY(MockVideoStreamServerLog)

/// \brief Serves a synthetic (videotestsrc) live video stream for MockLink so the
/// GStreamer receive pipeline can be exercised end-to-end without real hardware.
///
/// One instance serves exactly one stream. The served URI (available via servedUri()
/// after a successful start()) is what MockLink advertises via VIDEO_STREAM_INFORMATION,
/// so QGC auto-configures its receiver to the matching transport/codec.
class MockVideoStreamServer
{
public:
    /// Transport + codec of the served stream. Kept independent of MockConfiguration
    /// so the GStreamer layer carries no dependency on the Comms/MockLink module.
    enum class StreamType {
        RtpUdpH264, ///< RTP/H.264 over UDP   -> udp://
        RtpUdpH265, ///< RTP/H.265 over UDP   -> udp265://
        RtspH264,   ///< RTSP (H.264)         -> rtsp://
        MpegTsUdp,  ///< MPEG-TS over UDP     -> mpegts://
        MpegTsTcp,  ///< MPEG-TS over TCP     -> tcp://
    };

    MockVideoStreamServer() = default;
    ~MockVideoStreamServer();

    MockVideoStreamServer(const MockVideoStreamServer &) = delete;
    MockVideoStreamServer &operator=(const MockVideoStreamServer &) = delete;

    /// Start serving a test pattern of the given type on host:port.
    /// @return true on success; on success servedUri() holds the URI QGC should connect to.
    bool start(StreamType type, const QString &host, quint16 port);

    /// Stop the stream and release all resources. Safe to call when not running.
    void stop();

    bool isRunning() const { return _running; }
    QString servedUri() const { return _servedUri; }

    /// True when the encoder/muxer elements required for @p type are present in the
    /// local GStreamer install (e.g. false for RtpUdpH265 when x265enc is missing,
    /// or RtspH264 when gst-rtsp-server was not built in).
    static bool isStreamTypeAvailable(StreamType type);

private:
    bool _startPipeline(const QString &launch);
#ifdef QGC_GST_RTSP_SERVER
    bool _startRtsp(const QString &host, quint16 port);
    void _stopRtsp();

    GstRTSPServer *_rtspServer = nullptr;
    GMainLoop *_rtspLoop = nullptr;
    GMainContext *_rtspContext = nullptr;
    std::thread _rtspThread;
#endif

    GstElement *_pipeline = nullptr;
    bool _running = false;
    QString _servedUri;
};
