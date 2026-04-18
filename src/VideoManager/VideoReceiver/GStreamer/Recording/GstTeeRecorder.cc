#include "GstTeeRecorder.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstTeeRecorderLog, "Video.GstTeeRecorder")

#ifdef QGC_GST_STREAMING
#include <glib.h>
#include <gst/gst.h>
#include <QtMultimedia/QMediaFormat>

#include "GstVideoReceiver.h"
#include "VideoReceiver.h"

GstTeeRecorder::GstTeeRecorder(QObject* parent) : VideoRecorder(parent) {}

GstTeeRecorder::~GstTeeRecorder()
{
    if (_receiver && isRecording()) {
        // Best-effort: if destroyed while recording, ask the receiver-owned
        // branch to close. The receiver may already be tearing down; the slot
        // handles missing pipeline state.
        _receiver->stopRecordingBranch();
    }
    disconnect(_eosDrainedConn);
    disconnect(_startedConn);
    disconnect(_pipelineStoppingConn);
}

void GstTeeRecorder::setReceiver(GstVideoReceiver* receiver)
{
    disconnect(_eosDrainedConn);
    disconnect(_startedConn);
    disconnect(_pipelineStoppingConn);

    _receiver = receiver;

    if (_receiver) {
        _startedConn = connect(_receiver, &GstVideoReceiver::recordingBranchStarted,
                               this, &GstTeeRecorder::_onBranchStarted);
        _eosDrainedConn = connect(_receiver, &GstVideoReceiver::recordingBranchEosDrained,
                                  this, &GstTeeRecorder::_onBranchEosDrained);
        _pipelineStoppingConn = connect(_receiver, &GstVideoReceiver::pipelineStopping,
                                        this, &GstTeeRecorder::_onPipelineStopping);
    }
}

bool GstTeeRecorder::start(const QString& path, QMediaFormat::FileFormat format)
{
    if (isRecording()) {
        qCWarning(GstTeeRecorderLog) << "Already recording";
        return false;
    }

    if (!_receiver) {
        qCWarning(GstTeeRecorderLog) << "No GstVideoReceiver set";
        emit error(QStringLiteral("GStreamer receiver not available for recording"));
        return false;
    }

    _currentPath = path;
    setState(State::Starting);

    _receiver->startRecordingBranch(path, format);
    return true;
}

void GstTeeRecorder::stop()
{
    if (_state == State::Idle || _state == State::Stopping)
        return;

    if (!_receiver) {
        setState(State::Idle);
        emit stopped(_currentPath);
        return;
    }

    setState(State::Stopping);

    _receiver->stopRecordingBranch();
    // Completion arrives via _onBranchEosDrained.
}

VideoRecorder::Capabilities GstTeeRecorder::capabilities() const
{
    return Capabilities{
        /*lossless=*/true,
        {QMediaFormat::Matroska, QMediaFormat::QuickTime, QMediaFormat::MPEG4},
        QStringLiteral("GStreamer tee (lossless; may downgrade to MKV for newer codecs)"),
    };
}

void GstTeeRecorder::_onBranchStarted(const QString& path)
{
    if (_state != State::Starting)
        return;

    _currentPath = path;
    setState(State::Recording);
    emit started(path);
}

void GstTeeRecorder::_onBranchEosDrained(bool success)
{
    if (_state != State::Starting && _state != State::Recording && _state != State::Stopping)
        return;

    const QString path = _currentPath;
    setState(State::Idle);
    if (!success)
        emit error(QStringLiteral("Recording stop failed"));
    emit stopped(path);
}

void GstTeeRecorder::_onPipelineStopping()
{
    if (_state != State::Idle) {
        const QString path = _currentPath;
        setState(State::Idle);
        emit stopped(path);
    }
}

#else  // QGC_GST_STREAMING not defined

GstTeeRecorder::GstTeeRecorder(QObject* parent) : VideoRecorder(parent) {}

bool GstTeeRecorder::start(const QString& path, QMediaFormat::FileFormat format)
{
    Q_UNUSED(path)
    Q_UNUSED(format)
    emit error(QStringLiteral("GStreamer not compiled in"));
    return false;
}

void GstTeeRecorder::stop() {}

VideoRecorder::Capabilities GstTeeRecorder::capabilities() const
{
    return Capabilities{false, {}, QStringLiteral("GStreamer not compiled in")};
}

#endif  // QGC_GST_STREAMING
