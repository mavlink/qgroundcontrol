#include "VideoStreamController.h"

#include <utility>

#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QGCVideoStreamInfo.h"
#include "VideoSettings.h"

QGC_LOGGING_CATEGORY(VideoStreamControllerLog, "Video.VideoStreamController")

VideoStreamController::VideoStreamController(Role role, VideoSettings* videoSettings, QObject* parent)
    : QObject(parent)
    , _role(role)
    , _videoSettings(videoSettings)
{
    if (_role == Role::Primary) {
        (void) connect(_videoSettings->aspectRatio(), &Fact::rawValueChanged, this,
                       &VideoStreamController::aspectRatioChanged);
    }
}

VideoStreamController::~VideoStreamController() = default;

QString VideoStreamController::name() const
{
    return (_role == Role::Primary) ? QStringLiteral("videoContent") : QStringLiteral("thermalVideo");
}

QString VideoStreamController::taggedFile(const QString& file) const
{
    if (_role == Role::Primary) {
        return file;
    }
    const QString tag = QLatin1Char('.') + name();
    const qsizetype dot = file.lastIndexOf(QLatin1Char('.'));
    if (dot > file.lastIndexOf(QLatin1Char('/'))) {
        return QString(file).insert(dot, tag);
    }
    return file + tag;
}

void VideoStreamController::setReceiver(VideoReceiver* receiver)
{
    if (receiver == _receiver) {
        return;
    }

    const bool wasStreaming = streaming();
    const bool wasDecoding = decoding();
    const bool wasRecording = recording();
    const QSize oldVideoSize = videoSize();

    // Detach the old receiver first: what it emits while being destroyed must not reach a half-destroyed object.
    VideoReceiver* const oldReceiver = std::exchange(_receiver, receiver);
    if (oldReceiver) {
        (void) disconnect(oldReceiver, nullptr, this, nullptr);
        delete oldReceiver;
    }

    if (_receiver) {
        _receiver->setParent(this);
        _receiver->setName(name());
        (void) connect(_receiver, &VideoReceiver::streamingChanged, this, &VideoStreamController::streamingChanged);
        (void) connect(_receiver, &VideoReceiver::decodingChanged, this, &VideoStreamController::decodingChanged);
        (void) connect(_receiver, &VideoReceiver::recordingChanged, this, &VideoStreamController::recordingChanged);
        (void) connect(_receiver, &VideoReceiver::videoSizeChanged, this, [this](QSize size) {
            qCDebug(VideoStreamControllerLog) << name() << "video size" << size;
            emit videoSizeChanged();
            emit aspectRatioChanged();
        });
        (void) connect(_receiver, &VideoReceiver::recordingStarted, this, &VideoStreamController::recordingStarted);
        (void) connect(_receiver, &VideoReceiver::recordingFinished, this, &VideoStreamController::recordingFinished);
        (void) connect(_receiver, &VideoReceiver::errorOccurred, this, &VideoStreamController::errorOccurred);
        _receiver->setVideoOutput(_videoOutput);
        _apply();
    }

    if (streaming() != wasStreaming) {
        emit streamingChanged();
    }
    if (decoding() != wasDecoding) {
        emit decodingChanged();
    }
    if (recording() != wasRecording) {
        emit recordingChanged(recording());
    }
    if (videoSize() != oldVideoSize) {
        emit videoSizeChanged();
        emit aspectRatioChanged();
    }
}

QGCVideoStreamInfo* VideoStreamController::streamInfo() const
{
    return _streamInfo;
}

void VideoStreamController::setStreamInfo(QGCVideoStreamInfo* streamInfo)
{
    if (streamInfo == _streamInfo) {
        return;
    }

    if (_streamInfo) {
        (void) disconnect(_streamInfo, nullptr, this, nullptr);
    }
    _streamInfo = streamInfo;
    if (_streamInfo) {
        (void) connect(_streamInfo, &QGCVideoStreamInfo::infoChanged, this,
                       &VideoStreamController::_onStreamInfoChanged);
        (void) connect(_streamInfo, &QObject::destroyed, this, &VideoStreamController::_onStreamInfoChanged);
    }
    _onStreamInfoChanged();
}

void VideoStreamController::_onStreamInfoChanged()
{
    emit streamInfoChanged();
    emit aspectRatioChanged();
    scheduleApply();
}

void VideoStreamController::setEnabled(bool enabled)
{
    if (enabled != _enabled) {
        _enabled = enabled;
        scheduleApply();
    }
}

std::optional<VideoReceiver::Config> VideoStreamController::_resolvedConfig() const
{
    return VideoStreamResolver::resolve(VideoStreamResolver::snapshot(_videoSettings), _streamInfo, _role);
}

void VideoStreamController::scheduleApply()
{
    if (_applyPending) {
        return;
    }
    _applyPending = true;
    (void) QMetaObject::invokeMethod(this, &VideoStreamController::_apply, Qt::QueuedConnection);
}

void VideoStreamController::_apply()
{
    _applyPending = false;
    if (!_receiver) {
        return;
    }

    const std::optional<VideoReceiver::Config> config = _resolvedConfig();
    const bool active = _enabled && config.has_value();
    qCDebug(VideoStreamControllerLog) << name() << "active:" << active << "uri:"
                                      << (config ? QGCNetworkHelper::redactedUrlForLogging(config->uri) : QString());

    // Configure before activating and deactivate before configuring, so the receiver never starts a stream that is
    // about to change.
    if (active) {
        _receiver->setConfig(*config);
        _receiver->setActive(true);
    } else {
        _receiver->setActive(false);
        if (config) {
            _receiver->setConfig(*config);
        }
    }
}

void VideoStreamController::stop()
{
    _enabled = false;
    _apply();
}

void VideoStreamController::restart()
{
    if (_receiver) {
        _receiver->restart();
    }
}

QQuickItem* VideoStreamController::videoOutput() const
{
    return _videoOutput;
}

void VideoStreamController::registerVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput) {
        return;
    }

    if (videoOutput == _videoOutput) {
        qCDebug(VideoStreamControllerLog) << name() << "re-attaching video output" << videoOutput;
        if (_receiver) {
            _receiver->setVideoOutput(videoOutput);
        }
        return;
    }

    qCDebug(VideoStreamControllerLog) << name() << "video output" << videoOutput;
    _videoOutput = videoOutput;
    if (_receiver) {
        _receiver->setVideoOutput(videoOutput);
    }
}

void VideoStreamController::unregisterVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput || (videoOutput != _videoOutput)) {
        return;
    }

    qCDebug(VideoStreamControllerLog) << name() << "video output removed" << videoOutput;
    _videoOutput = nullptr;
    if (_receiver) {
        _receiver->setVideoOutput(nullptr);
    }
}

bool VideoStreamController::active() const
{
    return _receiver && _receiver->active();
}

void VideoStreamController::startRecording(const QString& file, VideoReceiver::FileFormat format)
{
    if (_receiver) {
        _receiver->startRecording(file, format);
    }
}

void VideoStreamController::stopRecording()
{
    if (_receiver) {
        _receiver->stopRecording();
    }
}

bool VideoStreamController::canTakeScreenshot() const
{
    return _receiver && _receiver->streaming() && _receiver->videoOutput();
}

bool VideoStreamController::takeScreenshot(const QString& file) const
{
    return _receiver && _receiver->takeScreenshot(file);
}

bool VideoStreamController::streaming() const
{
    return _receiver && _receiver->streaming();
}

bool VideoStreamController::decoding() const
{
    return _receiver && _receiver->decoding();
}

bool VideoStreamController::recording() const
{
    return _receiver && _receiver->recording();
}

QSize VideoStreamController::videoSize() const
{
    return _receiver ? _receiver->videoSize() : QSize();
}

double VideoStreamController::aspectRatio() const
{
    const QSize size = videoSize();
    if (!size.isEmpty()) {
        return static_cast<double>(size.width()) / size.height();
    }
    if (_streamInfo) {
        return _streamInfo->aspectRatio();
    }
    return (_role == Role::Primary) ? _videoSettings->aspectRatio()->rawValue().toDouble() : 1.0;
}
