#include "GstPipelineController.h"

#include "GstPipelineFactory.h"
#include "QGCLoggingCategory.h"

#include <utility>

QGC_LOGGING_CATEGORY(GstPipelineControllerLog, "Video.GstPipelineController")

bool GstPipelineController::build(const VideoSourceResolver::SourceDescriptor& source,
                                  int bufferMs,
                                  bool forceSwDecoders)
{
    clear();

    auto layout = GstPipelineFactory::build(source, bufferMs, forceSwDecoders);
    if (!layout) {
        qCCritical(GstPipelineControllerLog) << "Failed to build pipeline";
        return false;
    }

    _pipeline = std::move(layout->pipeline);
    _source = std::move(layout->source);
    _tee = std::move(layout->tee);
    _decoderValve = std::move(layout->decoderValve);
    _recorderValve = std::move(layout->recorderValve);
    return true;
}

GstNonFloatingPtr<GstBus> GstPipelineController::bus() const
{
    return _pipeline ? GstNonFloatingPtr<GstBus>(gst_pipeline_get_bus(GST_PIPELINE(_pipeline.get())))
                     : GstNonFloatingPtr<GstBus>();
}

GstNonFloatingPtr<GstPad> GstPipelineController::firstSourcePad() const
{
    return _source ? GStreamer::gstFirstSrcPad(_source.get()) : GstNonFloatingPtr<GstPad>();
}

void GstPipelineController::connectBus(GCallback callback, gpointer data)
{
    auto pipelineBus = bus();
    if (!pipelineBus) {
        qCCritical(GstPipelineControllerLog) << "gst_pipeline_get_bus() failed";
        return;
    }

    gst_bus_enable_sync_message_emission(pipelineBus.get());
    (void)g_signal_connect(pipelineBus.get(), "sync-message", callback, data);
}

void GstPipelineController::disconnectBus(gpointer data)
{
    auto pipelineBus = bus();
    if (!pipelineBus)
        return;

    gst_bus_disable_sync_message_emission(pipelineBus.get());
    (void)g_signal_handlers_disconnect_by_data(pipelineBus.get(), data);
}

void GstPipelineController::installTeeProbe(GstPadProbeCallback callback, gpointer data)
{
    _teeProbeGuard = gstAddPadProbe(_tee.get(), "sink", GST_PAD_PROBE_TYPE_BUFFER, callback, data);
}

void GstPipelineController::removeTeeProbe()
{
    _teeProbeGuard.remove();
}

bool GstPipelineController::setPlaying()
{
    return _pipeline && gst_element_set_state(_pipeline.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
}

void GstPipelineController::setNull()
{
    if (_pipeline)
        (void)gst_element_set_state(_pipeline.get(), GST_STATE_NULL);
}

void GstPipelineController::clear()
{
    removeTeeProbe();
    _pipeline = {};
    _recorderValve = {};
    _decoderValve = {};
    _tee = {};
    _source = {};
}

void GstPipelineController::drainEos(GstBus* pipelineBus, gpointer signalData)
{
    gst_bus_disable_sync_message_emission(pipelineBus);
    (void)g_signal_handlers_disconnect_by_data(pipelineBus, signalData);

    gboolean recordingValveClosed = TRUE;
    g_object_get(_recorderValve.get(), "drop", &recordingValveClosed, nullptr);

    if (recordingValveClosed)
        return;

    (void)gst_element_send_event(_pipeline.get(), gst_event_new_eos());

    // Block up to 1s for EOS - the muxer needs to finalize the file. If this
    // times out, fragmented mp4/mov output may still be playable.
    GstMessagePtr msg(gst_bus_timed_pop_filtered(pipelineBus, 1 * GST_SECOND,
                                                 static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR)));
    if (msg) {
        if (GST_MESSAGE_TYPE(msg.get()) == GST_MESSAGE_EOS)
            qCDebug(GstPipelineControllerLog) << "End of stream received";
        else if (GST_MESSAGE_TYPE(msg.get()) == GST_MESSAGE_ERROR)
            qCCritical(GstPipelineControllerLog) << "Error stopping pipeline";
    } else {
        qCWarning(GstPipelineControllerLog) << "EOS drain timed out after 1s - forcing pipeline stop";
    }
}

void GstPipelineController::dumpGraph(const char* name) const
{
    if (_pipeline)
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline.get()), GST_DEBUG_GRAPH_SHOW_ALL, name);
}
