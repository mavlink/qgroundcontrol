#include "VideoReceiver.h"

#include <QtGui/QTransform>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoReceiverLog, "Video.VideoReceiver")

VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent)
{}

VideoReceiver::~VideoReceiver() = default;

QString VideoReceiver::fileExtension(FileFormat format)
{
    switch (format) {
        case FileFormat::Mkv:
            return QStringLiteral("mkv");
        case FileFormat::Mov:
            return QStringLiteral("mov");
        case FileFormat::Mp4:
            return QStringLiteral("mp4");
    }
    return QStringLiteral("mkv");
}

bool VideoReceiver::isValidFileFormat(int format)
{
    return (format >= static_cast<int>(FileFormat::Mkv)) && (format <= static_cast<int>(FileFormat::Mp4));
}

QImage VideoReceiver::frameToImage(const QVideoFrame& frame)
{
    if (!frame.isValid()) {
        return {};
    }

    // toImage() applies only the surface format's own transformation: not the frame's rotation or mirroring, nor the
    // viewport that crops decoder padding.
    QImage image = frame.toImage();
    if (image.isNull()) {
        return image;
    }

    const QRect viewport = frame.surfaceFormat().viewport();
    if (viewport.isValid() && (image.size() == frame.size()) && (viewport != image.rect())) {
        image = image.copy(viewport.intersected(image.rect()));
    }

    // Same order as Qt's presentation (qNormalizedFrameTransformation): rotate, then mirror horizontally.
    const int degrees = static_cast<int>(frame.rotation());
    if (degrees != 0) {
        image = image.transformed(QTransform().rotate(degrees));
    }
    if (frame.mirrored()) {
        image = image.flipped(Qt::Horizontal);
    }
    return image;
}

void VideoReceiver::setName(const QString& name)
{
    _name = name;
}

void VideoReceiver::setConfig(const Config& config)
{
    if (config == _config) {
        return;
    }
    const Config previous = _config;
    _config = config;
    onConfigChanged(previous);
}

void VideoReceiver::setActive(bool active)
{
    if (active == _active) {
        return;
    }
    _active = active;
    onActiveChanged();
}

void VideoReceiver::restart()
{
    if (_active) {
        onRestartRequested();
    }
}

QQuickItem* VideoReceiver::videoOutput() const
{
    return _videoOutput;
}

void VideoReceiver::setVideoOutput(QQuickItem* videoOutput)
{
    if (videoOutput == _videoOutput) {
        return;
    }
    (void) disconnect(_videoOutputDestroyed);
    _videoOutput = videoOutput;
    if (videoOutput) {
        _videoOutputDestroyed =
            connect(videoOutput, &QObject::destroyed, this, &VideoReceiver::_onVideoOutputDestroyed);
    }
    onVideoOutputChanged();
}

void VideoReceiver::_onVideoOutputDestroyed()
{
    _videoOutput = nullptr;
    onVideoOutputChanged();
}

void VideoReceiver::startRecording(const QString& file, FileFormat format)
{
    if (!_active) {
        qCWarning(VideoReceiverLog) << "Not recording" << _name << "- the stream is not active";
        emit errorOccurred(tr("Unable to record video: the video stream is not active."));
        return;
    }
    if (file.isEmpty()) {
        qCWarning(VideoReceiverLog) << "Not recording" << _name << "- no file name";
        emit errorOccurred(tr("Unable to record video: no file name."));
        return;
    }
    onStartRecording(file, format);
}

void VideoReceiver::stopRecording()
{
    onStopRecording();
}

bool VideoReceiver::takeScreenshot(const QString& file) const
{
    QVideoSink* const sink = videoSink();
    if (!sink) {
        qCWarning(VideoReceiverLog) << "No video output to take a screenshot from" << _name;
        return false;
    }

    const QImage image = frameToImage(sink->videoFrame());
    if (image.isNull()) {
        qCWarning(VideoReceiverLog) << "No video frame to take a screenshot from" << _name;
        return false;
    }

    if (!image.save(file)) {
        qCWarning(VideoReceiverLog) << "Failed to save screenshot" << file;
        return false;
    }

    qCDebug(VideoReceiverLog) << "Screenshot saved:" << file;
    return true;
}

QVideoSink* VideoReceiver::videoSink() const
{
    return _videoOutput ? _videoOutput->property("videoSink").value<QVideoSink*>() : nullptr;
}

void VideoReceiver::setState(State state)
{
    if (state == _state) {
        return;
    }
    const bool wasStreaming = streaming();
    _state = state;
    emit stateChanged(_state);
    if (wasStreaming != streaming()) {
        emit streamingChanged(streaming());
    }
}

void VideoReceiver::setDecoding(bool decoding)
{
    if (decoding != _decoding) {
        _decoding = decoding;
        emit decodingChanged(_decoding);
    }
}

void VideoReceiver::setRecording(bool recording)
{
    if (recording != _recording) {
        _recording = recording;
        emit recordingChanged(_recording);
    }
}

void VideoReceiver::setVideoSize(QSize size)
{
    if (size != _videoSize) {
        _videoSize = size;
        emit videoSizeChanged(_videoSize);
    }
}
