#include "GstContextBridgeCommon.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>
#include <glib.h>

#include "GstContextBridgeRegistry.h"

namespace GstContextBridge {

const char* matchContextType(const BridgeVTable& vt, const char* type)
{
    if (!type) {
        return nullptr;
    }
    for (int i = 0; i < vt.contextTypeCount; ++i) {
        if (g_strcmp0(type, vt.contextTypes[i]) == 0) {
            return vt.contextTypes[i];
        }
    }
    return nullptr;
}

namespace {

// Shared snapshot path for both NEED_CONTEXT and GST_QUERY_CONTEXT: prime + ref the object under the bridge mutex, then
// build the GstContext with the lock released. The built context holds its own ref on the object, so our snapshot ref
// is dropped here. Returns the context, or null when not primed / object unavailable / build failed.
GstContext* snapshotContext(const BridgeVTable& vt, void* user, const char* matched)
{
    GstObject* object = nullptr;
    {
        QMutexLocker lock(&vt.mutex(user));
        if (!vt.primeLocked(user)) {
            return nullptr;
        }
        object = vt.refObject(user, matched);
    }
    if (!object) {
        return nullptr;
    }
    GstContext* ctx = vt.buildContext(user, matched, object);
    gst_object_unref(object);
    return ctx;
}

}  // namespace

GstBusSyncReply handleSyncMessage(const BridgeVTable& vt, void* user, GstMessage* message)
{
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_NEED_CONTEXT) {
        return GST_BUS_PASS;
    }
    const gchar* contextType = nullptr;
    if (!gst_message_parse_context_type(message, &contextType) || !contextType) {
        return GST_BUS_PASS;
    }
    const char* matched = matchContextType(vt, contextType);
    if (!matched) {
        return GST_BUS_PASS;
    }
    GstElement* element = GST_ELEMENT(GST_MESSAGE_SRC(message));
    if (!element) {
        return GST_BUS_PASS;
    }

    GstContext* ctx = snapshotContext(vt, user, matched);
    if (!ctx) {
        return GST_BUS_PASS;
    }
    gst_element_set_context(element, ctx);
    gst_context_unref(ctx);

    if (vt.onHandoff) {
        vt.onHandoff(user, element, matched);
    } else {
        qCDebug(vt.cat(user)) << "Provided" << vt.apiName << matched << "context to" << GST_ELEMENT_NAME(element);
    }
    // `element` is borrowed from the message (transfer-none); unref the message only after the
    // last use of `element`, or the message could drop the final ref under us.
    gst_message_unref(message);
    return GST_BUS_DROP;
}

bool answerContextQuery(const BridgeVTable& vt, void* user, GstQuery* query)
{
    if (!query || GST_QUERY_TYPE(query) != GST_QUERY_CONTEXT) {
        return false;
    }
    const gchar* contextType = nullptr;
    if (!gst_query_parse_context_type(query, &contextType) || !contextType) {
        return false;
    }
    const char* matched = matchContextType(vt, contextType);
    if (!matched) {
        return false;
    }

    GstContext* ctx = snapshotContext(vt, user, matched);
    if (!ctx) {
        return false;
    }
    gst_query_set_context(query, ctx);
    gst_context_unref(ctx);
    return true;
}

void registerBridge(const QLoggingCategory& cat, const char* apiName, GstBusSyncReply (*handler)(GstMessage*),
                    void (*reset)())
{
    // Register both unconditionally so resetAllBridges() always reaches the bridge's reset(); registry warns on
    // overflow.
    const auto h = GstContextBridgeRegistry::registerBridgeHandler(handler);
    const auto r = GstContextBridgeRegistry::registerResetCallback(reset);
    if ((h == GstContextBridgeRegistry::kInvalidHandle) || (r == GstContextBridgeRegistry::kInvalidHandle)) {
        qCWarning(cat) << apiName << "bridge registration incomplete (registry full); GPU path inactive";
    }
}

}  // namespace GstContextBridge

#endif  // QGC_HAS_ANY_GPU_PATH
