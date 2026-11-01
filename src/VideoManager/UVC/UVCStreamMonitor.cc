#include "UVCStreamMonitor.h"

#include <chrono>
#include <utility>

#include <QtCore/QMutexLocker>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UVCStreamMonitorLog, "Video.UVCStreamMonitor")

namespace {

constexpr std::chrono::milliseconds kSampleInterval{1000};

qint64 uvcSteadyClockMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

QString uvcPixelFormatText(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
        case QVideoFrameFormat::Format_Invalid:
            return QString();
        case QVideoFrameFormat::Format_Jpeg:
            // A camera's JPEG frames are motion JPEG.
            return QStringLiteral("MJPEG");
        default:
            return QVideoFrameFormat::pixelFormatToString(pixelFormat);
    }
}

}  // namespace

UVCStreamMonitor::UVCStreamMonitor(QObject* parent, VideoStreamStatsMeter::Clock clock)
    : QObject(parent)
    , _clock(clock ? std::move(clock) : VideoStreamStatsMeter::Clock(uvcSteadyClockMs))
    , _meter(_clock)
{
    _sampleTimer.setInterval(kSampleInterval);
    (void) connect(&_sampleTimer, &QTimer::timeout, this, &UVCStreamMonitor::_sample);
}

void UVCStreamMonitor::attach(QCamera* camera, QVideoSink* videoSink)
{
    if (!videoSink) {
        qCWarning(UVCStreamMonitorLog) << "attach without a video sink";
        return;
    }
    qCDebug(UVCStreamMonitorLog) << "attach" << camera << videoSink;
    _disconnect();
    _camera = camera;
    _videoSink = videoSink;
    _connections.append(connect(
        videoSink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) { _onVideoFrame(frame); },
        Qt::DirectConnection));
    if (camera) {
        _connections.append(connect(camera, &QCamera::activeChanged, this, &UVCStreamMonitor::_restart));
        _connections.append(connect(camera, &QCamera::cameraDeviceChanged, this, &UVCStreamMonitor::_updateCameraName));
        _connections.append(connect(camera, &QCamera::cameraDeviceChanged, this, &UVCStreamMonitor::_restart));
    }
    _restart();
    _updateCameraName();
    _sampleTimer.start();
}

void UVCStreamMonitor::detach(QVideoSink* videoSink)
{
    if (videoSink != _videoSink) {
        return;
    }
    qCDebug(UVCStreamMonitorLog) << "detach" << videoSink;
    _disconnect();
    _restart();
    _updateCameraName();
}

void UVCStreamMonitor::_disconnect()
{
    for (const QMetaObject::Connection& connection : std::as_const(_connections)) {
        (void) disconnect(connection);
    }
    _connections.clear();
    _camera.clear();
    _videoSink.clear();
    _sampleTimer.stop();
}

void UVCStreamMonitor::_onVideoFrame(const QVideoFrame& frame)
{
    // The sink clears the display with an invalid frame when the camera stops.
    if (!frame.isValid()) {
        return;
    }
    const QVideoFrameFormat format = frame.surfaceFormat();
    const QMutexLocker locker(&_frameMutex);
    ++_frameCounters.frames;
    _frameCounters.size = frame.size();
    _frameCounters.pixelFormat = format.pixelFormat();
    _frameCounters.streamFrameRate = format.streamFrameRate();
    _frameCounters.gpu = (frame.handleType() != QVideoFrame::NoHandle);
}

void UVCStreamMonitor::_sample()
{
    if (!_videoSink) {
        // The sink went away without detaching.
        _disconnect();
        _restart();
        _updateCameraName();
        return;
    }
    FrameCounters frameCounters;
    {
        const QMutexLocker locker(&_frameMutex);
        frameCounters = _frameCounters;
    }
    // Rates from before the first frame would only measure the camera's startup.
    if (frameCounters.frames == 0) {
        return;
    }
    const qint64 nowMs = _clock();
    if (!_streamingSinceMs) {
        _streamingSinceMs = nowMs;
    }

    VideoStreamStats stats = _stats;
    stats.codec = uvcPixelFormatText(frameCounters.pixelFormat);
    stats.gpuPath = frameCounters.gpu ? QStringLiteral("GPU") : QStringLiteral("CPU");
    stats.uptimeMs = nowMs - *_streamingSinceMs;
    VideoStreamStatsMeter::Counters counters;
    counters.framesDisplayed = frameCounters.frames;
    if (_meter.update(counters, stats)) {
        _publish(stats, frameCounters.size, frameCounters.streamFrameRate);
    }
}

void UVCStreamMonitor::_restart()
{
    {
        const QMutexLocker locker(&_frameMutex);
        _frameCounters = FrameCounters();
    }
    _meter.reset();
    _streamingSinceMs.reset();
    _publish(VideoStreamStats(), QSize(), 0.0);
}

void UVCStreamMonitor::_updateCameraName()
{
    const QString cameraName = _camera ? _camera->cameraDevice().description() : QString();
    if (cameraName == _cameraName) {
        return;
    }
    _cameraName = cameraName;
    emit cameraNameChanged();
}

void UVCStreamMonitor::_publish(const VideoStreamStats& stats, QSize videoSize, double cameraFrameRate)
{
    if ((stats == _stats) && (videoSize == _videoSize) && (cameraFrameRate == _cameraFrameRate)) {
        return;
    }
    _stats = stats;
    _videoSize = videoSize;
    _cameraFrameRate = cameraFrameRate;
    emit statsChanged();
}
