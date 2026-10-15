#pragma once

#include <atomic>
#include <gst/gstbin.h>
#include <gst/gstpad.h>

#include <QtCore/QMutex>
#include <QtCore/QString>

class GstVideoReceiver;

/// \brief The display branch behind GstVideoReceiver's tee: leaky queue ! valve [! decodebin3 ! video sink].
///
/// Holds the caller's video sink until the source streams, then inserts decodebin3 and links its pad to the sink.
/// The caller keeps its own ref on the sink; the branch takes one more, and the pipeline its own while the sink is
/// inside. Decoding is reported from the first frame that reaches the sink. Also keeps the decoder statistics the
/// receiver exposes: the plugged decoder's name and the latest QoS values. Reports through the receiver's signals.
///
/// Threading: the receiver's worker, except the sink probe and the decoder-added callback (streaming threads) and the
/// onBus*() methods (whichever thread posts). The statistics getters are thread-safe.
class DecodingBranch
{
public:
    explicit DecodingBranch(GstVideoReceiver& receiver);

    /// Adds the branch's leaky queue and closed valve to @p pipeline behind @p tee.
    [[nodiscard]] bool build(GstElement* pipeline, GstElement* tee);
    /// Detaches the video sink and decoder and forgets the pipeline, which must be in NULL with its callbacks stopped.
    void reset();

    /// Takes @p videoSink and, once @p streaming, inserts the decoder. Reports onStartDecodingComplete.
    void start(GstElement* videoSink, bool hasWidget, bool streaming);
    /// Tears the branch down at once (there is nothing to flush). Reports onStopDecodingComplete.
    void stop();
    /// The source pad is linked: inserts the decoder when a video sink is waiting for it.
    void onSourceStreaming();
    void onDecoderPad(GstPad* pad);

    GstElement* decoder() const { return _decoder; }

    bool isDecoding() const { return _decoding.load(std::memory_order_relaxed); }

    /// The sink's negotiated raw caps ("qgc-caps-info").
    void onBusCapsInfo(GstMessage* message);
    void onBusQos(GstMessage* message);
    /// Emits decoderStatsChanged when QoS values arrived since the last call. Bounds the emit rate, since a sink can
    /// post QOS per dropped buffer.
    void publishQosStats();

    QString decoderName() const;

    quint64 processedFrames() const { return _processedFrames.load(std::memory_order_relaxed); }

    quint64 droppedFrames() const { return _droppedFrames.load(std::memory_order_relaxed); }

    qint64 currentJitterNs() const { return _currentJitterNs.load(std::memory_order_relaxed); }

    double qosProportion() const { return _qosProportion.load(std::memory_order_relaxed); }

    int qosQuality() const { return _qosQuality.load(std::memory_order_relaxed); }

private:
    /// Puts the video sink into the pipeline and decodebin3 between it and the valve, then opens the valve.
    bool _insertDecoder();
    bool _addDecoder();
    void _ensureVideoSinkInPipeline();
    bool _addVideoSink(GstPad* pad);
    void _onFirstFrame();
    void _notifyStatsChanged();
    void _shutdown();

    static void _onDecoderElementAdded(GstBin* decodebin, GstBin* subBin, GstElement* element, gpointer data);
    static GstPadProbeReturn _sinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    GstVideoReceiver& _receiver;
    GstElement* _pipeline = nullptr;
    GstElement* _valve = nullptr;
    GstElement* _decoder = nullptr;
    GstElement* _videoSink = nullptr;
    gulong _sinkProbeId = 0;
    std::atomic<bool> _decoding = false;       ///< Read by the receiver's tee probe log.

    mutable QMutex _decoderNameMutex;          // QString refcount isn't thread-safe across reader/writer threads
    QString _decoderName;
    std::atomic<quint64> _processedFrames{0};  // written on streaming thread (QOS), read on GUI
    std::atomic<quint64> _droppedFrames{0};
    std::atomic<qint64> _currentJitterNs{0};
    std::atomic<double> _qosProportion{1.0};
    std::atomic<int> _qosQuality{1000000};
    std::atomic<bool> _qosStatsDirty{false};  // set per QOS message, drained by publishQosStats()
};
