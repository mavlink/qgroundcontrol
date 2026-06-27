#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include <QtCore/QMutexLocker>
#include <glib-object.h>

#include "GstContextBridgeCommon.h"
#include "GstContextBridgeRegistry.h"
#include "QGCRhiCapture.h"

namespace GstD3DContextBridgeCommon {
namespace {

bool primeLocked(BridgeState& state, const BridgeOps& ops)
{
    if (state.primed) {
        return true;
    }
    if (!checkSnapshotBackend(state, ops.cat(), ops.qrhiBackend, ops.apiName)) {
        return false;
    }
    GstObject* device = ops.createDevice(ops.cat());
    if (!device) {
        return false;  // createDevice already logged the cause.
    }
    state.device = device;
    state.primed = true;
    qCInfo(ops.cat()) << ops.apiName << "bridge primed: shared device =" << device;
    return true;
}

}  // namespace

// Reads QGCRhiCapture::deviceSnapshot() via atomic loads — safe from the bus-sync thread without touching QRhi
// cross-thread.
bool checkSnapshotBackend(BridgeState& state, const QLoggingCategory& cat, int expectedBackend, const char* backendName)
{
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    if (backend < 0) {
        qCDebug(cat) << "QRhi snapshot not yet populated; will retry on next NEED_CONTEXT";
        return false;
    }
    if (backend != expectedBackend) {
        if (!state.warnedWrongBackend) {
            qCInfo(cat) << "QRhi backend tag is" << backend << "(not" << backendName << "); bridge inactive";
            state.warnedWrongBackend = true;
        }
        return false;
    }
    return true;
}

void logHandoff(BridgeState& state, const QLoggingCategory& cat, GstElement* element, const char* apiName)
{
    if (!state.loggedFirstHandoff.exchange(true, std::memory_order_relaxed)) {
        qCInfo(cat) << "First" << apiName << "device handoff to element" << GST_ELEMENT_NAME(element);
    } else {
        qCDebug(cat) << "Provided" << apiName << "device context to" << GST_ELEMENT_NAME(element);
    }
}

gint64 readAdapterLuid(gpointer device)
{
    if (!device || !G_IS_OBJECT(device))
        return 0;
    gint64 luid = 0;
    g_object_get(G_OBJECT(device), "adapter-luid", &luid, nullptr);
    return luid;
}

void logAdapterMatch(gint64 expectedLuid, gpointer gstDevice, const QLoggingCategory& cat, const char* apiName)
{
    const gint64 actualLuid = readAdapterLuid(gstDevice);
    if (actualLuid != expectedLuid) {
        qCWarning(cat).noquote() << apiName << "bridge: gst device LUID mismatch — QRhi LUID=" << expectedLuid
                                 << "but wrapped device LUID=" << actualLuid
                                 << "(zero-copy will appear corrupt; check NEED_CONTEXT race)";
        return;
    }
    qCInfo(cat).noquote() << apiName << "bridge adapter LUID="
                          << QString::asprintf("0x%llx", static_cast<long long>(expectedLuid));
}

void registerBridge(const QLoggingCategory& cat, GstBusSyncReply (*handler)(GstMessage*), void (*reset)())
{
    GstContextBridge::registerBridge(cat, "D3D", handler, reset);
}

bool prime(BridgeState& state, const BridgeOps& ops)
{
    QMutexLocker lock(&state.mutex);
    return primeLocked(state, ops);
}

GstObject* currentDevice(BridgeState& state)
{
    QMutexLocker lock(&state.mutex);
    if (!state.device) {
        return nullptr;
    }
    return GST_OBJECT(gst_object_ref(state.device));
}

namespace {

// Adapts a D3D BridgeState+BridgeOps pair onto the path-agnostic GstContextBridge skeleton. Both D3D bridges share this
// TU, so the per-instance state is threaded through `user` rather than file statics. One context-type → one device.
struct D3DUser
{
    BridgeState* state;
    const BridgeOps* ops;
};

const QLoggingCategory& vtCat(void* user)
{
    return static_cast<D3DUser*>(user)->ops->cat();
}

QMutex& vtMutex(void* user)
{
    return static_cast<D3DUser*>(user)->state->mutex;
}

bool vtPrime(void* user)
{
    auto* d = static_cast<D3DUser*>(user);
    return primeLocked(*d->state, *d->ops);
}

GstObject* vtRefObject(void* user, const char* /*contextType*/)
{
    auto* d = static_cast<D3DUser*>(user);
    return d->state->device ? GST_OBJECT(gst_object_ref(d->state->device)) : nullptr;
}

GstContext* vtBuildContext(void* user, const char* /*contextType*/, GstObject* object)
{
    auto* d = static_cast<D3DUser*>(user);
    GstContext* ctx = d->ops->makeContext(object);
    if (!ctx) {
        qCWarning(d->ops->cat()) << d->ops->apiName << "context_new failed";
    }
    return ctx;
}

void vtOnHandoff(void* user, GstElement* element, const char* /*contextType*/)
{
    auto* d = static_cast<D3DUser*>(user);
    logHandoff(*d->state, d->ops->cat(), element, d->ops->apiName);
}

GstContextBridge::BridgeVTable makeVTable(const BridgeOps& ops, const char** typeStorage)
{
    typeStorage[0] = ops.contextType;
    return GstContextBridge::BridgeVTable{
        ops.apiName, typeStorage, 1, &vtCat, &vtMutex, &vtPrime, &vtRefObject, &vtBuildContext, &vtOnHandoff,
    };
}

}  // namespace

GstBusSyncReply handleSyncMessage(BridgeState& state, const BridgeOps& ops, GstMessage* message)
{
    D3DUser user = {&state, &ops};
    const char* types[1];
    const GstContextBridge::BridgeVTable vt = makeVTable(ops, types);
    return GstContextBridge::handleSyncMessage(vt, &user, message);
}

bool answerContextQuery(BridgeState& state, const BridgeOps& ops, GstQuery* query)
{
    D3DUser user = {&state, &ops};
    const char* types[1];
    const GstContextBridge::BridgeVTable vt = makeVTable(ops, types);
    return GstContextBridge::answerContextQuery(vt, &user, query);
}

void reset(BridgeState& state, const BridgeOps& ops)
{
    QMutexLocker lock(&state.mutex);
    gst_clear_object(&state.device);
    state.primed = false;
    state.warnedWrongBackend = false;
    qCDebug(ops.cat()) << ops.apiName << "bridge reset";
}

}  // namespace GstD3DContextBridgeCommon

#endif  // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
