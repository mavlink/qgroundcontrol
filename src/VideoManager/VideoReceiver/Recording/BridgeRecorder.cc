#include "BridgeRecorder.h"

#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QVideoFrame>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(BridgeRecorderLog, "Video.BridgeRecorder")

// The three container formats supported via QMediaRecorder.
static const QList<QMediaFormat::FileFormat> kSupportedFormats = {
    QMediaFormat::Matroska,
    QMediaFormat::QuickTime,
    QMediaFormat::MPEG4,
};

BridgeRecorder::BridgeRecorder(VideoFrameDelivery* delivery, QObject* parent)
    : VideoRecorder(parent), _delivery(delivery)
{
    _captureSession.setRecorder(&_recorder);

    connect(&_recorder, &QMediaRecorder::recorderStateChanged, this, &BridgeRecorder::_onRecorderStateChanged);
    connect(&_recorder, &QMediaRecorder::errorOccurred, this, [this](QMediaRecorder::Error /*error*/, const QString& msg) {
        qCWarning(BridgeRecorderLog) << "Recorder error:" << msg;
        emit error(msg);
    });
}

BridgeRecorder::~BridgeRecorder()
{
    if (isRecording())
        stop();
}

bool BridgeRecorder::start(const QString& path, QMediaFormat::FileFormat format)
{
    if (isRecording()) {
        qCWarning(BridgeRecorderLog) << "Already recording";
        return false;
    }

    if (!_delivery) {
        qCWarning(BridgeRecorderLog) << "No frame delivery available";
        emit error(QStringLiteral("No frame delivery available for recording"));
        return false;
    }

    QVideoFrame seedFrame = _delivery->lastRawFrame();
    const QSize size = seedFrame.isValid() ? seedFrame.size() : _delivery->videoSize();
    if (size.isEmpty()) {
        qCWarning(BridgeRecorderLog) << "No video size available yet";
        emit error(QStringLiteral("Video not yet streaming — cannot determine frame size"));
        return false;
    }

    const auto pixelFormat = (seedFrame.isValid() && seedFrame.pixelFormat() != QVideoFrameFormat::Format_Invalid)
                                 ? seedFrame.pixelFormat()
                                 : QVideoFrameFormat::Format_NV12;

    _currentPath = path;
    setState(State::Starting);

    delete _frameInput;
    _frameInput = new QVideoFrameInput(QVideoFrameFormat(size, pixelFormat), this);
    _captureSession.setVideoFrameInput(_frameInput);

    _recorder.setMediaFormat(QMediaFormat(format));
    _selectBestVideoCodec();
    _recorder.setOutputLocation(QUrl::fromLocalFile(path));
    _recorder.setVideoFrameRate(0);  // auto
    _recorder.setVideoResolution(size);
    _recorder.setAutoStop(true);

    _frameConn = connect(_delivery, &VideoFrameDelivery::frameArrived, this, &BridgeRecorder::_tryPushFrame);
    _readyConn = connect(_frameInput, &QVideoFrameInput::readyToSendVideoFrame, this, &BridgeRecorder::_tryPushFrame);

    _recorder.record();
    qCDebug(BridgeRecorderLog) << "Started recording to" << path << "size:" << size;
    return true;
}

void BridgeRecorder::stop()
{
    if (_state == State::Idle || _state == State::Stopping)
        return;

    setState(State::Stopping);

    disconnect(_frameConn);
    disconnect(_readyConn);

    if (_frameInput)
        _frameInput->sendVideoFrame(QVideoFrame{});

    _recorder.stop();

    if (_frameInput) {
        _captureSession.setVideoFrameInput(nullptr);
        delete _frameInput;
        _frameInput = nullptr;
    }

    qCDebug(BridgeRecorderLog) << "Stopped recording";
}

VideoRecorder::Capabilities BridgeRecorder::capabilities() const
{
    // Platform-supported encode formats are stable for the process lifetime;
    // cache the intersection with kSupportedFormats on first call.
    static const QList<QMediaFormat::FileFormat> kAvailable = [] {
        const QList<QMediaFormat::FileFormat> platformFormats =
            QMediaFormat().supportedFileFormats(QMediaFormat::Encode);
        QList<QMediaFormat::FileFormat> a;
        for (auto fmt : kSupportedFormats) {
            if (platformFormats.contains(fmt))
                a.append(fmt);
        }
        return a.isEmpty() ? kSupportedFormats : a;
    }();

    return Capabilities{
        /*lossless=*/false,
        kAvailable,
        QStringLiteral("QMediaRecorder (re-encoded via VideoFrameDelivery)"),
    };
}

void BridgeRecorder::_onRecorderStateChanged(QMediaRecorder::RecorderState recState)
{
    switch (recState) {
        case QMediaRecorder::RecordingState:
            _currentPath = _recorder.actualLocation().toLocalFile();
            setState(State::Recording);
            emit started(_currentPath);
            break;
        case QMediaRecorder::StoppedState: {
            const QString path = _recorder.actualLocation().toLocalFile();
            const State prev = _state;
            setState(State::Idle);
            if (prev != State::Idle)
                emit stopped(path.isEmpty() ? _currentPath : path);
            break;
        }
        default:
            break;
    }
}

void BridgeRecorder::_tryPushFrame()
{
    if (!_frameInput || _state != State::Recording || !_delivery)
        return;

    const QVideoFrame frame = _delivery->lastRawFrame();
    if (frame.isValid())
        _frameInput->sendVideoFrame(frame);
}

void BridgeRecorder::_selectBestVideoCodec()
{
    QMediaFormat fmt = _recorder.mediaFormat();
    const auto supported = fmt.supportedVideoCodecs(QMediaFormat::Encode);

    if (supported.contains(QMediaFormat::VideoCodec::H264)) {
        fmt.setVideoCodec(QMediaFormat::VideoCodec::H264);
        qCDebug(BridgeRecorderLog) << "Selected H.264 encoder";
    } else if (supported.contains(QMediaFormat::VideoCodec::H265)) {
        fmt.setVideoCodec(QMediaFormat::VideoCodec::H265);
        qCDebug(BridgeRecorderLog) << "Selected H.265 encoder";
    } else {
        qCDebug(BridgeRecorderLog) << "Using default video codec";
    }

    _recorder.setMediaFormat(fmt);
}
