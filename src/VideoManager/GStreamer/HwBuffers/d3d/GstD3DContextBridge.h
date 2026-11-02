#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/gst.h>

#include "GstContextBridgeBase.h"

/// Shares one GstD3D11Device/GstD3D12Device on QRhi's adapter through a single device-handle context type. Priming
/// waits for QRhi to run the matching backend and retries on every request until it does.
class GstD3DContextBridge : public GstContextBridgeBase
{
public:
    /// Transfer-full ref to the shared device (caller unrefs), nullptr when not primed.
    GstObject* refDevice();

    /// `adapter-luid` of a GstD3D11Device/GstD3D12Device; 0 for null or on read failure.
    static gint64 readAdapterLuid(gpointer device);

protected:
    constexpr GstD3DContextBridge(const char* api, std::span<const char* const> contextTypes,
                                  const QLoggingCategory& (*logCategory)(), int qrhiBackend) noexcept
        : GstContextBridgeBase(api, contextTypes, logCategory)
        , _qrhiBackend(qrhiBackend)
    {}

    ~GstD3DContextBridge() = default;

    /// Wraps QRhi's device from QGCRhiCapture's snapshot; logs and returns null on failure. The mutex is held.
    virtual GstObject* createDevice() = 0;

    /// New GstContext carrying @p device (the context refs it).
    virtual GstContext* makeDeviceContext(GstObject* device) = 0;

    /// Logs whether @p gstDevice sits on the adapter QRhi reported (@p expectedLuid); a mismatch corrupts frames.
    void logAdapterMatch(gint64 expectedLuid, gpointer gstDevice) const;

private:
    bool primeLocked() final;
    GstObject* refObject(const char* contextType) final;
    GstContext* buildContext(const char* contextType, GstObject* object) final;
    void clearLocked() final;

    /// True when QGCRhiCapture's snapshot reports this bridge's QRhi backend; logs the first mismatch.
    bool snapshotBackendMatches();

    int _qrhiBackend;
    GstObject* _device = nullptr;
    bool _warnedWrongBackend = false;
};

#endif  // QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH
