#include "VideoSinkBinder.h"

#include "VideoStream.h"

#include <QtCore/QCoreApplication>
#include <QtQml/qqml.h>

namespace {

void registerVideoSinkBinderQmlType()
{
    qmlRegisterType<VideoSinkBinder>("QGroundControl.VideoManager", 1, 0, "VideoSinkBinder");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerVideoSinkBinderQmlType)

VideoSinkBinder::VideoSinkBinder(QObject* parent)
    : QObject(parent)
{
}

VideoSinkBinder::~VideoSinkBinder()
{
    _clearRegisteredSink();
}

void VideoSinkBinder::setStream(VideoStream* stream)
{
    if (_stream == stream)
        return;

    _stream = stream;
    _rebind();
    emit streamChanged();
}

void VideoSinkBinder::setVideoSink(QVideoSink* videoSink)
{
    if (_videoSink == videoSink)
        return;

    _videoSink = videoSink;
    _rebind();
    emit videoSinkChanged();
}

void VideoSinkBinder::_rebind()
{
    if (_registeredStream && (_registeredStream != _stream || _registeredSink != _videoSink))
        _clearRegisteredSink();

    if (!_registeredStream && _stream && _videoSink) {
        _stream->registerVideoSink(_videoSink);
        _registeredStream = _stream;
        _registeredSink = _videoSink;
    }
}

void VideoSinkBinder::_clearRegisteredSink()
{
    if (_registeredStream)
        _registeredStream->registerVideoSink(nullptr);

    _registeredStream = nullptr;
    _registeredSink = nullptr;
}
