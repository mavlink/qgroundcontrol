#include "UVCFrameRelay.h"

#include <memory>
#include <utility>

#include <QtCore/QThread>
#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

namespace {

/// Maps another frame's buffer through that frame, which counts the mappings of both.
QAbstractVideoBuffer::MapData mapThrough(QVideoFrame& frame, QVideoFrame::MapMode mode)
{
    QAbstractVideoBuffer::MapData data;
    if (!frame.map(mode)) {
        return data;
    }
    data.planeCount = frame.planeCount();
    for (int plane = 0; plane < data.planeCount; plane++) {
        data.bytesPerLine[plane] = frame.bytesPerLine(plane);
        data.data[plane] = frame.bits(plane);
        data.dataSize[plane] = frame.mappedBytes(plane);
    }
    return data;
}

/// The buffer of a frame in memory, shared with another frame.
class SharedVideoBuffer final : public QAbstractVideoBuffer
{
public:
    explicit SharedVideoBuffer(QVideoFrame frame)
        : _frame(std::move(frame))
    {}

    MapData map(QVideoFrame::MapMode mode) override { return mapThrough(_frame, mode); }

    void unmap() override { _frame.unmap(); }

    QVideoFrameFormat format() const override { return _frame.surfaceFormat(); }

private:
    QVideoFrame _frame;
};

/// The buffer of a frame with GPU textures, shared with another frame: its textures are the other frame's.
class SharedHwVideoBuffer final : public QHwVideoBuffer
{
public:
    explicit SharedHwVideoBuffer(QVideoFrame frame)
        : QHwVideoBuffer(frame.handleType())
        , _frame(std::move(frame))
        , _source(QVideoFramePrivate::hwBuffer(_frame))
    {}

    MapData map(QVideoFrame::MapMode mode) override { return mapThrough(_frame, mode); }

    void unmap() override { _frame.unmap(); }

    QVideoFrameFormat format() const override { return _frame.surfaceFormat(); }

    QRhi* associatedCurrentThreadRhi() const override { return _source->associatedCurrentThreadRhi(); }

    QMatrix4x4 externalTextureMatrix() const override { return _source->externalTextureMatrix(); }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override
    {
        return _source->mapTextures(rhi, oldTextures);
    }

    void initTextureConverter(QRhi& rhi) override { _source->initTextureConverter(rhi); }

    bool isDmaBuf() const override { return _source->isDmaBuf(); }

    quint64 textureHandle(QRhi& rhi, int plane) override { return _source->textureHandle(rhi, plane); }

private:
    QVideoFrame _frame;
    QHwVideoBuffer* const _source;
};

}  // namespace

UVCFrameRelay::UVCFrameRelay(QObject* parent)
    : QObject(parent)
{
    (void) connect(&_input, &QVideoSink::videoFrameChanged, this, &UVCFrameRelay::_onFrame, Qt::DirectConnection);
}

UVCFrameRelay::~UVCFrameRelay() = default;

void UVCFrameRelay::setOutput(QVideoSink* output)
{
    _output = output;
}

void UVCFrameRelay::setTransform(QtVideo::Rotation rotation, bool mirrored)
{
    _rotation = rotation;
    _mirrored = mirrored;
}

QVideoFrame UVCFrameRelay::transformed(const QVideoFrame& frame, QtVideo::Rotation rotation, bool mirrored)
{
    if (!frame.isValid()) {
        return frame;
    }
    QVideoFrame result =
        QVideoFramePrivate::hwBuffer(frame)
            ? QVideoFramePrivate::createFrame(std::make_unique<SharedHwVideoBuffer>(frame), frame.surfaceFormat())
            : QVideoFramePrivate::createFrame(std::make_unique<SharedVideoBuffer>(frame), frame.surfaceFormat());
    result.setStartTime(frame.startTime());
    result.setEndTime(frame.endTime());
    // QtVideo::Rotation's values are degrees.
    result.setRotation(
        static_cast<QtVideo::Rotation>((static_cast<int>(frame.rotation()) + static_cast<int>(rotation)) % 360));
    result.setMirrored(frame.mirrored() != mirrored);
    return result;
}

void UVCFrameRelay::_onFrame(const QVideoFrame& frame)
{
    {
        const QMutexLocker lock(&_pendingMutex);
        _pending = frame;
        if (_deliveryQueued) {
            return;
        }
        _deliveryQueued = true;
    }
    if (QThread::currentThread() == thread()) {
        _deliver();
    } else {
        QMetaObject::invokeMethod(this, &UVCFrameRelay::_deliver, Qt::QueuedConnection);
    }
}

void UVCFrameRelay::_deliver()
{
    QVideoFrame frame;
    {
        const QMutexLocker lock(&_pendingMutex);
        frame = std::exchange(_pending, QVideoFrame());
        _deliveryQueued = false;
    }
    if (_output) {
        _output->setVideoFrame(transformed(frame, _rotation, _mirrored));
    }
}
