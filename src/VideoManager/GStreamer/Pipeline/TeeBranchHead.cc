#include "TeeBranchHead.h"

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"

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
