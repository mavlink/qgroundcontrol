#include "GstNativeRecorder.h"

#include "GstRemuxPipeline.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(GstNativeRecorderLog, "VideoManager.GStreamer.NativeRecorder")

GstNativeRecorder::GstNativeRecorder(VideoSourceResolver::SourceDescriptor source, QObject* parent)
    : VideoRecorder(parent)
    , _source(std::move(source))
{
    _startTimeoutTimer.setSingleShot(true);
    _stopTimeoutTimer.setSingleShot(true);

    connect(&_startTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (_state == State::Starting && !_videoLinked)
            _failStartOrRecording(QStringLiteral("Timed out waiting for a video pad from GStreamer source"));
    });

    connect(&_stopTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (_state == State::Stopping) {
            qCWarning(GstNativeRecorderLog) << "Timed out waiting for native recorder EOS";
            _finalizeStop(true);
        }
    });
}

GstNativeRecorder::~GstNativeRecorder()
{
    if (isRecording())
        stop();
    _finalizeStop(false);
}

bool GstNativeRecorder::supportsSource(const VideoSourceResolver::SourceDescriptor& source)
{
    return source.needsIngestSession() && !source.isLocalCamera && !source.uri.isEmpty();
}

bool GstNativeRecorder::start(const QString& path, QMediaFormat::FileFormat format)
{
    if (isRecording()) {
        qCWarning(GstNativeRecorderLog) << "Already recording";
        return false;
    }

    if (!supportsSource(_source)) {
        emit error(QStringLiteral("GStreamer native recording is only available for ingest-session streams"));
        return false;
    }

    if (!_availableFormats().contains(format)) {
        emit error(QStringLiteral("GStreamer native recording does not support the requested container"));
        return false;
    }

    _currentPath = path;
    _outputPreexisting = QFileInfo::exists(path);
    _videoLinked = false;
    _pipelinePlaying = false;
    _startedEmitted = false;
    setState(State::Starting);

    if (!_startPipeline(format)) {
        _removeIncompleteOutput();
        _finalizeStop(false);
        setState(State::Idle);
        return false;
    }

    _pipelinePlaying = true;
    if (_videoLinked)
        _confirmStarted();
    else
        _startTimeoutTimer.start(5000);

    qCDebug(GstNativeRecorderLog) << "Started native recording" << _source.uri << "to" << _currentPath;
    return true;
}

void GstNativeRecorder::stop()
{
    if (_state == State::Idle || _state == State::Stopping)
        return;

    setState(State::Stopping);
    if (!_pipeline || !_pipeline->sendEos()) {
        _finalizeStop(true);
        return;
    }

    _stopTimeoutTimer.start(3000);
}

VideoRecorder::Capabilities GstNativeRecorder::capabilities() const
{
    return Capabilities{
        true,
        _availableFormats(),
        QStringLiteral("GStreamer native recorder (lossless mux of ingest-session stream)"),
    };
}

GstNativeRecorder::MuxerSpec GstNativeRecorder::_muxerForFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
        case QMediaFormat::Matroska:
            return {"matroskamux", format};
        case QMediaFormat::MPEG4:
            return {"mp4mux", format};
        case QMediaFormat::QuickTime:
            return {"qtmux", format};
        default:
            return {};
    }
}

bool GstNativeRecorder::_factoryAvailable(const char* factoryName)
{
    return GstRemuxPipeline::factoryAvailable(factoryName);
}

QList<QMediaFormat::FileFormat> GstNativeRecorder::_availableFormats()
{
    QList<QMediaFormat::FileFormat> formats;
    for (const QMediaFormat::FileFormat format :
         {QMediaFormat::Matroska, QMediaFormat::QuickTime, QMediaFormat::MPEG4}) {
        const MuxerSpec spec = _muxerForFormat(format);
        if (_factoryAvailable(spec.factory))
            formats.append(format);
    }
    return formats;
}

bool GstNativeRecorder::_startPipeline(QMediaFormat::FileFormat format)
{
    const MuxerSpec spec = _muxerForFormat(format);
    if (!_factoryAvailable(spec.factory)) {
        qCWarning(GstNativeRecorderLog) << "Missing GStreamer muxer for recording:" << spec.factory;
        return false;
    }

    _pipeline = std::make_unique<GstRemuxPipeline>(QStringLiteral("qgc-native-recorder"), this);
    connect(_pipeline.get(), &GstRemuxPipeline::errorOccurred,
            this, &GstNativeRecorder::_handlePipelineError);
    connect(_pipeline.get(), &GstRemuxPipeline::endOfStream,
            this, &GstNativeRecorder::_handlePipelineEos);
    connect(_pipeline.get(), &GstRemuxPipeline::videoPadLinked,
            this, &GstNativeRecorder::_handleVideoLinked);

    GstRemuxPipeline::OutputConfig output;
    output.muxerFactory = spec.factory;
    output.sinkFactory = "filesink";
    output.sinkName = "record-sink";
    output.configureSink = [this](GstElement* sink) {
        g_object_set(sink, "location", _currentPath.toUtf8().constData(),
                     "sync", FALSE, "async", FALSE, nullptr);
    };

    return _pipeline->start(_source, output, _source.lowLatencyRecommended ? 25 : 60);
}

void GstNativeRecorder::_handlePipelineError(VideoReceiver::ErrorCategory /*category*/, const QString& message)
{
    _failStartOrRecording(message);
}

void GstNativeRecorder::_handlePipelineEos()
{
    if (!_videoLinked) {
        _failStartOrRecording(QStringLiteral("GStreamer source ended before a video pad was linked"));
        return;
    }

    qCDebug(GstNativeRecorderLog) << "Native recorder reached EOS";
    _finalizeStop(true);
}

void GstNativeRecorder::_handleVideoLinked()
{
    _videoLinked = true;
    if (_pipelinePlaying)
        _confirmStarted();
}

void GstNativeRecorder::_finalizeStop(bool emitStopped)
{
    _startTimeoutTimer.stop();
    _stopTimeoutTimer.stop();
    if (_pipeline) {
        _pipeline->stop();
        _pipeline.reset();
    }

    const State previous = _state;
    setState(State::Idle);
    if (emitStopped && previous != State::Idle)
        emit stopped(_currentPath);

    _videoLinked = false;
    _pipelinePlaying = false;
    _startedEmitted = false;
}

void GstNativeRecorder::_failStartOrRecording(const QString& message)
{
    qCWarning(GstNativeRecorderLog) << "Native recorder error:" << message;
    const bool wasActive = _state != State::Idle;
    const bool hadStarted = _startedEmitted;
    _finalizeStop(false);
    if (!hadStarted)
        _removeIncompleteOutput();
    emit error(message);
    if (wasActive && hadStarted)
        emit stopped(_currentPath);
}

void GstNativeRecorder::_confirmStarted()
{
    if (_startedEmitted || _state != State::Starting)
        return;

    _startTimeoutTimer.stop();
    _startedEmitted = true;
    setState(State::Recording);
    emit started(_currentPath);
}

void GstNativeRecorder::_removeIncompleteOutput()
{
    if (_currentPath.isEmpty() || _outputPreexisting)
        return;
    QFile::remove(_currentPath);
}

#ifdef QGC_UNITTEST_BUILD
void GstNativeRecorder::handleBusMessageForTest(GstMessage* message)
{
    if (!_pipeline) {
        _pipeline = std::make_unique<GstRemuxPipeline>(QStringLiteral("qgc-native-recorder-test"), this);
        connect(_pipeline.get(), &GstRemuxPipeline::errorOccurred,
                this, &GstNativeRecorder::_handlePipelineError);
        connect(_pipeline.get(), &GstRemuxPipeline::endOfStream,
                this, &GstNativeRecorder::_handlePipelineEos);
    }
    _pipeline->handleBusMessageForTest(message);
}
#endif
