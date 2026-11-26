#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/gst.h>

#include "GstContextBridgeBase.h"

/// Shares one GstD3D11Device/GstD3D12Device on QRhi's adapter through a single device-handle context type. Priming
/// waits for QRhi to run the matching backend and retries for each decoder a pipeline takes until it does. When
/// GStreamer reports the device removed (its `device-removed-reason`, from 1.26), every cached GPU resource is dropped
/// and the pipelines that got the device are told (noteDeviceRemoved()).
class GstD3DContextBridge : public GstContextBridgeBase
{
public:
    /// Transfer-full ref to the shared device (caller unrefs), nullptr when not primed.
    GstObject* refDevice();

    /// `adapter-luid` of a GstD3D11Device/GstD3D12Device; 0 for null or on read failure.
    static gint64 readAdapterLuid(gpointer device);

protected:
    /// @p qrhiBackend is the QRhi backend whose device the bridge shares; under @p secondQrhiBackend, when given, it
    /// also primes, with a device of its own on that backend's adapter.
    constexpr GstD3DContextBridge(const char* api, std::span<const char* const> contextTypes,
                                  const QLoggingCategory& (*logCategory)(), int qrhiBackend,
                                  int secondQrhiBackend = -1) noexcept
        : GstContextBridgeBase(api, contextTypes, logCategory)
        , _qrhiBackend(qrhiBackend)
        , _secondQrhiBackend(secondQrhiBackend)
    {}

    ~GstD3DContextBridge() = default;

    /// The device to share, from QGCRhiCapture's snapshot: D3D11 wraps QRhi's device, D3D12 creates one on QRhi's
    /// adapter. Logs and returns null on failure. The mutex is held.
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

    /// True when QGCRhiCapture's snapshot reports a QRhi backend this bridge primes under; logs the first mismatch.
    bool snapshotBackendMatches();
    /// The device's notify::device-removed-reason handler; @p bridge is the GstD3DContextBridge.
    static void onDeviceRemoved(GObject* device, GParamSpec* property, gpointer bridge);

    int _qrhiBackend;
    int _secondQrhiBackend;
    GstObject* _device = nullptr;
    /// The device-removed-reason notify handler on _device; 0 where GStreamer's device has no such property.
    gulong _removalHandler = 0;
    bool _warnedWrongBackend = false;
};

#endif  // QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH
