#include "VideoStreamController.h"

#include <utility>

#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QGCVideoStreamInfo.h"
#include "RecordingPlan.h"
#include "VideoSettings.h"

QGC_LOGGING_CATEGORY(VideoStreamControllerLog, "Video.VideoStreamController")

namespace {

QString nameForRole(VideoStreamResolver::Role role)
{
    switch (role) {
        case VideoStreamResolver::Role::Primary:
            break;
        case VideoStreamResolver::Role::Thermal:
            return QStringLiteral("thermalVideo");
        case VideoStreamResolver::Role::Camera:
            return QStringLiteral("uvcCamera");
        case VideoStreamResolver::Role::Tile:
            return QStringLiteral("videoTile");
    }
    return QStringLiteral("videoContent");
}

/// The rotation @p streamInfo reports, when it is one a VideoOutput can show: a multiple of 90 degrees. 0 otherwise.
int shownRotation(const QGCVideoStreamInfo* streamInfo)
{
    if (!streamInfo) {
        return 0;
    }
    const int degrees = streamInfo->rotation() % 360;
    if ((degrees % 90) != 0) {
        qCDebug(VideoStreamControllerLog) << "Ignoring the stream's rotation of" << degrees << "degrees";
        return 0;
    }
    return degrees;
}

}  // namespace

VideoStreamController::VideoStreamController(Role role, VideoSettings* videoSettings, QObject* parent,
                                             VideoReceiver* ownReceiver)
    : QObject(parent)
    , _role(role)
    , _name(nameForRole(role))
    , _videoSettings(videoSettings)
    , _idle(role == Role::Thermal)
{
    // The setting stands in for the primary stream's ratio, which a stream in a tile may become.
    (void) connect(_videoSettings->aspectRatio(), &Fact::rawValueChanged, this,
                   &VideoStreamController::aspectRatioChanged);
    _idleTimer.setSingleShot(true);
    _idleTimer.setInterval(kIdleGrace);
    (void) connect(&_idleTimer, &QTimer::timeout, this, &VideoStreamController::_onIdleTimeout);
    if (ownReceiver) {
        setReceiver(ownReceiver);
        _hasOwnReceiver = true;
    }
}

VideoStreamController::~VideoStreamController() = default;

void VideoStreamController::setRole(Role role)
{
    if (role == _role) {
        return;
    }
    _role = role;
    emit roleChanged();
    emit aspectRatioChanged();
    scheduleApply();
}

void VideoStreamController::setTile(bool tile)
{
    if (tile != _tile) {
        _tile = tile;
        emit tileChanged();
    }
}

void VideoStreamController::setDisplayName(const QString& displayName)
{
    if (displayName != _displayName) {
        _displayName = displayName;
        emit displayNameChanged();
    }
}

QString VideoStreamController::taggedFile(const QString& file) const
{
    if (_role != Role::Thermal) {
        return file;
    }
    return RecordingPlan::insertBeforeExtension(file, QLatin1Char('.') + name());
}

void VideoStreamController::shutDown()
{
    if (!_hasOwnReceiver) {
        setReceiver(nullptr);
        return;
    }
    // Disabled first: a pending _apply() must not bring the receiver back.
    _enabled = false;
    _receiver->shutDown();
}

void VideoStreamController::setReceiver(VideoReceiver* receiver)
{
    if (_hasOwnReceiver || (receiver == _receiver)) {
        return;
    }

    const bool wasDecoding = decoding();
    const bool wasRecording = recording();
    const bool wasEncoding = encodesRecordings();
    const QSize oldVideoSize = videoSize();
    const VideoStreamStats oldStats = stats();

    // Detach the old receiver first: what it emits while being destroyed must not reach a half-destroyed object.
    // No receiver is set while it goes: one that finishes a recording runs the event loop in its destructor, from
    // where this function may be called again.
    _decoded = false;
    _recordingRequested = false;
    if (VideoReceiver* const oldReceiver = std::exchange(_receiver, nullptr)) {
        (void) disconnect(oldReceiver, nullptr, this, nullptr);
        delete oldReceiver;
    }

    if (_receiver) {
        // Set by such a call, which came after this one: its receiver stays.
        if (receiver != _receiver) {
            delete receiver;
        }
    } else if (receiver) {
        _receiver = receiver;
        _receiver->setParent(this);
        _receiver->setName(name());
        (void) connect(_receiver, &VideoReceiver::decodingChanged, this, [this] {
            // Updated first: a stream that loses its decoder is reconnecting by the time decodingChanged arrives, so
            // QML never sees it neither decoding nor reconnecting, which would clear the frame it keeps.
            if (decoding()) {
                _decoded = true;
            }
            _updateReconnecting();
            emit decodingChanged();
        });
        (void) connect(_receiver, &VideoReceiver::stateChanged, this, &VideoStreamController::_onReceiverStateChanged);
        (void) connect(_receiver, &VideoReceiver::recordingChanged, this, [this](bool recording) {
            if (!recording) {
                _recordingRequested = false;
            }
            _updateInUse();
            emit recordingChanged(recording);
        });
        (void) connect(_receiver, &VideoReceiver::videoSizeChanged, this, [this](QSize size) {
            qCDebug(VideoStreamControllerLog) << name() << "video size" << size;
            emit videoSizeChanged();
            emit aspectRatioChanged();
        });
        (void) connect(_receiver, &VideoReceiver::statsChanged, this, &VideoStreamController::statsChanged);
        (void) connect(_receiver, &VideoReceiver::recordingStarted, this, &VideoStreamController::recordingStarted);
        (void) connect(_receiver, &VideoReceiver::recordingFinished, this,
                       [this](const QString& file, const QString& error) {
                           // A recording that failed before it began tells of its end this way alone.
                           if (!error.isEmpty() && !recording()) {
                               _recordingRequested = false;
                               _updateInUse();
                           }
                           emit recordingFinished(file, error);
                       });
        (void) connect(_receiver, &VideoReceiver::errorOccurred, this, &VideoStreamController::errorOccurred);
        _receiver->setDisplayConfig(_resolvedDisplayConfig());
        _receiver->setRecordingEncoding(_recordingEncoding);
        _receiver->setVideoOutput(_displaySuspended ? nullptr : _videoOutput.data());
        _apply();
    }

    if (decoding() != wasDecoding) {
        emit decodingChanged();
    }
    if (recording() != wasRecording) {
        emit recordingChanged(recording());
    }
    if (encodesRecordings() != wasEncoding) {
        emit encodesRecordingsChanged();
    }
    if (videoSize() != oldVideoSize) {
        emit videoSizeChanged();
        emit aspectRatioChanged();
    }
    if (stats() != oldStats) {
        emit statsChanged();
    }
    _updateReconnecting();
    _updateInUse();
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
    if (const int rotation = shownRotation(_streamInfo); rotation != _rotation) {
        qCDebug(VideoStreamControllerLog) << name() << "rotation" << rotation;
        _rotation = rotation;
        emit rotationChanged();
        emit videoSizeChanged();
    }
    emit aspectRatioChanged();
    scheduleApply();
}

void VideoStreamController::setCamera(const QString& camera)
{
    if (camera != _camera) {
        _camera = camera;
        scheduleApply();
    }
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
    if (_role == Role::Camera) {
        if (_camera.isEmpty()) {
            return std::nullopt;
        }
        VideoReceiver::Config config;
        config.uri = _camera;
        return config;
    }
    return VideoStreamResolver::resolve(VideoStreamResolver::snapshot(_videoSettings), _streamInfo, _role);
}

VideoReceiver::DisplayConfig VideoStreamController::_resolvedDisplayConfig() const
{
    return VideoStreamResolver::displayConfig(VideoStreamResolver::snapshot(_videoSettings));
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
    const std::optional<VideoReceiver::Config> config = _resolvedConfig();
    if (const QString uri = config ? config->uri : QString(); uri != _uri) {
        _uri = uri;
        _forgetDecoded();
        emit uriChanged();
    }
    if (!_receiver) {
        return;
    }

    const bool wanted = _enabled && config.has_value();
    if (!wanted && _recordingRequested) {
        // The receiver ends its recording, one it has yet to begin too, as it stops.
        _recordingRequested = false;
        _updateInUse();
    }
    const bool active = wanted && !_idle;
    if (!active) {
        _forgetDecoded();
    }
    qCDebug(VideoStreamControllerLog) << name() << "active:" << active << "uri:" << uri();

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
        _receiver->setDisplayConfig(_resolvedDisplayConfig());
        _receiver->restart();
    }
}

void VideoStreamController::reconnectNow()
{
    if (_receiver) {
        _receiver->reconnectNow();
    }
}

void VideoStreamController::_onReceiverStateChanged(VideoReceiver::State state)
{
    if (state == VideoReceiver::State::Stopped) {
        _decoded = false;
    }
    _updateReconnecting();
}

void VideoStreamController::_forgetDecoded()
{
    _decoded = false;
    _updateReconnecting();
}

void VideoStreamController::_updateReconnecting()
{
    const bool reconnecting =
        _decoded && _receiver && !_receiver->decoding() && (_receiver->state() != VideoReceiver::State::Stopped);
    if (reconnecting != _reconnecting) {
        _reconnecting = reconnecting;
        qCDebug(VideoStreamControllerLog) << name() << "reconnecting" << reconnecting;
        emit reconnectingChanged();
    }
}

bool VideoStreamController::_inUse() const
{
    return _videoOutput || _recordingRequested || recording();
}

void VideoStreamController::_updateInUse()
{
    if (_role != Role::Thermal) {
        return;
    }
    if (!_inUse()) {
        if (!_idle && !_idleTimer.isActive()) {
            _idleTimer.start();
        }
        return;
    }
    _idleTimer.stop();
    if (_idle) {
        qCDebug(VideoStreamControllerLog) << name() << "in use";
        _idle = false;
        scheduleApply();
    }
}

void VideoStreamController::_onIdleTimeout()
{
    if (!_inUse()) {
        qCDebug(VideoStreamControllerLog) << name() << "out of use";
        _idle = true;
        _apply();
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

    qCDebug(VideoStreamControllerLog) << name()
                                      << ((videoOutput == _videoOutput) ? "re-attaching video output" : "video output")
                                      << videoOutput;
    _videoOutput = videoOutput;
    // An output that goes away without a word leaves the stream out of use as well.
    (void) connect(videoOutput, &QObject::destroyed, this, &VideoStreamController::_updateInUse, Qt::UniqueConnection);
    // Also for the current output: the receiver binds it anew.
    if (_receiver && !_displaySuspended) {
        _receiver->setVideoOutput(videoOutput);
    }
    _updateInUse();
}

void VideoStreamController::unregisterVideoOutput(QQuickItem* videoOutput)
{
    if (!videoOutput || (videoOutput != _videoOutput)) {
        return;
    }

    qCDebug(VideoStreamControllerLog) << name() << "video output removed" << videoOutput;
    _videoOutput = nullptr;
    // Queued: the output that takes this one's place, in the main picture or in a tile, registers in the same pass.
    (void) QMetaObject::invokeMethod(this, &VideoStreamController::_detachVideoOutput, Qt::QueuedConnection);
    _updateInUse();
}

void VideoStreamController::_detachVideoOutput()
{
    if (!_videoOutput && _receiver) {
        // Not decoded without an output, and decoded again from the next keyframe once it has one: the stream wasn't
        // lost, so the output it returns to waits for video rather than report a reconnect.
        _forgetDecoded();
        _receiver->setVideoOutput(nullptr);
    }
}

void VideoStreamController::setDisplaySuspended(bool suspended)
{
    if (suspended == _displaySuspended) {
        return;
    }
    _displaySuspended = suspended;
    qCDebug(VideoStreamControllerLog) << name() << (suspended ? "display suspended" : "display resumed");
    if (!_receiver || !_videoOutput) {
        return;
    }
    if (suspended) {
        // As for an output that was removed: the stream is not lost, and its output waits for video on its return.
        _forgetDecoded();
        _receiver->setVideoOutput(nullptr);
    } else {
        _receiver->setVideoOutput(_videoOutput);
    }
}

bool VideoStreamController::canRecord() const
{
    return _receiver && _enabled && configured();
}

void VideoStreamController::startRecording(const QString& file, VideoReceiver::FileFormat format)
{
    if (!_receiver) {
        return;
    }
    _recordingRequested = true;
    _updateInUse();
    // Now, not on the next pass: a receiver takes a recording only while it is active, which the thermal stream out
    // of use isn't.
    _apply();
    _receiver->startRecording(file, format);
}

void VideoStreamController::stopRecording()
{
    _recordingRequested = false;
    if (_receiver) {
        _receiver->stopRecording();
    }
    _updateInUse();
}

bool VideoStreamController::encodesRecordings() const
{
    return _receiver && _receiver->encodesRecordings();
}

void VideoStreamController::setRecordingEncoding(const VideoReceiver::RecordingEncoding& encoding)
{
    _recordingEncoding = encoding;
    if (_receiver) {
        _receiver->setRecordingEncoding(encoding);
    }
}

bool VideoStreamController::canTakeScreenshot() const
{
    return _receiver && _receiver->streaming() && _receiver->videoOutput();
}

bool VideoStreamController::takeScreenshot(const QString& file, const VideoScreenshot::Exif& exif) const
{
    return _receiver && _receiver->takeScreenshot(file, exif);
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
    const QSize size = _receiver ? _receiver->videoSize() : QSize();
    return ((_rotation % 180) != 0) ? size.transposed() : size;
}

double VideoStreamController::aspectRatio() const
{
    const QSize size = videoSize();
    if (!size.isEmpty()) {
        return static_cast<double>(size.width()) / size.height();
    }
    // A camera that advertises no resolution leaves the ratio to the setting.
    if (_streamInfo && !_streamInfo->resolution().isEmpty()) {
        const double advertised = _streamInfo->aspectRatio();
        return ((_rotation % 180) != 0) ? (1.0 / advertised) : advertised;
    }
    return (_role == Role::Primary) ? _videoSettings->aspectRatio()->rawValue().toDouble() : 1.0;
}

VideoStreamStats VideoStreamController::stats() const
{
    return _receiver ? _receiver->stats() : VideoStreamStats{};
}

QString VideoStreamController::uri() const
{
    if (_uri.isEmpty() || (_role == Role::Camera)) {
        return _uri;
    }
    return QGCNetworkHelper::redactedUrlForLogging(_uri);
}

QString VideoStreamController::transport() const
{
    return (_role == Role::Camera) ? QString() : VideoStreamResolver::transportName(_uri);
}
