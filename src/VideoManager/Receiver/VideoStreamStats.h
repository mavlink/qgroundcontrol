#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

/// One video stream's health, as its VideoReceiver (UVCStreamMonitor for a USB camera) measured it last: about once a
/// second while it streams.
///
/// A field that doesn't apply to the stream is -1, which tells it apart from a measured zero: the RTP fields for a
/// stream without an RTP jitter buffer (MPEG-TS, or UDP RTP in low-latency mode), latencyMs while the pipeline
/// can't tell its latency, keyframeIntervalMs until two keyframes arrived and the recording fields while not recording.
/// Strings are empty until known.
struct VideoStreamStats
{
    Q_GADGET
    QML_VALUE_TYPE(videoStreamStats)
    Q_PROPERTY(bool valid MEMBER valid FINAL)
    Q_PROPERTY(double fps MEMBER fps FINAL)
    Q_PROPERTY(double bitrateKbps MEMBER bitrateKbps FINAL)
    Q_PROPERTY(qint64 framesDropped MEMBER framesDropped FINAL)
    Q_PROPERTY(bool hasRtpStats READ hasRtpStats FINAL)
    Q_PROPERTY(qint64 packetsLost MEMBER packetsLost FINAL)
    Q_PROPERTY(double lossPercent MEMBER lossPercent FINAL)
    Q_PROPERTY(double jitterMs MEMBER jitterMs FINAL)
    Q_PROPERTY(double latencyMs MEMBER latencyMs FINAL)
    Q_PROPERTY(QString decoder MEMBER decoder FINAL)
    Q_PROPERTY(bool hardwareDecoder MEMBER hardwareDecoder FINAL)
    Q_PROPERTY(QString gpuPath MEMBER gpuPath FINAL)
    Q_PROPERTY(QString codec MEMBER codec FINAL)
    Q_PROPERTY(double droppedFps MEMBER droppedFps FINAL)
    Q_PROPERTY(qint64 gpuFallbacks MEMBER gpuFallbacks FINAL)
    Q_PROPERTY(qint64 decoderRestarts MEMBER decoderRestarts FINAL)
    Q_PROPERTY(double keyframeIntervalMs MEMBER keyframeIntervalMs FINAL)
    Q_PROPERTY(qint64 packetsLate MEMBER packetsLate FINAL)
    Q_PROPERTY(qint64 packetsDuplicate MEMBER packetsDuplicate FINAL)
    Q_PROPERTY(qint64 rtxRequested MEMBER rtxRequested FINAL)
    Q_PROPERTY(qint64 rtxRecovered MEMBER rtxRecovered FINAL)
    Q_PROPERTY(qint64 uptimeMs MEMBER uptimeMs FINAL)
    Q_PROPERTY(int reconnects MEMBER reconnects FINAL)
    Q_PROPERTY(qint64 recordingMs MEMBER recordingMs FINAL)
    Q_PROPERTY(qint64 recordingBytes MEMBER recordingBytes FINAL)

public:
    bool hasRtpStats() const { return packetsLost >= 0; }

    bool operator==(const VideoStreamStats&) const = default;

    /// Measured at least once for the current stream; the other fields mean nothing while false.
    bool valid = false;
    /// Frames shown per second over the last few seconds: handed to the display and not replaced by a newer frame
    /// before it was shown.
    double fps = 0.0;
    /// Compressed video received over the last few seconds, in kbit/s; -1 for a source that can't count it (Qt
    /// Multimedia's player, a USB camera).
    double bitrateKbps = 0.0;
    /// Since the stream started: frames the display dropped to keep up with the stream.
    qint64 framesDropped = 0;
    /// RTP packets lost since the stream started.
    qint64 packetsLost = -1;
    /// Share of the RTP packets due in the last interval that were lost.
    double lossPercent = -1.0;
    /// RTP interarrival jitter, averaged by the jitter buffer.
    double jitterMs = -1.0;
    /// The pipeline's minimum latency, the playout delay it adds on top of the network's.
    double latencyMs = -1.0;
    /// The decoder's element factory, e.g. "avdec_h264".
    QString decoder;
    bool hardwareDecoder = false;
    /// How decoded frames reach the display: "CPU", or a zero-copy GPU path such as "DMABuf", "GL" or "D3D11". A USB
    /// camera's frames: "CPU" or "GPU".
    QString gpuPath;
    /// "H.264", "H.265", ..., or a USB camera's capture format, such as "MJPEG" or "YUYV".
    QString codec;
    /// Frames dropped per second in the last interval.
    double droppedFps = 0.0;
    /// Since the stream started: frames a zero-copy GPU path was tried for but the CPU delivered.
    qint64 gpuFallbacks = 0;
    /// Since the stream started: times the decoder was replaced after an error or a stall.
    qint64 decoderRestarts = 0;
    /// Between keyframes, from their timestamps, averaged over the last few.
    double keyframeIntervalMs = -1.0;
    /// Since the stream started: RTP packets that arrived after their playout time, and duplicate RTP packets.
    qint64 packetsLate = -1;
    qint64 packetsDuplicate = -1;
    /// Since the stream started: RTP retransmissions requested (RTSP), and how many arrived in time.
    qint64 rtxRequested = -1;
    qint64 rtxRecovered = -1;
    /// Since the stream last connected.
    qint64 uptimeMs = 0;
    /// Since the stream was started or restarted: reconnects after the stream failed.
    int reconnects = 0;
    /// While recording: for how long, and the bytes written to its files so far.
    qint64 recordingMs = -1;
    qint64 recordingBytes = -1;
};

Q_DECLARE_METATYPE(VideoStreamStats)
