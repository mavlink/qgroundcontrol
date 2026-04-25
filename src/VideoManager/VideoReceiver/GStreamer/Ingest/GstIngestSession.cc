#include "GstIngestSession.h"

#include "GstRemuxPipeline.h"
#include "GstStreamDevice.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <gst/app/gstappsink.h>
#include <memory>

QGC_LOGGING_CATEGORY(GstIngestSessionLog, "VideoManager.GStreamer.IngestSession")

namespace {

const char* muxerForFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
        case QMediaFormat::Matroska:
            return "matroskamux";
        case QMediaFormat::MPEG4:
            return "mp4mux";
        case QMediaFormat::QuickTime:
            return "qtmux";
        default:
            return nullptr;
    }
}

QUrl ingestPlaybackUrl()
{
    return QUrl::fromLocalFile(QDir::temp().absoluteFilePath(QStringLiteral("qgc-gstreamer-session.ts")));
}

}  // namespace

GstIngestSession::GstIngestSession(QObject* parent)
    : QObject(parent)
{
}

GstIngestSession::~GstIngestSession()
{
    stop();
}

bool GstIngestSession::start(const VideoSourceResolver::SourceDescriptor& source, bool lowLatency)
{
    stop();

    if (!source.needsIngestSession())
        return false;

    _device = std::make_unique<GstStreamDevice>(this);
    _device->resetStream();

    _pipeline = std::make_unique<GstRemuxPipeline>(QStringLiteral("qgc-ingest-session"), this);
    connect(_pipeline.get(), &GstRemuxPipeline::errorOccurred,
            this, [this](VideoReceiver::ErrorCategory category, const QString& message) {
                if (_device)
                    _device->finishStream();
                emit errorOccurred(category, message);
            });
    connect(_pipeline.get(), &GstRemuxPipeline::endOfStream, this, [this]() {
        if (_device)
            _device->finishStream();
        emit endOfStream();
    });

    GstRemuxPipeline::OutputConfig output;
    output.muxerFactory = "mpegtsmux";
    output.sinkFactory = "appsink";
    output.sinkName = "ingest-sink";
    output.leakyQueue = true;
    output.configureSink = [this](GstElement* sink) {
        g_object_set(sink,
                     "emit-signals", TRUE,
                     "max-buffers", 64,
                     "drop", TRUE,
                     "sync", FALSE,
                     "async", FALSE,
                     nullptr);
        (void)g_signal_connect(sink, "new-sample", G_CALLBACK(_onNewSample), this);
    };

    if (!_pipeline->start(source, output, lowLatency ? 25 : 60)) {
        stop();
        return false;
    }

    _playbackUri = source.uri;
    _playbackDeviceUrl = ingestPlaybackUrl();
    qCDebug(GstIngestSessionLog) << "Proxying" << source.uri << "to QIODevice MPEG-TS stream";
    return true;
}

void GstIngestSession::stop()
{
    stopRecording();

    if (_pipeline) {
        _pipeline->stop();
        _pipeline.reset();
    }

    if (_device)
        _device->finishStream();

    _playbackUri.clear();
    _playbackDeviceUrl = {};
    _device.reset();
}

GstFlowReturn GstIngestSession::_handleNewSample()
{
    if (!_pipeline || !_pipeline->sinkElement() || !_device)
        return GST_FLOW_EOS;

    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(_pipeline->sinkElement()));
    if (!sample)
        return GST_FLOW_EOS;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo mapInfo;
    if (!buffer || !gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const bool accepted = _device->append(reinterpret_cast<const char*>(mapInfo.data),
                                         static_cast<qint64>(mapInfo.size));
    gst_buffer_unmap(buffer, &mapInfo);
    gst_sample_unref(sample);
    return accepted ? GST_FLOW_OK : GST_FLOW_FLUSHING;
}

GstFlowReturn GstIngestSession::_onNewSample(GstElement* /*sink*/, gpointer userData)
{
    return static_cast<GstIngestSession*>(userData)->_handleNewSample();
}

QIODevice* GstIngestSession::playbackDevice() const
{
    return _device.get();
}

bool GstIngestSession::running() const
{
    return _pipeline && _pipeline->running();
}

bool GstIngestSession::startRecording(const QString& path, QMediaFormat::FileFormat format)
{
    if (!_pipeline || !_pipeline->running() || path.isEmpty() || isRecording())
        return false;

    const char* muxer = muxerForFormat(format);
    if (!muxer || !GstRemuxPipeline::factoryAvailable(muxer))
        return false;

    GstRemuxPipeline::OutputConfig output;
    output.muxerFactory = muxer;
    output.sinkFactory = "filesink";
    output.sinkName = "ingest-record-sink";
    output.configureSink = [path](GstElement* sink) {
        g_object_set(sink, "location", path.toUtf8().constData(),
                     "sync", FALSE, "async", FALSE, nullptr);
    };

    _recordingOutputId = _pipeline->addOutput(output);
    return _recordingOutputId >= 0;
}

void GstIngestSession::stopRecording()
{
    if (_recordingOutputId < 0)
        return;

    if (_pipeline)
        (void)_pipeline->stopOutput(_recordingOutputId);
    _recordingOutputId = -1;
}

#ifdef QGC_UNITTEST_BUILD
void GstIngestSession::handleBusMessageForTest(GstMessage* message)
{
    if (!_pipeline) {
        _pipeline = std::make_unique<GstRemuxPipeline>(QStringLiteral("qgc-ingest-session-test"), this);
        connect(_pipeline.get(), &GstRemuxPipeline::errorOccurred,
                this, &GstIngestSession::errorOccurred);
        connect(_pipeline.get(), &GstRemuxPipeline::endOfStream,
                this, &GstIngestSession::endOfStream);
    }
    _pipeline->handleBusMessageForTest(message);
}
#endif
