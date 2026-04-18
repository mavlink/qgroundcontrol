#include "GstPipelineFactory.h"

#include <gst/gst.h>

#include "GstSourceFactory.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstPipelineFactoryLog, "Video.GstPipelineFactory")

namespace GstPipelineFactory {

std::optional<GstPipelineLayout> build(const QString& uri, int bufferMs, bool forceSwDecoders)
{
    return build(VideoSourceResolver::describeUri(uri), bufferMs, forceSwDecoders);
}

std::optional<GstPipelineLayout> build(const VideoSourceResolver::SourceDescriptor& sourceDescriptor,
                                       int bufferMs,
                                       bool forceSwDecoders)
{
    GstObjectPtr<GstElement> tee(gst_element_factory_make("tee", nullptr));
    GstObjectPtr<GstElement> decoderQueue(gst_element_factory_make("queue", nullptr));
    GstObjectPtr<GstElement> decoderValve(gst_element_factory_make("valve", nullptr));
    GstObjectPtr<GstElement> recorderQueue(gst_element_factory_make("queue", nullptr));
    GstObjectPtr<GstElement> recorderValve(gst_element_factory_make("valve", nullptr));

    if (!tee || !decoderQueue || !decoderValve || !recorderQueue || !recorderValve) {
        qCCritical(GstPipelineFactoryLog) << "Failed to create pipeline elements";
        return std::nullopt;
    }

    for (auto* q : {decoderQueue.get(), recorderQueue.get()}) {
        g_object_set(q, "max-size-buffers", 2u, "max-size-bytes", 0u, "max-size-time", guint64(0), "leaky",
                     2,  // downstream — drop oldest
                     nullptr);
    }

    // Both valves start closed — without this the tee would post GST_FLOW_NOT_LINKED
    // on the first buffer and stall dynamic-pad sources (RTSP/WHEP) at startup.
    g_object_set(tee.get(), "allow-not-linked", TRUE, nullptr);

    g_object_set(decoderValve.get(), "drop", TRUE, nullptr);
    g_object_set(recorderValve.get(), "drop", TRUE, nullptr);

    GstObjectPtr<GstElement> pipeline(gst_pipeline_new("receiver"));
    if (!pipeline) {
        qCCritical(GstPipelineFactoryLog) << "gst_pipeline_new() failed";
        return std::nullopt;
    }

    g_object_set(pipeline.get(), "message-forward", TRUE, nullptr);

    GstObjectPtr<GstElement> source(GstSourceFactory::createFromSource(sourceDescriptor, bufferMs, forceSwDecoders));
    if (!source) {
        qCCritical(GstPipelineFactoryLog) << "Failed to create source for" << sourceDescriptor.uri;
        return std::nullopt;
    }

    gst_bin_add_many(GST_BIN(pipeline.get()), source.get(), tee.get(), decoderQueue.get(), decoderValve.get(),
                     recorderQueue.get(), recorderValve.get(), nullptr);

    auto cleanupPipeline = [&]() {
        (void)gst_element_set_state(pipeline.get(), GST_STATE_NULL);
        pipeline = {};
    };

    if (!gst_element_link_many(tee.get(), decoderQueue.get(), decoderValve.get(), nullptr)) {
        qCCritical(GstPipelineFactoryLog) << "Failed to link decoder branch (tee→queue→valve)";
        cleanupPipeline();
        return std::nullopt;
    }

    if (!gst_element_link_many(tee.get(), recorderQueue.get(), recorderValve.get(), nullptr)) {
        qCCritical(GstPipelineFactoryLog) << "Failed to link recorder branch (tee→queue→valve)";
        cleanupPipeline();
        return std::nullopt;
    }

    return GstPipelineLayout{
        std::move(pipeline), std::move(source), std::move(tee), std::move(decoderValve), std::move(recorderValve),
    };
}

}  // namespace GstPipelineFactory
