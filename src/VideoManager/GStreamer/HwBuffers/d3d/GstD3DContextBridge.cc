#include "GstD3DContextBridge.h"

#if defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <glib-object.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>

#include "QGCRhiCapture.h"

gint64 GstD3DContextBridge::readAdapterLuid(gpointer device)
{
    if (!device || !G_IS_OBJECT(device)) {
        return 0;
    }
    gint64 luid = 0;
    g_object_get(G_OBJECT(device), "adapter-luid", &luid, nullptr);
    return luid;
}

// Reads QGCRhiCapture::deviceSnapshot() via atomic loads — safe from the bus-sync thread without touching QRhi
// cross-thread.
bool GstD3DContextBridge::snapshotBackendMatches()
{
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    if (backend < 0) {
        qCDebug(category()) << "QRhi snapshot not yet populated; will retry on next NEED_CONTEXT";
        return false;
    }
    if (backend != _qrhiBackend) {
        if (!_warnedWrongBackend) {
            qCInfo(category()) << "QRhi backend tag is" << backend << "(not" << apiName() << "); bridge inactive";
            _warnedWrongBackend = true;
        }
        return false;
    }
    return true;
}

bool GstD3DContextBridge::primeLocked()
{
    if (!snapshotBackendMatches()) {
        return retryLater();
    }
    GstObject* device = createDevice();
    if (!device) {
        return retryLater();
    }
    _device = device;
    qCInfo(category()) << apiName() << "bridge primed: shared device =" << device;
    return true;
}

GstObject* GstD3DContextBridge::refObject(const char* /*contextType*/)
{
    return _device ? GST_OBJECT(gst_object_ref(_device)) : nullptr;
}

GstContext* GstD3DContextBridge::buildContext(const char* /*contextType*/, GstObject* object)
{
    GstContext* ctx = makeDeviceContext(object);
    if (!ctx) {
        qCWarning(category()) << apiName() << "context_new failed";
    }
    return ctx;
}

void GstD3DContextBridge::clearLocked()
{
    gst_clear_object(&_device);
    _warnedWrongBackend = false;
}

GstObject* GstD3DContextBridge::refDevice()
{
    QMutexLocker lock(&_mutex);
    return _device ? GST_OBJECT(gst_object_ref(_device)) : nullptr;
}

void GstD3DContextBridge::logAdapterMatch(gint64 expectedLuid, gpointer gstDevice) const
{
    const gint64 actualLuid = readAdapterLuid(gstDevice);
    if (actualLuid != expectedLuid) {
        qCWarning(category()).noquote() << apiName() << "bridge: gst device LUID mismatch — QRhi LUID=" << expectedLuid
                                        << "but wrapped device LUID=" << actualLuid
                                        << "(zero-copy will appear corrupt; check NEED_CONTEXT race)";
        return;
    }
    qCInfo(category()).noquote() << apiName() << "bridge adapter LUID="
                                 << QString::asprintf("0x%llx", static_cast<long long>(expectedLuid));
}

#endif  // QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH
