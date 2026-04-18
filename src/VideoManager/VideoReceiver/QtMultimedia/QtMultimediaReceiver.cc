#include "QtMultimediaReceiver.h"

#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QPlaybackOptions>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(QtMultimediaReceiverLog, "Video.QtMultimediaReceiver")

QtMultimediaReceiver::QtMultimediaReceiver(QObject* parent)
    : VideoReceiver(parent),
      _mediaPlayer(new QMediaPlayer(this)),
      _internalSink(new QVideoSink(this))
{
    _startTimeoutTimer.setSingleShot(true);

    // The internal sink receives decoded frames from QMediaPlayer.
    // Its videoFrameChanged is connected to VideoFrameDelivery::deliverFrame
    // in _rewireInternalSink() so frame delivery is symmetric with GStreamer.
    _mediaPlayer->setVideoSink(_internalSink);

    (void)connect(_mediaPlayer, &QMediaPlayer::playingChanged, this, [this](bool playing) { _setStreamingActive(playing); });
    (void)connect(_mediaPlayer, &QMediaPlayer::hasVideoChanged, this, [this](bool has) {
        if (has)
            _pullDecoderInfoFromMetadata();
        _setDecoderActive(has);
    });
    (void)connect(_mediaPlayer, &QMediaPlayer::bufferProgressChanged, this, [](float filled) {
        qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << "Buffer Progress:" << filled;
    });
    (void)connect(_mediaPlayer, &QMediaPlayer::metaDataChanged, this,
                  &QtMultimediaReceiver::_pullDecoderInfoFromMetadata);
    (void)connect(_mediaPlayer, &QMediaPlayer::errorOccurred, this,
                  [this](QMediaPlayer::Error error, const QString& errorString) {
                      qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << errorString;
                      const auto category = (error == QMediaPlayer::FormatError)
                                                ? VideoReceiver::ErrorCategory::MissingPlugin
                                                : VideoReceiver::ErrorCategory::Fatal;
                      emit receiverError(category, errorString);
                  });
}

void QtMultimediaReceiver::_setStreamingActive(bool active)
{
    if (_streamingActive == active)
        return;
    _streamingActive = active;
    emit streamingChanged(active);
}

void QtMultimediaReceiver::_setDecoderActive(bool active)
{
    if (_decoderActive == active)
        return;
    _decoderActive = active;
    emit decodingChanged(active);
}

QtMultimediaReceiver::~QtMultimediaReceiver()
{
    qCDebug(QtMultimediaReceiverLog) << this;
}

void QtMultimediaReceiver::_pullDecoderInfoFromMetadata()
{
    const QMediaMetaData md = _mediaPlayer->metaData();
    const QVariant codec = md.value(QMediaMetaData::VideoCodec);
    if (!codec.isValid())
        return;

    QString codecName;
    if (codec.canConvert<QMediaFormat::VideoCodec>()) {
        codecName = QMediaFormat::videoCodecName(codec.value<QMediaFormat::VideoCodec>());
    } else {
        codecName = codec.toString();
    }
    if (codecName.isEmpty() || codecName == activeDecoderName())
        return;

    // Qt's FFmpeg backend does not expose a dedicated "decoder name" metadata
    // key, so the only signal we get is whatever the backend embeds in the
    // existing string values. Scan every string-like metadata entry for known
    // hardware-decoder tokens. This is a heuristic — it will miss custom
    // plugins and can be fooled by a codec string that happens to contain a
    // matching substring — but it is strictly better than the previous
    // unconditional `hwDecoding = false`, which made the stats overlay report
    // "(SW)" even when VAAPI / VideoToolbox / D3D11 were active.
    static const QStringList kHwTokens = {
        QStringLiteral("vaapi"),        // Linux VA-API
        QStringLiteral("nvdec"),        // NVIDIA NVDEC
        QStringLiteral("cuda"),         // NVIDIA CUVID
        QStringLiteral("d3d11"),        // Windows D3D11VA
        QStringLiteral("dxva"),         // Windows DXVA2
        QStringLiteral("videotoolbox"), // macOS / iOS
        QStringLiteral("mediacodec"),   // Android MediaCodec
        QStringLiteral("qsv"),          // Intel QuickSync
        QStringLiteral("v4l2m2m"),      // Linux V4L2 mem-to-mem
    };

    bool hwDetected = false;
    for (const auto key : md.keys()) {
        const QVariant v = md.value(key);
        if (v.typeId() != QMetaType::QString)
            continue;
        const QString s = v.toString().toLower();
        for (const QString& tok : kHwTokens) {
            if (s.contains(tok)) {
                hwDetected = true;
                break;
            }
        }
        if (hwDetected)
            break;
    }

    setDecoderInfo(hwDetected, codecName);
    qCDebug(QtMultimediaReceiverLog) << "Decoder info: codec =" << codecName << "hw =" << hwDetected;
}

VideoReceiver* QtMultimediaReceiver::createVideoReceiver(QObject* parent)
{
    return new QtMultimediaReceiver(parent);
}

void QtMultimediaReceiver::_rewireInternalSink()
{
    // Disconnect previous frame forwarding connection.
    disconnect(_internalFrameConn);

    if (!_delivery)
        return;

    // Forward every decoded frame from QMediaPlayer through frame delivery.
    // This gives us frame counts, frameArrived(), and latency measurement
    // symmetrically with the GStreamer appsink path.
    _internalFrameConn = connect(_internalSink, &QVideoSink::videoFrameChanged, this,
                                 [this](const QVideoFrame& frame) {
                                     if (frame.isValid() && _delivery)
                                         _delivery->deliverFrame(frame);
                                 });
}

void QtMultimediaReceiver::onSinkAboutToChange()
{
    disconnect(_internalFrameConn);
}

void QtMultimediaReceiver::onSinkChanged(QVideoSink* /*newSink*/)
{
    // Re-wire the internal sink → bridge forwarding whenever the bridge sink changes.
    _rewireInternalSink();
}

void QtMultimediaReceiver::_clearStartHandlers()
{
    disconnect(_startPlayingConn);
    disconnect(_startBufferConn);
    disconnect(_startTimeoutConn);
    _startTimeoutTimer.stop();
}

void QtMultimediaReceiver::start(uint32_t timeout)
{
    if (_mediaPlayer->isPlaying()) {
        qCDebug(QtMultimediaReceiverLog) << "Already running!";
        return;
    }

    if (_uri.isEmpty()) {
        qCDebug(QtMultimediaReceiverLog) << "Failed because URI is not specified";
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("Empty URI"));
        return;
    }

    _clearStartHandlers();

    const std::chrono::milliseconds timeoutMs(static_cast<qint64>(timeout) * 1000);

    if (_lowLatency) {
        QPlaybackOptions opts;
        opts.setPlaybackIntent(QPlaybackOptions::PlaybackIntent::LowLatencyStreaming);
        opts.setProbeSize(32768);
        opts.setNetworkTimeout(timeoutMs);
        _mediaPlayer->setPlaybackOptions(opts);
        qCDebug(QtMultimediaReceiverLog) << "Low-latency streaming enabled";
    } else {
        _mediaPlayer->resetPlaybackOptions();
    }

    _mediaPlayer->setSource(QUrl::fromUserInput(_uri));
    // Stash the per-frame watchdog interval for startDecoding() to arm the
    // bridge with once decoding actually begins.
    _watchdogInterval = timeoutMs;

    _startPlayingConn = connect(_mediaPlayer, &QMediaPlayer::playingChanged, this, [this](bool playing) {
        if (!playing)
            return;
        qCDebug(QtMultimediaReceiverLog) << "playingChanged(true) — receiver started";
        _clearStartHandlers();
        emit receiverStarted();
    });

    _startTimeoutTimer.setInterval(timeoutMs);
    _startTimeoutConn = connect(&_startTimeoutTimer, &QTimer::timeout, this, [this]() {
        qCWarning(QtMultimediaReceiverLog) << "start() timed out";
        _clearStartHandlers();
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("QMediaPlayer start timed out"));
    });

    _mediaPlayer->play();
    _startTimeoutTimer.start();

    qCDebug(QtMultimediaReceiverLog) << "Starting (async receiverStarted pending)";
}

void QtMultimediaReceiver::stop()
{
    _clearStartHandlers();

    if (_mediaPlayer->playbackState() == QMediaPlayer::StoppedState) {
        qCDebug(QtMultimediaReceiverLog) << "Already stopped — no-op";
        return;
    }

    if (_mediaPlayer->source().isEmpty()) {
        qCWarning(QtMultimediaReceiverLog) << "Stop called on empty URI";
        emit receiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("Stop called on empty URI"));
        return;
    }

    _mediaPlayer->stop();
    qCDebug(QtMultimediaReceiverLog) << "Stopped";
    emit receiverStopped();
}

void QtMultimediaReceiver::startDecoding()
{
    if (!validateBridgeForDecoding())
        return;

    // Wire frame forwarding from the internal sink through frame delivery.
    _rewireInternalSink();

    // Frame watchdog lives on delivery (centralized across backends). It
    // observes deliverFrame() so no sink-side connection is needed here.
    if (_watchdogInterval.count() > 0)
        _delivery->armWatchdog(_watchdogInterval);

    // The internal sink already receives frames from QMediaPlayer. The external
    // (QML-registered) sink is set on delivery; delivery forwards frames there
    // via deliverFrame(). QMediaPlayer does NOT write to the external sink directly.
    // This preserves the decoupling: QML VideoOutput gets frames via QVideoSink
    // registered on the bridge, not wired into QMediaPlayer directly.

    _setDecoderActive(true);
    qCDebug(QtMultimediaReceiverLog) << "Decoding";
}

void QtMultimediaReceiver::stopDecoding()
{
    // Parity with GstVideoReceiver/UVCReceiver: reject only when we're not
    // actually decoding. Proceed even when the sink has been detached so the
    // decode state tears down cleanly — otherwise a sink-detach + reconnect
    // leaves _decoderActive stuck true and _internalFrameConn dangling.
    if (!_decoderActive) {
        qCWarning(QtMultimediaReceiverLog) << "Not decoding — ignoring stopDecoding";
        return;
    }

    if (_delivery) {
        _delivery->disarmWatchdog();
    }
    disconnect(_internalFrameConn);

    _setDecoderActive(false);
    qCDebug(QtMultimediaReceiverLog) << "Stopped Decoding";
}

void QtMultimediaReceiver::pause()
{
    if (_mediaPlayer && _mediaPlayer->isPlaying()) {
        qCDebug(QtMultimediaReceiverLog) << "Pausing QMediaPlayer for reconnect";
        _mediaPlayer->pause();
        emit receiverPaused();
    }
}

void QtMultimediaReceiver::resume()
{
    if (_mediaPlayer && _mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        qCDebug(QtMultimediaReceiverLog) << "Resuming QMediaPlayer";
        _mediaPlayer->play();
        emit receiverResumed();
    }
}
