#include "TeeBranchHead.h"

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"

namespace {

GQuark flowErrorPostedQuark()
{
    static const GQuark quark = g_quark_from_static_string("qgc-flow-error-posted");
    return quark;
}

/// A fatal @p result of a push into @p pad is posted, once, as an ERROR from @p parent, and becomes success. Not
/// linked, flushing and EOS are not failures of what the pad leads to, and pass.
GstFlowReturn absorbFlowError(GstPad* pad, GstObject* parent, GstFlowReturn result)
{
    if (result >= GST_FLOW_EOS) {
        return result;
    }
    // The pad's stream lock serializes the pushes.
    if (!g_object_get_qdata(G_OBJECT(pad), flowErrorPostedQuark())) {
        g_object_set_qdata(G_OBJECT(pad), flowErrorPostedQuark(), GINT_TO_POINTER(TRUE));
        GST_ELEMENT_FLOW_ERROR(GST_ELEMENT_CAST(parent), result);
    }
    return GST_FLOW_OK;
}

GstFlowReturn guardedChain(GstPad* pad, GstObject* parent, GstBuffer* buffer)
{
    return absorbFlowError(pad, parent, gst_proxy_pad_chain_default(pad, parent, buffer));
}

GstFlowReturn guardedChainList(GstPad* pad, GstObject* parent, GstBufferList* list)
{
    return absorbFlowError(pad, parent, gst_proxy_pad_chain_list_default(pad, parent, list));
}

}  // namespace

void TeeBranchHead::absorbFlowErrors(GstElement* part)
{
    const GStreamer::GstPadPtr sink = GStreamer::adoptPad(gst_element_get_static_pad(part, "sink"));
    // Wrapping the ghost pad's default functions is only right for a pad that still has them.
    if (!sink || !GST_IS_GHOST_PAD(sink.get()) || (GST_PAD_CHAINFUNC(sink.get()) != gst_proxy_pad_chain_default) ||
        (GST_PAD_CHAINLISTFUNC(sink.get()) != gst_proxy_pad_chain_list_default)) {
        qCWarning(GstVideoReceiverLog) << "Unable to keep the flow errors of" << GST_OBJECT_NAME(part)
                                       << "out of the tee";
        return;
    }
    gst_pad_set_chain_function(sink.get(), guardedChain);
    gst_pad_set_chain_list_function(sink.get(), guardedChainList);
}

bool TeeBranchHead::build(GstElement* pipeline, GstElement* tee, GStreamer::PipelineBranch branch, const char* what)
{
    reset();

    GStreamer::GstElementPtr queue = GStreamer::makeElement("queue");
    GStreamer::GstElementPtr valve = GStreamer::makeElement("valve");
    if (!queue || !valve) {
        qCWarning(GstVideoReceiverLog) << "Failed to create the queue and valve of the" << what << "branch";
        return false;
    }
    g_object_set(valve.get(), "drop", TRUE, nullptr);
    GStreamer::markBranch(queue.get(), branch);
    GStreamer::markBranch(valve.get(), branch);

    GstElement* const binQueue = GStreamer::addToBin(pipeline, std::move(queue), "queue");
    GstElement* const binValve = GStreamer::addToBin(pipeline, std::move(valve), "valve");
    if (!binQueue || !binValve || !gst_element_link_many(tee, binQueue, binValve, nullptr)) {
        qCWarning(GstVideoReceiverLog) << "Unable to link the" << what << "branch";
        return false;
    }
    _queue = binQueue;
    _valve = binValve;
    return true;
}

void TeeBranchHead::reset()
{
    _overrun.disconnect();
    _queue = nullptr;
    _valve = nullptr;
}

void TeeBranchHead::onOverrun(GCallback callback, gpointer data)
{
    if (_queue) {
        (void) _overrun.connect(_queue, "overrun", callback, data);
    }
}

void TeeBranchHead::open()
{
    if (!_valve) {
        return;
    }
    g_object_set(_valve, "drop", FALSE, nullptr);
    requestKeyframe();
}

void TeeBranchHead::close()
{
    if (_valve) {
        g_object_set(_valve, "drop", TRUE, nullptr);
    }
}

void TeeBranchHead::requestKeyframe() const
{
    if (_valve) {
        const GStreamer::GstPadPtr valveSink = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "sink"));
        GStreamer::requestKeyframe(valveSink.get());
    }
}

bool TeeBranchHead::contains(GstObject* object) const
{
    return (object == GST_OBJECT_CAST(_queue)) || (object == GST_OBJECT_CAST(_valve));
}
