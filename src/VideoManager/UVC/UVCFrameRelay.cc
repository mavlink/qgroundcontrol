#include "UVCFrameRelay.h"

#include <utility>

#include <QtCore/QThread>

#include "SharedVideoFrame.h"

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
    QVideoFrame result = SharedVideoFrame::withOwnMetadata(frame);
    // Qt mirrors a frame after rotating it, so a rotation applied on top of a mirrored frame turns the other way
    // (Rot(a) * Mirror == Mirror * Rot(-a)), as Qt's own VideoTransformation::rotate() composes it. QtVideo::Rotation's
    // values are degrees.
    const int added = frame.mirrored() ? (360 - static_cast<int>(rotation)) : static_cast<int>(rotation);
    result.setRotation(static_cast<QtVideo::Rotation>((static_cast<int>(frame.rotation()) + added) % 360));
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
