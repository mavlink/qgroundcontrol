#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <QtCore/QMutex>
#include <gst/gst.h>

QT_FORWARD_DECLARE_CLASS(QLoggingCategory)

/// Shared NEED_CONTEXT / GST_QUERY_CONTEXT skeleton for every QGC↔GStreamer context bridge (GL, Vulkan, D3D11, D3D12).
/// Centralizes the one delicate invariant all bridges share: snapshot the cached GstObject under the bridge mutex, then
/// release the mutex BEFORE gst_element_set_context()/gst_query_set_context() — those re-enter the handler via nested
/// context queries and self-deadlock if the mutex is still held. Each bridge supplies its prime/ref/build specifics via
/// BridgeVTable; one context-type maps to exactly one GstObject across all bridges.
namespace GstContextBridge {

/// Per-bridge hooks for the shared skeleton. `user` is an opaque pointer threaded back to every callback (the D3D
/// bridges pass per-instance state; the single-instance GL/Vulkan bridges pass nullptr and use file statics).
struct BridgeVTable
{
    const char* apiName;                         ///< "GL"/"Vulkan"/"D3D11"/"D3D12" — log label.
    const char* const* contextTypes;             ///< gst context-type strings this bridge answers.
    int contextTypeCount;                        ///< entries in contextTypes.
    const QLoggingCategory& (*cat)(void* user);  ///< bridge logging category.
    QMutex& (*mutex)(void* user);                ///< bridge state mutex (guards prime/ref).
    bool (*primeLocked)(void* user);             ///< build/validate cached objects; caller holds mutex.
    GstObject* (*refObject)(void* user, const char* contextType);  ///< transfer-full ref to the object backing
                                                                   ///< contextType, or null; caller holds mutex.
    GstContext* (*buildContext)(void* user, const char* contextType,
                                GstObject* object);                ///< wrap object in a
                                                                   ///< GstContext (gst refs internally); no lock held.
    void (*onHandoff)(void* user, GstElement* element, const char* contextType);  ///< optional post-set log hook;
                                                                                  ///< null → default qCDebug.
};

/// Matched canonical context-type string (from vt.contextTypes) for @p type, or nullptr if this bridge ignores it.
const char* matchContextType(const BridgeVTable& vt, const char* type);

/// Answer a NEED_CONTEXT sync message; GST_BUS_DROP when consumed (message unref'd), else GST_BUS_PASS.
GstBusSyncReply handleSyncMessage(const BridgeVTable& vt, void* user, GstMessage* message);

/// Answer a GST_QUERY_CONTEXT (sink-bin query path); true when consumed.
bool answerContextQuery(const BridgeVTable& vt, void* user, GstQuery* query);

/// Register a bridge's sync handler + reset callback with GstContextBridgeRegistry (the two calls every bridge
/// repeats).
void registerBridge(const QLoggingCategory& cat, const char* apiName, GstBusSyncReply (*handler)(GstMessage*),
                    void (*reset)());

}  // namespace GstContextBridge

#endif  // QGC_HAS_ANY_GPU_PATH
