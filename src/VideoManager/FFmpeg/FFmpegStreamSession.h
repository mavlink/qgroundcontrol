#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <thread>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtTypes>
#include <QtMultimedia/QVideoFrame>

#include "FFmpegPointers.h"
#include "FFmpegStreamRecorder.h"
#include "KeyframeIntervalMeter.h"
#include "VideoStreamUri.h"

struct AVStream;
class StreamWatchdog;

/// One connection of an FFmpegVideoReceiver to its stream, on a thread of its own: opens the stream with libavformat,
/// reads its video packets, and while the receiver displays decodes them with libavcodec, a hardware decoder first, and
/// hands over each frame as soon as it is decoded (a MediaCodec decoder's when it is ready, see _awaitFrame()); every
/// video packet also goes to the receiver's recorder, which copies it while it records this session. Nothing waits for
/// a clock and nothing is queued: FFmpeg's input buffering is off, its RTP reordering waits the config's jitter latency
/// (none in low-latency mode), and the decoder gives out each frame as it can (slice threads; frame threads only when
/// the config asks for more than one).
///
/// Threading: the receiver constructs, starts, steers (setDecoding(), requestStop()) and destroys it on its own thread.
/// The session's thread runs everything else and calls the Listener. A stop request interrupts a blocking open or read
/// at once: FFmpeg asks the interrupt callback at least every 100 ms while it waits on the network. The destructor
/// requests a stop and waits for the thread.
class FFmpegStreamSession
{
public:
    struct Settings
    {
        VideoStreamUri::FFmpegSource source;
        /// The stream's URI without credentials, for logs.
        QString redactedUri;
        /// How long FFmpeg waits on a server that doesn't answer (RTSP, TCP). A UDP port waits for its sender.
        std::chrono::milliseconds networkTimeout{3000};
        bool lowLatency = false;
        int rtpJitterLatencyMs = 80;
        int softwareDecoderThreads = 1;
        /// A hardware decoder is tried first: VA-API, then NVDEC; MediaCodec on Android.
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
        /// The libavcodec decoder, with the hardware it decodes on unless it is that hardware's own decoder:
        /// "h264 (VA-API)", "h264_mediacodec".
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
        /// KeyframeIntervalMeter's, of the video packets' timestamps; -1 before the second keyframe.
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

    /// @p id names the session in the listener's calls and to @p recorder. Each video packet is a source frame of
    /// @p watchdog, noted on the session's thread.
    FFmpegStreamSession(quint64 id, const Settings& settings, Listener& listener, FFmpegStreamRecorder& recorder,
                        StreamWatchdog& watchdog);
    ~FFmpegStreamSession();

    FFmpegStreamSession(const FFmpegStreamSession&) = delete;
    FFmpegStreamSession& operator=(const FFmpegStreamSession&) = delete;

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
    void _decode(const AVStream& stream, AVPacket& packet);
    /// Opens _decoder for @p stream, in hardware where it can; the message why not otherwise. @p keyframe: the packet
    /// decoding starts at.
    std::optional<QString> _openDecoder(const AVStream& stream, const AVPacket& keyframe);
    /// Hands over the frames the decoder has ready; how many.
    int _deliverFrames();
    /// For a decoder that gives a packet's frame some milliseconds after it took the packet (MediaCodec): hands over
    /// what is ready, or waits for that frame rather than hand it over with the next packet, but never longer than
    /// half the time between packets, so that reading keeps up with the stream.
    void _awaitFrame();

    const quint64 _id;
    const Settings _settings;
    Listener& _listener;
    FFmpegStreamRecorder& _recorder;
    StreamWatchdog& _watchdog;
    std::thread _thread;
    std::atomic<bool> _stopRequested{false};
    std::atomic<bool> _decodingWanted{false};
    std::atomic<quint64> _packets{0};
    std::atomic<quint64> _bytes{0};
    std::atomic<quint64> _framesDecoded{0};
    std::atomic<bool> _hardwareFrames{false};
    /// Noted on the session's thread.
    KeyframeIntervalMeter _keyframes;

    // The session's thread only.
    /// The video stream, as the recorder takes it: created with its first packet.
    std::shared_ptr<const FFmpegStreamRecorder::Stream> _recordedStream;
    /// Shared with the frames of a MediaCodec decoder, which need it for as long as they live.
    std::shared_ptr<AVCodecContext> _decoder;
    /// FFmpegQtInternals::completeHardwareFrame()'s, of _decoder.
    FFmpegPointers::Buffer _hardwareFramesContext;
    /// _decoder gives its frames when they are ready, not as it takes their packets.
    bool _framesComeLater = false;
    std::chrono::steady_clock::time_point _lastSendTime;
    /// The time between the packets the decoder took, smoothed; zero before the second.
    std::chrono::microseconds _packetInterval{0};
    FFmpegPointers::Frame _decodedFrame;
    /// Decoding starts, and starts again after a pause, at a keyframe.
    bool _awaitingKeyframe = true;
    bool _decoderFailed = false;
};
