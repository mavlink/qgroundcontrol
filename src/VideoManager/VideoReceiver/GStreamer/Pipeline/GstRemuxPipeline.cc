#include "GstRemuxPipeline.h"

#include "GstSourceFactory.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMetaObject>
#include <QtCore/QSemaphore>
#include <gst/pbutils/missing-plugins.h>
#include <algorithm>
#include <memory>
#include <utility>

QGC_LOGGING_CATEGORY(GstRemuxPipelineLog, "VideoManager.GStreamer.RemuxPipeline")

namespace {

GstCaps* padCaps(GstPad* pad)
{
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps)
        caps = gst_pad_query_caps(pad, nullptr);
    return caps;
}

QString describeCaps(GstCaps* caps)
{
    if (!caps)
        return QStringLiteral("<none>");

    gchar* description = gst_caps_to_string(caps);
    const QString out = description ? QString::fromUtf8(description) : QStringLiteral("<unprintable>");
    g_clear_pointer(&description, g_free);
    return out;
}

bool capsNameIsVideo(const char* name)
{
    return name && (g_str_has_prefix(name, "video/") || g_str_equal(name, "application/x-rtp"));
}

const GstStructure* firstVideoStructure(GstCaps* caps)
{
    if (!caps)
        return nullptr;

    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstStructure* structure = gst_caps_get_structure(caps, i);
        if (!structure)
            continue;
        const char* name = gst_structure_get_name(structure);
        if (capsNameIsVideo(name))
            return structure;
    }

    return nullptr;
}

const char* parserFactoryForCaps(GstCaps* caps)
{
    const GstStructure* structure = firstVideoStructure(caps);
    if (!structure)
        return nullptr;

    const char* name = gst_structure_get_name(structure);
    if (g_str_equal(name, "video/x-h264"))
        return "h264parse";
    if (g_str_equal(name, "video/x-h265"))
        return "h265parse";

    return nullptr;
}

}  // namespace

GstRemuxPipeline::GstRemuxPipeline(QString name, QObject* parent)
    : QObject(parent)
    , _name(std::move(name))
{
}

GstRemuxPipeline::~GstRemuxPipeline()
{
    stop();
}

bool GstRemuxPipeline::factoryAvailable(const char* factoryName)
{
    if (!factoryName)
        return false;

    GstElementFactory* factory = gst_element_factory_find(factoryName);
    if (!factory)
        return false;

    gst_object_unref(factory);
    return true;
}

bool GstRemuxPipeline::start(const VideoSourceResolver::SourceDescriptor& source,
                             const OutputConfig& output,
                             int bufferMs)
{
    stop();

    if (!source.needsIngestSession() || !output.sinkFactory || !output.muxerFactory)
        return false;

    if (!_build(source, output, bufferMs)) {
        stop();
        return false;
    }
    _installBusHandler();

    const GstStateChangeReturn state = gst_element_set_state(_pipeline.get(), GST_STATE_PLAYING);
    if (state == GST_STATE_CHANGE_FAILURE) {
        qCWarning(GstRemuxPipelineLog) << _name << "failed to start pipeline for" << source.uri;
        stop();
        return false;
    }

    return true;
}

int GstRemuxPipeline::addOutput(const OutputConfig& output)
{
    if (!_pipeline || !_tee)
        return -1;

    return _addOutputBranch(output, true);
}

bool GstRemuxPipeline::stopOutput(int outputId)
{
    if (outputId <= 0 || !_pipeline || !_tee)
        return false;

    auto it = std::find_if(_outputs.begin(), _outputs.end(), [outputId](const std::unique_ptr<OutputBranch>& branch) {
        return branch && branch->id == outputId;
    });
    if (it == _outputs.end())
        return false;

    OutputBranch* branch = it->get();
    QSemaphore eosReachedSink;
    GstPad* sinkPad = gst_element_get_static_pad(branch->sink.get(), "sink");
    const gulong eosProbeId = sinkPad
                                  ? gst_pad_add_probe(
                                        sinkPad,
                                        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                                        [](GstPad*, GstPadProbeInfo* info, gpointer userData) -> GstPadProbeReturn {
                                            if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
                                                GstEvent* event = gst_pad_probe_info_get_event(info);
                                                if (event && GST_EVENT_TYPE(event) == GST_EVENT_EOS)
                                                    static_cast<QSemaphore*>(userData)->release();
                                            }
                                            return GST_PAD_PROBE_OK;
                                        },
                                        &eosReachedSink,
                                        nullptr)
                                  : 0;

    const bool eosSent = gst_element_send_event(branch->queue.get(), gst_event_new_eos());
    if (eosSent && !eosReachedSink.tryAcquire(1, 2000)) {
        qCWarning(GstRemuxPipelineLog) << _name << "timed out waiting for output branch EOS" << outputId;
    }
    if (sinkPad) {
        if (eosProbeId != 0)
            gst_pad_remove_probe(sinkPad, eosProbeId);
        gst_object_unref(sinkPad);
    }

    (void)gst_element_set_state(branch->sink.get(), GST_STATE_NULL);
    (void)gst_element_set_state(branch->mux.get(), GST_STATE_NULL);
    if (branch->parser)
        (void)gst_element_set_state(branch->parser.get(), GST_STATE_NULL);
    (void)gst_element_set_state(branch->queue.get(), GST_STATE_NULL);
    if (branch->teePad) {
        gst_element_release_request_pad(_tee.get(), branch->teePad.get());
        branch->teePad.reset();
    }
    if (branch->parser) {
        gst_bin_remove_many(GST_BIN(_pipeline.get()), branch->queue.get(), branch->parser.get(),
                            branch->mux.get(), branch->sink.get(), nullptr);
    } else {
        gst_bin_remove_many(GST_BIN(_pipeline.get()), branch->queue.get(),
                            branch->mux.get(), branch->sink.get(), nullptr);
    }
    _outputs.erase(it);
    return true;
}

bool GstRemuxPipeline::sendEos()
{
    return _pipeline && gst_element_send_event(_pipeline.get(), gst_event_new_eos());
}

void GstRemuxPipeline::stop()
{
    if (_pipeline) {
        _removeBusHandler();
        (void)gst_element_set_state(_pipeline.get(), GST_STATE_NULL);
        _pipeline.reset();
    }

    _source.reset();
    _tee.reset();
    _outputs.clear();
    _parserFactoryName.clear();
    _nextOutputId = 0;
    _videoLinked = false;
}

GstElement* GstRemuxPipeline::sinkElement() const
{
    if (_outputs.empty())
        return nullptr;
    return _outputs.front()->sink.get();
}

bool GstRemuxPipeline::_build(const VideoSourceResolver::SourceDescriptor& source,
                              const OutputConfig& output,
                              int bufferMs)
{
    _source.reset(GstSourceFactory::createFromSource(source, bufferMs));
    _pipeline.reset(gst_pipeline_new(qPrintable(_name)));
    _tee.reset(gst_element_factory_make("tee", qPrintable(_name + QStringLiteral("-tee"))));

    if (!_pipeline || !_source || !_tee) {
        qCWarning(GstRemuxPipelineLog) << _name << "failed to create source/tee elements for" << source.uri;
        return false;
    }

    gst_bin_add_many(GST_BIN(_pipeline.get()), _source.get(), _tee.get(), nullptr);
    if (_addOutputBranch(output, false) < 0)
        return false;

    auto linkExistingPad = [](GstElement*, GstPad* pad, gpointer userData) -> gboolean {
        auto* self = static_cast<GstRemuxPipeline*>(userData);
        if (self->_linkSourcePad(pad))
            return FALSE;
        return TRUE;
    };
    gst_element_foreach_src_pad(_source.get(), linkExistingPad, this);

    GstPad* teeSink = gst_element_get_static_pad(_tee.get(), "sink");
    const bool linked = teeSink && gst_pad_is_linked(teeSink);
    gst_clear_object(&teeSink);

    if (!linked)
        (void)g_signal_connect(_source.get(), "pad-added", G_CALLBACK(_onSourcePadAdded), this);

    return true;
}

int GstRemuxPipeline::_addOutputBranch(const OutputConfig& output, bool syncState)
{
    if (!output.sinkFactory || !output.muxerFactory || !_pipeline || !_tee)
        return -1;

    const int branchIndex = static_cast<int>(_outputs.size());
    const QString branchName = QStringLiteral("%1-output-%2").arg(_name).arg(branchIndex);

    auto branch = std::make_unique<OutputBranch>();
    branch->queue.reset(gst_element_factory_make("queue", qPrintable(branchName + QStringLiteral("-queue"))));
    if (!_parserFactoryName.isEmpty())
        branch->parser.reset(gst_element_factory_make(_parserFactoryName.constData(),
                                                      qPrintable(branchName + QStringLiteral("-parser"))));
    branch->mux.reset(gst_element_factory_make(output.muxerFactory, qPrintable(branchName + QStringLiteral("-mux"))));
    branch->sink.reset(gst_element_factory_make(output.sinkFactory, output.sinkName.constData()));

    if (!branch->queue || (!_parserFactoryName.isEmpty() && !branch->parser) || !branch->mux || !branch->sink) {
        qCWarning(GstRemuxPipelineLog) << _name << "failed to create output branch"
                                       << output.muxerFactory << output.sinkFactory;
        return -1;
    }

    if (output.leakyQueue) {
        g_object_set(branch->queue.get(), "leaky", 2, "max-size-buffers", 2,
                     "max-size-time", 0, "max-size-bytes", 0, nullptr);
    } else {
        g_object_set(branch->queue.get(), "max-size-time", 0, "max-size-bytes", 0, nullptr);
    }

    if (output.configureSink)
        output.configureSink(branch->sink.get());

    if (branch->parser) {
        g_object_set(branch->parser.get(), "config-interval", -1, nullptr);
        gst_bin_add_many(GST_BIN(_pipeline.get()), branch->queue.get(), branch->parser.get(),
                         branch->mux.get(), branch->sink.get(), nullptr);
    } else {
        gst_bin_add_many(GST_BIN(_pipeline.get()), branch->queue.get(), branch->mux.get(), branch->sink.get(), nullptr);
    }

    const bool linked = branch->parser
                            ? gst_element_link_many(branch->queue.get(), branch->parser.get(),
                                                    branch->mux.get(), branch->sink.get(), nullptr)
                            : (gst_element_link(branch->queue.get(), branch->mux.get()) &&
                               gst_element_link(branch->mux.get(), branch->sink.get()));
    if (!linked) {
        qCWarning(GstRemuxPipelineLog) << _name << "failed to link output branch";
        if (branch->parser) {
            gst_bin_remove_many(GST_BIN(_pipeline.get()), branch->queue.get(), branch->parser.get(),
                                branch->mux.get(), branch->sink.get(), nullptr);
        } else {
            gst_bin_remove_many(GST_BIN(_pipeline.get()), branch->queue.get(),
                                branch->mux.get(), branch->sink.get(), nullptr);
        }
        return -1;
    }

    GstPad* queueSink = gst_element_get_static_pad(branch->queue.get(), "sink");
    GstPad* teeSrc = gst_element_request_pad_simple(_tee.get(), "src_%u");
    if (!queueSink || !teeSrc) {
        gst_clear_object(&queueSink);
        gst_clear_object(&teeSrc);
        gst_bin_remove_many(GST_BIN(_pipeline.get()), branch->queue.get(), branch->mux.get(), branch->sink.get(), nullptr);
        return -1;
    }

    const GstPadLinkReturn ret = gst_pad_link(teeSrc, queueSink);
    gst_object_unref(queueSink);
    if (GST_PAD_LINK_FAILED(ret)) {
        qCWarning(GstRemuxPipelineLog) << _name << "failed to link tee to output branch:" << ret;
        gst_element_release_request_pad(_tee.get(), teeSrc);
        gst_object_unref(teeSrc);
        gst_bin_remove_many(GST_BIN(_pipeline.get()), branch->queue.get(), branch->mux.get(), branch->sink.get(), nullptr);
        return -1;
    }
    branch->teePad.reset(teeSrc);

    if (syncState) {
        (void)gst_element_sync_state_with_parent(branch->queue.get());
        if (branch->parser)
            (void)gst_element_sync_state_with_parent(branch->parser.get());
        (void)gst_element_sync_state_with_parent(branch->mux.get());
        (void)gst_element_sync_state_with_parent(branch->sink.get());
    }

    const int outputId = _nextOutputId++;
    branch->id = outputId;
    _outputs.push_back(std::move(branch));
    return outputId;
}

bool GstRemuxPipeline::_linkSourcePad(GstPad* pad)
{
    if (!pad)
        return false;

    GstCaps* caps = padCaps(pad);
    const GstStructure* videoStructure = firstVideoStructure(caps);
    if (!videoStructure) {
        gst_clear_caps(&caps);
        return false;
    }

    const char* parserFactoryName = parserFactoryForCaps(caps);
    if (!parserFactoryName) {
        qCDebug(GstRemuxPipelineLog) << _name << "ignoring unsupported video pad caps:" << describeCaps(caps);
        gst_clear_caps(&caps);
        return false;
    }

    GstPad* teeSink = gst_element_get_static_pad(_tee.get(), "sink");
    if (!teeSink) {
        gst_clear_caps(&caps);
        return false;
    }

    if (gst_pad_is_linked(teeSink)) {
        gst_object_unref(teeSink);
        gst_clear_caps(&caps);
        return false;
    }
    gst_object_unref(teeSink);

    _parserFactoryName = parserFactoryName;
    if (!_ensureBranchParsers(parserFactoryName)) {
        gst_clear_caps(&caps);
        return false;
    }

    GstPad* sinkPad = gst_element_get_static_pad(_tee.get(), "sink");
    if (!sinkPad) {
        gst_clear_caps(&caps);
        return false;
    }

    const GstPadLinkReturn ret = gst_pad_link(pad, sinkPad);
    gst_object_unref(sinkPad);
    if (GST_PAD_LINK_FAILED(ret)) {
        qCWarning(GstRemuxPipelineLog) << _name << "failed to link source pad:"
                                       << ret << "caps:" << describeCaps(caps);
        gst_clear_caps(&caps);
        return false;
    }

    gst_clear_caps(&caps);
    _videoLinked = true;
    emit videoPadLinked();
    return true;
}

bool GstRemuxPipeline::_ensureBranchParsers(const char* parserFactoryName)
{
    if (!parserFactoryName || !_pipeline)
        return false;

    for (const std::unique_ptr<OutputBranch>& branch : _outputs) {
        if (!branch || branch->parser)
            continue;

        const QString parserName = QStringLiteral("%1-output-%2-parser").arg(_name).arg(branch->id);
        branch->parser.reset(gst_element_factory_make(parserFactoryName, qPrintable(parserName)));
        if (!branch->parser) {
            const QString message = QStringLiteral("Missing GStreamer plugin: %1").arg(QLatin1String(parserFactoryName));
            qCWarning(GstRemuxPipelineLog) << _name << message;
            emit errorOccurred(VideoReceiver::ErrorCategory::MissingPlugin, message);
            return false;
        }

        g_object_set(branch->parser.get(), "config-interval", -1, nullptr);
        gst_element_unlink(branch->queue.get(), branch->mux.get());
        gst_bin_add(GST_BIN(_pipeline.get()), branch->parser.get());
        if (!gst_element_link_many(branch->queue.get(), branch->parser.get(), branch->mux.get(), nullptr)) {
            qCWarning(GstRemuxPipelineLog) << _name << "failed to insert branch parser" << parserFactoryName;
            gst_bin_remove(GST_BIN(_pipeline.get()), branch->parser.get());
            branch->parser.reset();
            return false;
        }
        (void)gst_element_sync_state_with_parent(branch->parser.get());
    }

    return true;
}

void GstRemuxPipeline::_installBusHandler()
{
    if (!_pipeline)
        return;

    _bus.reset(gst_element_get_bus(_pipeline.get()));
    if (_bus)
        gst_bus_set_sync_handler(_bus.get(), _onBusMessage, this, nullptr);
}

void GstRemuxPipeline::_removeBusHandler()
{
    if (_bus) {
        gst_bus_set_sync_handler(_bus.get(), nullptr, nullptr, nullptr);
        _bus.reset();
    }
}

void GstRemuxPipeline::_handleBusMessage(GstMessage* message)
{
    if (!message)
        return;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* error = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &error, &debug);
            QString text = error ? QString::fromUtf8(error->message) : QStringLiteral("Unknown GStreamer error");
            if (debug && debug[0] != '\0')
                text += QStringLiteral(" (%1)").arg(QString::fromUtf8(debug));
            g_clear_error(&error);
            g_clear_pointer(&debug, g_free);
            qCWarning(GstRemuxPipelineLog) << _name << "error:" << text;
            emit errorOccurred(VideoReceiver::ErrorCategory::Fatal, text);
            break;
        }

        case GST_MESSAGE_ELEMENT:
            if (gst_is_missing_plugin_message(message)) {
                gchar* description = gst_missing_plugin_message_get_description(message);
                const QString text = description
                                         ? QStringLiteral("Missing GStreamer plugin: %1").arg(QString::fromUtf8(description))
                                         : QStringLiteral("Missing GStreamer plugin");
                g_clear_pointer(&description, g_free);
                qCWarning(GstRemuxPipelineLog) << _name << text;
                emit errorOccurred(VideoReceiver::ErrorCategory::MissingPlugin, text);
            }
            break;

        case GST_MESSAGE_EOS:
            if (_pipeline && GST_MESSAGE_SRC(message) == GST_OBJECT(_pipeline.get())) {
                qCDebug(GstRemuxPipelineLog) << _name << "reached EOS";
                emit endOfStream();
            }
            break;

        case GST_MESSAGE_STATE_CHANGED:
            if (_pipeline && GST_MESSAGE_SRC(message) == GST_OBJECT(_pipeline.get())) {
                GstState oldState = GST_STATE_NULL;
                GstState newState = GST_STATE_NULL;
                GstState pendingState = GST_STATE_NULL;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
                qCDebug(GstRemuxPipelineLog) << _name << "state"
                                             << gst_element_state_get_name(oldState) << "->"
                                             << gst_element_state_get_name(newState)
                                             << "pending" << gst_element_state_get_name(pendingState);
            }
            break;

        default:
            break;
    }
}

void GstRemuxPipeline::_handleSourcePadAdded(GstPad* pad)
{
    (void)_linkSourcePad(pad);
}

void GstRemuxPipeline::_onSourcePadAdded(GstElement* /*source*/, GstPad* pad, gpointer userData)
{
    static_cast<GstRemuxPipeline*>(userData)->_handleSourcePadAdded(pad);
}

GstBusSyncReply GstRemuxPipeline::_onBusMessage(GstBus* /*bus*/, GstMessage* message, gpointer userData)
{
    auto* self = static_cast<GstRemuxPipeline*>(userData);
    if (!self || !message)
        return GST_BUS_PASS;

    gst_message_ref(message);
    auto messageRef = std::shared_ptr<GstMessage>(message, [](GstMessage* msg) {
        gst_message_unref(msg);
    });
    QMetaObject::invokeMethod(self, [self, messageRef]() {
        self->_handleBusMessage(messageRef.get());
    }, Qt::QueuedConnection);

    return GST_BUS_PASS;
}

#ifdef QGC_UNITTEST_BUILD
void GstRemuxPipeline::handleBusMessageForTest(GstMessage* message)
{
    _handleBusMessage(message);
}
#endif
