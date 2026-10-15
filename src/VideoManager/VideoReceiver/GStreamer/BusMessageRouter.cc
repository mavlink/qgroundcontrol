#include "BusMessageRouter.h"

#include <gst/gst.h>

#include "GstVideoReceiver.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCQVideoSinkController.h"

namespace {

void logBusWarning(GstMessage* message)
{
    // GStreamer posts WARNING for caps mismatches, decoder fallbacks, clock drift — surfacing keeps these
    // visible without escalating to STATUS_FAIL.
    gchar* debug = nullptr;
    GError* error = nullptr;
    gst_message_parse_warning(message, &error, &debug);
    qCWarning(GstVideoReceiverLog) << "GStreamer warning:" << (error ? error->message : "(no message)")
                                   << "debug:" << (debug ? debug : "(none)");
    g_clear_error(&error);
    g_clear_pointer(&debug, g_free);
}

/// Selecting only video keeps decodebin3 from instantiating audio decoder branches. Done synchronously so the
/// selection is in place before decodebin3 falls back to its default one.
void selectVideoStreams(GstMessage* message)
{
    GstStreamCollection* collection = nullptr;
    gst_message_parse_stream_collection(message, &collection);
    if (!collection) {
        return;
    }
    GList* selectedIds = nullptr;
    const guint nStreams = gst_stream_collection_get_size(collection);
    for (guint i = 0; i < nStreams; ++i) {
        GstStream* stream = gst_stream_collection_get_stream(collection, i);
        if (gst_stream_get_stream_type(stream) & GST_STREAM_TYPE_VIDEO) {
            selectedIds = g_list_append(selectedIds, g_strdup(gst_stream_get_stream_id(stream)));
        }
    }
    if (selectedIds) {
        (void) gst_element_send_event(GST_ELEMENT(GST_MESSAGE_SRC(message)), gst_event_new_select_streams(selectedIds));
        g_list_free_full(selectedIds, g_free);
    }
    gst_object_unref(collection);
}

bool isRecoverableH265PaciError(GstMessage* msg, const GError* error, const gchar* debug)
{
    if (!msg || !error || (error->domain != GST_STREAM_ERROR) || (error->code != GST_STREAM_ERROR_FORMAT) || !debug ||
        !g_strrstr(debug, "NAL unit type 50 not supported yet")) {
        return false;
    }

    GstObject* src = GST_MESSAGE_SRC(msg);
    if (!src || !GST_IS_ELEMENT(src)) {
        return false;
    }

    GstElementFactory* factory = gst_element_get_factory(GST_ELEMENT(src));
    return factory && (g_strcmp0(GST_OBJECT_NAME(factory), "rtph265depay") == 0);
}

}  // namespace

BusMessageRouter::BusMessageRouter(GstVideoReceiver& receiver)
    : _receiver(receiver)
{}

void BusMessageRouter::attach(GstElement* pipeline)
{
    _bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    if (!_bus) {
        qCCritical(GstVideoReceiverLog) << "gst_pipeline_get_bus() failed";
        return;
    }
    gst_bus_set_sync_handler(_bus, _onSyncMessage, this, [](gpointer router) {
        static_cast<BusMessageRouter*>(router)->_handlerReleased.release();
    });
}

void BusMessageRouter::detach()
{
    if (!_bus) {
        return;
    }

    gst_bus_set_sync_handler(_bus, nullptr, nullptr, nullptr);
    gst_clear_object(&_bus);
    // GStreamer refcounts the handler per post and releases it once the last in-flight call has returned.
    _handlerReleased.acquire();
}

GstBusSyncReply BusMessageRouter::_onSyncMessage(GstBus* bus, GstMessage* message, gpointer user_data)
{
    // The context bridges answer NEED_CONTEXT synchronously and consume (unref) the requests they handle.
    if (HwBuffers::onBusSyncMessage(bus, message, nullptr) == GST_BUS_DROP) {
        return GST_BUS_DROP;
    }

    auto* self = static_cast<BusMessageRouter*>(user_data);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            self->_onError(message);
            break;
        case GST_MESSAGE_WARNING:
            logBusWarning(message);
            break;
        case GST_MESSAGE_EOS:
            self->_receiver._recordingBranch.onBusEos();
            break;
        case GST_MESSAGE_ELEMENT:
            self->_onElement(message);
            break;
        case GST_MESSAGE_STREAM_COLLECTION:
            selectVideoStreams(message);
            break;
        case GST_MESSAGE_QOS:
            self->_receiver._decodingBranch.onBusQos(message);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            self->_onStateChanged(message);
            break;
        case GST_MESSAGE_LATENCY:
            self->_onLatency();
            break;
        default:
            break;
    }

    // Every message is handled above and nothing pops this bus; queueing it would grow the bus for the
    // pipeline's lifetime. A sync handler that drops a message owns it.
    gst_message_unref(message);
    return GST_BUS_DROP;
}

void BusMessageRouter::_onError(GstMessage* message)
{
    gchar* debug = nullptr;
    GError* error = nullptr;
    gst_message_parse_error(message, &error, &debug);
    const bool recoverableH265PaciError = isRecoverableH265PaciError(message, error, debug);
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");
    if (debug) {
        qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug;
    }
    g_clear_error(&error);
    g_clear_pointer(&debug, g_free);

    if (recoverableH265PaciError) {
        qCWarning(GstVideoReceiverLog) << "Ignoring unsupported H.265 RTP PACI packet from rtph265depay:" << text;
        return;
    }

    if (_receiver._recordingBranch.onBusError(message, text)) {
        return;
    }

    qCWarning(GstVideoReceiverLog) << "GStreamer error:" << text;
    // Drops cached GPU devices on a device-loss error.
    HwBuffers::dispatchBusMessage(message);
    // A failing pipeline usually posts several ERRORs; the first one's teardown makes the rest stale.
    GstVideoReceiver* const receiver = &_receiver;
    receiver->_runOnWorkerForPipeline([receiver]() { receiver->_onPipelineError(); });
}

void BusMessageRouter::_onElement(GstMessage* message)
{
    const GstStructure* structure = gst_message_get_structure(message);
    if (!structure) {
        return;
    }

    if (gst_structure_has_name(structure, "qgc-caps-info")) {
        _receiver._decodingBranch.onBusCapsInfo(message);
        return;
    }

    _receiver._recordingBranch.onBusElement(message);
}

void BusMessageRouter::_onStateChanged(GstMessage* message)
{
    // Only our pipeline is a GstPipeline on its own bus.
    if (!GST_IS_PIPELINE(GST_MESSAGE_SRC(message))) {
        return;
    }
    GstState oldState = GST_STATE_NULL, newState = GST_STATE_NULL;
    gst_message_parse_state_changed(message, &oldState, &newState, nullptr);
    if ((newState == GST_STATE_PLAYING) && (oldState != GST_STATE_PLAYING)) {
        GstVideoReceiver* const receiver = &_receiver;
        receiver->_runOnWorkerForPipeline([receiver]() { receiver->_onPipelinePlaying(); });
    }
}

void BusMessageRouter::_onLatency()
{
    GstVideoReceiver* const receiver = &_receiver;
    receiver->_runOnWorkerForPipeline(
        [receiver]() { (void) gst_bin_recalculate_latency(GST_BIN(receiver->_pipeline)); });
    // Re-prime sink-side latency tracking after the pipeline recalculation (e.g. RTSP
    // jitter-buffer reconfigure). Controllers live on the GUI thread; hop there to query.
    QMetaObject::invokeMethod(
        receiver,
        [receiver]() {
            for (auto* c : QGCQVideoSinkController::controllersOf(receiver)) {
                c->refreshLatency();
            }
        },
        Qt::QueuedConnection);
}
