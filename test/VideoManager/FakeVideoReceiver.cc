#include "FakeVideoReceiver.h"

#include <QtCore/QTimer>
#include <QtGui/QImage>

#include "VideoFrameDelivery.h"

FakeVideoReceiver::FakeVideoReceiver(QObject* parent) : VideoReceiver(parent), _capabilities(CapStreaming | CapRecording) {}

FakeVideoReceiver::FakeVideoReceiver(bool gstreamer, QObject* parent)
    : VideoReceiver(parent), _capabilities(CapStreaming | CapRecording)
{
    // GStreamer fakes advertise lossless recording so recording policy tests
    // see the same capability shape as the real backend.
    if (gstreamer) {
        _capabilities |= CapRecordingLossless;
        _backendKind = BackendKind::GStreamer;
    } else {
        _backendKind = BackendKind::QtMultimedia;
    }
}

bool FakeVideoReceiver::_consumeFlag(bool& flag)
{
    if (flag) {
        flag = false;
        return true;
    }
    return false;
}

template <typename Emitter>
void FakeVideoReceiver::_emitMaybeAsync(Emitter&& emitter)
{
    if (_asyncDelayMs <= 0) {
        emitter();
        return;
    }
    QTimer::singleShot(_asyncDelayMs, this, std::forward<Emitter>(emitter));
}

void FakeVideoReceiver::start(uint32_t /*timeout*/)
{
    ++startCallCount;
    const STATUS s = _consumeFlag(failNextStart) ? STATUS_FAIL : STATUS_OK;
    _emitMaybeAsync([this, s] {
        if (s == STATUS_OK) {
            setStarted(true);
            emit receiverStarted();
        } else {
            // Real backends route start failures through the unified error
            // channel so the FSM can route to Reconnecting/Failed. The FSM no
            // longer consumes startCompleted(FAIL).
            emit receiverError(ErrorCategory::Fatal, QStringLiteral("fake: failNextStart"));
        }
    });
}

void FakeVideoReceiver::stop()
{
    ++stopCallCount;
    // Mirrors the idempotent-stop contract: if already stopped, emit STATUS_OK.
    // Also emit receiverStopped so the FSM (if in Stopping) can transition to
    // Idle even when the receiver never became "started".
    if (!started()) {
        emit receiverStopped();
        return;
    }

    const STATUS s = _consumeFlag(failNextStop) ? STATUS_FAIL : STATUS_OK;
    _emitMaybeAsync([this, s] {
        setStarted(false);
        forceDecoding(false);
        forceStreaming(false);
        if (s == STATUS_OK)
            emit receiverStopped();
        else
            emit receiverError(ErrorCategory::Fatal, QStringLiteral("fake: failNextStop"));
    });
}

void FakeVideoReceiver::pause()
{
    emit receiverPaused();
}

void FakeVideoReceiver::resume()
{
    emit receiverResumed();
}

void FakeVideoReceiver::startDecoding()
{
    ++startDecodingCallCount;
    const STATUS s = _consumeFlag(failNextStartDecoding) ? STATUS_FAIL : STATUS_OK;
    _emitMaybeAsync([this, s] {
        if (s == STATUS_OK)
            forceDecoding(true);
        else
            emit receiverError(ErrorCategory::Fatal, QStringLiteral("fake: failNextStartDecoding"));
    });
}

void FakeVideoReceiver::stopDecoding()
{
    ++stopDecodingCallCount;
    const STATUS s = _consumeFlag(failNextStopDecoding) ? STATUS_FAIL : STATUS_OK;
    _emitMaybeAsync([this, s] {
        if (s == STATUS_OK)
            forceDecoding(false);
        else
            emit receiverError(ErrorCategory::Fatal, QStringLiteral("fake: failNextStopDecoding"));
    });
}

void FakeVideoReceiver::emitReceiverError(ErrorCategory category, const QString& message)
{
    emit receiverError(category, message);
}

// ─── Headless frame delivery ────────────────────────────────────────────────

QVideoFrame FakeVideoReceiver::makeSyntheticFrame(QSize size, QVideoFrameFormat::PixelFormat fmt, QColor fill)
{
    // QImage::Format must match the QVideoFrameFormat::PixelFormat; keep it
    // simple — the default RGBA8888 maps cleanly to QImage::Format_RGBA8888.
    // Callers asking for a different pixel format are responsible for ensuring
    // the mapping matches; for CI we only need one viable sample.
    QImage img(size, QImage::Format_RGBA8888);
    img.fill(fill);
    QVideoFrameFormat format(size, fmt);
    QVideoFrame frame(format);
    // QVideoFrame constructed from QImage is the zero-friction path; we still
    // build the format explicitly so callers that introspect frame.surfaceFormat()
    // get the expected pixel-format value rather than whatever QVideoFrame
    // guesses from the QImage.
    QVideoFrame fromImage(img);
    return fromImage.isValid() ? fromImage : frame;
}

void FakeVideoReceiver::announceFormat(QSize size, QVideoFrameFormat::PixelFormat fmt)
{
    VideoFrameDelivery* delivery = frameDelivery();
    if (!delivery)
        return;
    delivery->announceFormat(QVideoFrameFormat(size, fmt));
}

bool FakeVideoReceiver::deliverSyntheticFrame(QSize size)
{
    VideoFrameDelivery* delivery = frameDelivery();
    if (!delivery)
        return false;
    QVideoFrame frame = makeSyntheticFrame(size);
    if (!frame.isValid())
        return false;
    delivery->deliverFrame(std::move(frame));
    return true;
}

int FakeVideoReceiver::deliverSyntheticFrames(int count, QSize size)
{
    VideoFrameDelivery* delivery = frameDelivery();
    if (!delivery || count <= 0)
        return 0;
    int delivered = 0;
    for (int i = 0; i < count; ++i) {
        QVideoFrame frame = makeSyntheticFrame(size);
        if (!frame.isValid())
            break;
        delivery->deliverFrame(std::move(frame));
        ++delivered;
    }
    return delivered;
}
