#include "GStreamerHelpers.h"

#include <algorithm>
#include <gst/rtsp/gstrtspurl.h>
#include <gst/video/video-event.h>

#include <QtCore/QByteArray>
#include <QtCore/QLatin1String>
#include <QtCore/QString>

#include "GstScoped.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerHelpersLog, "Video.GStreamer.GStreamerHelpers")

namespace GStreamer {

bool isValidRtspUri(const gchar* uri_str)
{
    if (!uri_str) {
        return false;
    }

    if (!gst_uri_is_valid(uri_str)) {
        return false;
    }

    GstRTSPUrl* url = nullptr;
    const GstRTSPResult res = gst_rtsp_url_parse(uri_str, &url);
    if ((res != GST_RTSP_OK) || !url) {
        if (url) {
            gst_rtsp_url_free(url);
        }
        return false;
    }

    const bool hasHost = (url->host && url->host[0] != '\0');
    gst_rtsp_url_free(url);
    return hasHost;
}

namespace {

void removeAndStop(GstElement* element, bool flush)
{
    const GstObjectPtr parent = element ? adoptObject(gst_object_get_parent(GST_OBJECT(element))) : nullptr;
    if (!parent) {
        return;
    }

    // gst_bin_remove drops the bin's ref, which may be the last one, before the element reaches NULL.
    const GstElementPtr keepAlive = adoptElement(GST_ELEMENT(gst_object_ref(element)));
    (void) gst_bin_remove(GST_BIN(parent.get()), element);
    if (flush) {
        (void) gst_element_foreach_sink_pad(
            element,
            [](GstElement* /*element*/, GstPad* pad, gpointer /*userData*/) -> gboolean {
                (void) gst_pad_send_event(pad, gst_event_new_flush_start());
                return TRUE;
            },
            nullptr);
    }
    // Synchronous: gst_element_change_state() commits an ASYNC return at once when the target is READY or NULL, so
    // the streaming threads have stopped when this returns.
    (void) gst_element_set_state(element, GST_STATE_NULL);
}

}  // namespace

void disposeChild(GstElement* element)
{
    removeAndStop(element, false);
}

void flushAndDisposeChild(GstElement* element)
{
    removeAndStop(element, true);
}

GstElement* addToBin(GstElement* bin, GstElementPtr element, const char* what)
{
    if (!element || g_object_is_floating(element.get())) {
        qCCritical(GStreamerHelpersLog) << "Missing or floating element for" << what;
        return nullptr;
    }
    GstElement* const added = element.release();
    const bool ok = gst_bin_add(GST_BIN(bin), added);
    // The bin took its own reference; drop ours.
    gst_object_unref(added);
    if (!ok) {
        qCWarning(GStreamerHelpersLog) << "gst_bin_add() failed for" << what;
        return nullptr;
    }
    return added;
}

TeeBranch addTeeBranch(GstElement* pipeline, GstElement* tee, const char* what)
{
    GstElementPtr queue = makeElement("queue");
    GstElementPtr valve = makeElement("valve");
    if (!queue || !valve) {
        qCWarning(GStreamerHelpersLog) << "Failed to create the queue and valve of the" << what << "branch";
        return {};
    }
    g_object_set(valve.get(), "drop", TRUE, nullptr);

    const TeeBranch branch{.queue = addToBin(pipeline, std::move(queue), "queue"),
                           .valve = addToBin(pipeline, std::move(valve), "valve")};
    if (!branch.queue || !branch.valve || !gst_element_link_many(tee, branch.queue, branch.valve, nullptr)) {
        qCWarning(GStreamerHelpersLog) << "Unable to link the" << what << "branch";
        return {};
    }
    return branch;
}

void requestKeyframe(GstPad* sinkPad)
{
    if (!sinkPad) {
        return;
    }
    (void) gst_pad_push_event(sinkPad, gst_video_event_new_upstream_force_key_unit(GST_CLOCK_TIME_NONE, TRUE, 0));
}

void sendEventAsync(GstPad* pad, GstEvent* event)
{
    struct PendingEvent
    {
        GstPadPtr pad;
        GstEventPtr event;
    };

    GstElementPtr parent = pad ? adoptElement(gst_pad_get_parent_element(pad)) : nullptr;
    if (!parent) {
        gst_event_unref(event);
        return;
    }
    gst_element_call_async(
        parent.get(),
        [](GstElement* /*element*/, gpointer data) {
            auto* const pending = static_cast<PendingEvent*>(data);
            (void) gst_pad_send_event(pending->pad.get(), pending->event.release());
        },
        new PendingEvent{.pad = adoptPad(GST_PAD(gst_object_ref(pad))), .event = adoptEvent(event)},
        [](gpointer data) { delete static_cast<PendingEvent*>(data); });
}

GstPadPtr firstSrcPad(GstElement* element)
{
    GstPad* first = nullptr;
    // Handles iterator resync itself, unlike a bare gst_iterator_next loop.
    (void) gst_element_foreach_src_pad(
        element,
        [](GstElement* /*element*/, GstPad* pad, gpointer userData) -> gboolean {
            *static_cast<GstPad**>(userData) = GST_PAD(gst_object_ref(pad));
            return FALSE;
        },
        &first);
    return adoptPad(first);
}

std::function<void()> holdPadUntilLinked(GstPad* pad, std::function<void(GstPad* pad)> link)
{
    // Returning OK from a blocking probe keeps the pad blocked until the probe is removed.
    const gulong blockId = gst_pad_add_probe(
        pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        [](GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer /*data*/) { return GST_PAD_PROBE_OK; }, nullptr,
        nullptr);
    return [blockId, heldPad = shareObject(pad), link = std::move(link)]() {
        link(heldPad.get());
        gst_pad_remove_probe(heldPad.get(), blockId);
    };
}

void forEachPlugin(GstRegistry* registry, const std::function<void(GstPlugin*)>& visitor)
{
    if (!registry || !visitor)
        return;
    GList* plugins = gst_registry_get_plugin_list(registry);
    for (GList* node = plugins; node != nullptr; node = node->next) {
        GstPlugin* plugin = static_cast<GstPlugin*>(node->data);
        if (plugin)
            visitor(plugin);
    }
    gst_plugin_list_free(plugins);
}

}  // namespace GStreamer
