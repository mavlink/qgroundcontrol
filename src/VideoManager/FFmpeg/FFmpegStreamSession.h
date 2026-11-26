#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <optional>
#include <thread>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtTypes>
#include <QtMultimedia/QVideoFrame>

#include "FFmpegPointers.h"
#include "FFmpegStreamRecorder.h"
#include "VideoStreamUri.h"

struct AVStream;

/// One connection of an FFmpegVideoReceiver to its stream, on a thread of its own: opens the stream with libavformat,
/// reads its video packets, and while the receiver displays decodes them with libavcodec, a hardware decoder first, and
/// hands over each frame as soon as it is decoded; every video packet also goes to the receiver's recorder, which
/// copies it while it records this session. Nothing waits for a clock and nothing is queued: FFmpeg's input
/// buffering is off, its RTP reordering waits the config's jitter latency (none in low-latency mode), and the decoder
/// gives out each frame as it can (slice threads; frame threads only when the config asks for more than one).
///
/// Threading: the receiver constructs, starts, steers (setDecoding(), requestStop()) and destroys it on its own thread.
/// The session's thread runs everything else and calls the Listener. A stop request interrupts a blocking open or read
/// at once: FFmpeg asks the interrupt callback at least every 100 ms while it waits on the network. The destructor
/// requests a stop and waits for the thread.
class FFmpegStreamSession
{
public:
    /// What FFmpeg opens for a stream URI.
    struct Source
    {
        /// What avformat_open_input() opens; for an SDP description, the name it is opened under.
        QString url;
        /// RTP over UDP: the SDP description FFmpeg reads in place of a URL.
        QByteArray sdp;
        /// nullopt for a URL that is none of VideoStreamResolver's, which FFmpeg opens as it is.
        std::optional<VideoStreamUri::Transport> transport;
        /// RTSP interleaved over its TCP connection, rather than FFmpeg's choice, which tries UDP first.
        bool rtspOverTcp = false;

        friend bool operator==(const Source&, const Source&) = default;
    };

    struct Settings
    {
        Source source;
        /// The stream's URI without credentials, for logs.
        QString redactedUri;
        /// How long FFmpeg waits on a server that doesn't answer (RTSP, TCP). A UDP port waits for its sender.
        std::chrono::milliseconds networkTimeout{3000};
        bool lowLatency = false;
        int rtpJitterLatencyMs = 80;
        int softwareDecoderThreads = 1;
        /// A hardware decoder is tried first: VA-API, then NVDEC.
        bool hardwareDecoding = true;
        /// Frames are presented at their pixel aspect ratio.
        bool squarePixels = true;
    };

    struct Failure
    {
        QString message;
        /// Another session can't fare better: FFmpeg has no protocol or demuxer for the URL.
        bool permanent = false;
    };

    struct DecoderInfo
    {
        /// The libavcodec decoder, with the hardware it decodes on: "h264 (VA-API)".
        QString name;
        bool hardware = false;
    };

    /// Totals since the session started.
    struct Counters
    {
        quint64 packets = 0;
        quint64 bytes = 0;
        quint64 framesDecoded = 0;
        /// The last frame decoded was in GPU memory.
        bool hardwareFrames = false;
        /// nowMs() at the last video packet; 0 before the first.
        qint64 lastPacketMs = 0;
        /// Between keyframes, from their timestamps, averaged over the last kKeyframeWindow; -1 before the second.
        double keyframeIntervalMs = -1.0;
    };

    /// Called on the session's thread.
    class Listener
    {
    public:
        virtual ~Listener() = default;
        /// The first video packet arrived, of a stream of @p codec: "H.264", "H.265", ...
        virtual void onSessionStreaming(quint64 session, const QString& codec) = 0;
        /// A decoder opened for the stream.
        virtual void onSessionDecoder(quint64 session, const DecoderInfo& decoder) = 0;
        /// A frame was decoded.
        virtual void onSessionFrame(quint64 session, QVideoFrame frame) = 0;
        /// The stream's video can't be decoded; the session goes on without a picture. Once per session.
        virtual void onSessionDecoderFailed(quint64 session, const QString& message) = 0;
        /// The last call, as the thread ends: after a stop request without a failure.
        virtual void onSessionEnded(quint64 session, const std::optional<Failure>& failure) = 0;
    };

    /// @p id names the session in the listener's calls and to @p recorder.
    FFmpegStreamSession(quint64 id, const Settings& settings, Listener& listener, FFmpegStreamRecorder& recorder);
    ~FFmpegStreamSession();

    FFmpegStreamSession(const FFmpegStreamSession&) = delete;
    FFmpegStreamSession& operator=(const FFmpegStreamSession&) = delete;

    /// What FFmpeg opens for @p uri, one of VideoStreamResolver's: rtsp(s):// and tcp:// as they are, rtspt:// as
    /// rtsp:// over TCP, mpegts:// as udp://, and udp:// (H.264) or udp265:// (H.265) RTP, payload type 96, as an SDP
    /// description. Other URLs as they are; nullopt for an invalid one, and for one of those whose address no receiver
    /// opens (VideoStreamUri::addressError()) or names port 0 to listen on.
    static std::optional<Source> sourceFor(const QString& uri);

    /// Monotonic milliseconds, the clock of Counters::lastPacketMs.
    static qint64 nowMs();

    /// Keyframes the keyframe interval averages over, as the GStreamer receiver's: a stream's timestamps can be
    /// arrival times, which a busy sender or network bunches up.
    static constexpr int kKeyframeWindow = 5;

    quint64 id() const { return _id; }

    const Settings& settings() const { return _settings; }

    void start();
    /// Has the thread end as soon as it can. Returns at once.
    void requestStop();
    /// Decodes and hands over frames while true. Off, the stream is still read. Any thread.
    void setDecoding(bool decoding);
    /// Any thread.
    Counters counters() const;

private:
    static int _interrupt(void* opaque);

    void _run();
    /// Receives until a stop request (nullopt) or a failure.
    std::optional<Failure> _receive();
    void _onVideoPacket(const AVStream& stream, AVPacket& packet);
    void _noteKeyframe(const AVStream& stream, const AVPacket& packet);
    void _decode(const AVStream& stream, AVPacket& packet);
    /// Opens _decoder for @p stream, in hardware where it can; the message why not otherwise.
    std::optional<QString> _openDecoder(const AVStream& stream);
    void _deliverFrames();

    const quint64 _id;
    const Settings _settings;
    Listener& _listener;
    FFmpegStreamRecorder& _recorder;
    std::thread _thread;
    std::atomic<bool> _stopRequested{false};
    std::atomic<bool> _decodingWanted{false};
    std::atomic<quint64> _packets{0};
    std::atomic<quint64> _bytes{0};
    std::atomic<quint64> _framesDecoded{0};
    std::atomic<bool> _hardwareFrames{false};
    std::atomic<qint64> _lastPacketMs{0};
    std::atomic<double> _keyframeIntervalMs{-1.0};

    // The session's thread only.
    /// The video stream, as the recorder takes it: created with its first packet.
    std::shared_ptr<const FFmpegStreamRecorder::Stream> _recordedStream;
    FFmpegPointers::CodecContext _decoder;
    FFmpegPointers::Frame _decodedFrame;
    /// Decoding starts, and starts again after a pause, at a keyframe.
    bool _awaitingKeyframe = true;
    bool _decoderFailed = false;
    /// The timestamps of the last keyframes, in ms, oldest first.
    std::deque<double> _keyframeTimesMs;
};
