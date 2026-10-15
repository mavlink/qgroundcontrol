#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <gst/gst.h>
#include <memory>
#include <private/qhwvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstD3DContextBridgeCommon.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

/// Shared scaffolding for D3D11 / D3D12 zero-copy QHwVideoBuffer wrappers.
namespace GstD3DVideoBufferCommon {

using GstHw::kMaxPlanes;

inline bool canAliasSingleResourceAcrossPlanes(QVideoFrameFormat::PixelFormat pixelFormat) noexcept
{
    switch (pixelFormat) {
        case QVideoFrameFormat::Format_NV12:
        case QVideoFrameFormat::Format_NV21:
        case QVideoFrameFormat::Format_P010:
        case QVideoFrameFormat::Format_P016:
            return true;
        default:
            return false;
    }
}

/// D3D-specific failure causes added to the shared GstHw::MapDiagnostics; each .cc keeps its own instance so the
/// per-cause log throttle stays separated per API.
struct MapDiagnostics : GstHw::MapDiagnostics
{
    std::atomic<bool> loggedNonD3DMemory{false};
    std::atomic<bool> loggedNullResource{false};
    // Tripped when GstD3DXMemory carries a device that doesn't match the bridge's shared device — a NEED_CONTEXT race
    // the bridge lost.
    std::atomic<bool> loggedDeviceMismatch{false};

    /// Re-arm every log-once flag so device-loss recovery surfaces warnings again.
    void reset() noexcept
    {
        loggedFirstSuccess.store(false, std::memory_order_release);
        loggedNullSample.store(false, std::memory_order_release);
        loggedBadBackend.store(false, std::memory_order_release);
        loggedNullBuffer.store(false, std::memory_order_release);
        loggedTextureCreateFail.store(false, std::memory_order_release);
        loggedNonD3DMemory.store(false, std::memory_order_release);
        loggedNullResource.store(false, std::memory_order_release);
        loggedDeviceMismatch.store(false, std::memory_order_release);
    }
};

/// Pool key: a staging resource is reusable only for an identical (size, format, plane) layout. width is UINT64 to hold
/// D3D12_RESOURCE_DESC::Width; D3D11's UINT widens losslessly.
struct StagingKey
{
    quint64 width = 0;
    quint32 height = 0;
    quint32 format = 0;  // DXGI_FORMAT
    int plane = 0;

    bool operator==(const StagingKey& o) const noexcept = default;
};

/// Scope guard for the native handles a resolvePlaneResources() loop builds up: releases every owned handle on scope
/// exit unless commit() transfers ownership on the success path.
template <class HandleT>
struct PlaneResourceGuard
{
    std::array<HandleT*, kMaxPlanes> handles{};
    int owned = 0;

    ~PlaneResourceGuard()
    {
        for (int j = 0; j < owned; ++j) {
            if (handles[j])
                handles[j]->Release();
        }
    }

    /// Disarm the guard after handles are copied into the owning member, so the destructor leaves the transferred refs
    /// alone.
    void commit() noexcept { owned = 0; }
};

/// Streaming-thread resolve shared by D3D11 and D3D12, run once per buffer from the GstD3D1xVideoBuffer ctor. For each
/// memory: type check, native-device guard (plane 0) against the bridge's shared device, decoder sync, resource lookup,
/// and either a slice copy into staging (QRhi::createFrom has no subresource selector) or an AddRef of the decoder's
/// resource. Fills @p guard and returns the resolved plane count; 0 after warning the cause once.
///
/// @p Api supplies the API-specific pieces:
///   types Handle, Memory, Device; static kMemoryName, log(), isMemory(GstMemory*), bridgeDevice() (transfer-full),
///   sameDevice(Device*, Device*), resource(Memory*), subresource(Memory*, int plane), isArraySlice(Handle*, guint);
///   members prepare(Memory*, int plane) (decoder sync), copySlice(Handle*, guint subresource, int plane, Memory*)
///   (returns an AddRef'd staging handle), finish() (flush/fence-wait the submitted copies).
template <class Api>
int resolvePlanes(GstBuffer* buffer, Api& api, MapDiagnostics& diag, PlaneResourceGuard<typename Api::Handle>& guard)
{
    using Memory = typename Api::Memory;
    using Device = typename Api::Device;

    const int memCount = (std::min) (int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    for (int i = 0; i < memCount; ++i) {
        GstMemory* mem = gst_buffer_peek_memory(buffer, guint(i));
        if (!mem || !Api::isMemory(mem)) {
            QGC_HW_WARN_ONCE(Api::log, diag.loggedNonD3DMemory,
                             "resolve: plane" << i << "memory is not" << Api::kMemoryName << "(allocator="
                                              << (mem && mem->allocator ? mem->allocator->mem_type : "null") << ")");
            return 0;
        }
        auto* d3dmem = reinterpret_cast<Memory*>(mem);
        // gst-d3d may land on an isolated device when NEED_CONTEXT was preempted; sampling a foreign-device resource
        // from QRhi corrupts silently.
        if (i == 0) {
            // currentDevice() is transfer-full — unref on every branch to avoid UAF after reset().
            Device* bridgeDev = Api::bridgeDevice();
            if (bridgeDev && !Api::sameDevice(d3dmem->device, bridgeDev)) {
                const gint64 bridgeLuid = GstD3DContextBridgeCommon::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridgeCommon::readAdapterLuid(d3dmem->device);
                gst_object_unref(bridgeDev);
                QGC_HW_WARN_ONCE(Api::log, diag.loggedDeviceMismatch,
                                 "resolve:" << Api::kMemoryName << "on foreign device (bridge LUID=" << bridgeLuid
                                            << "buffer LUID=" << bufLuid
                                            << "); bridge missed NEED_CONTEXT race — rejecting frame");
                return 0;
            }
            if (bridgeDev) {
                gst_object_unref(bridgeDev);
            }
        }
        if (!api.prepare(d3dmem, i)) {
            return 0;
        }
        auto* resource = Api::resource(d3dmem);
        if (!resource) {
            QGC_HW_WARN_ONCE(Api::log, diag.loggedNullResource,
                             "resolve:" << Api::kMemoryName << "has no resource handle for plane" << i);
            return 0;
        }
        const guint subIdx = Api::subresource(d3dmem, i);
        if (Api::isArraySlice(resource, subIdx)) {
            auto* staging = api.copySlice(resource, subIdx, i, d3dmem);
            if (!staging) {
                return 0;
            }
            guard.handles[i] = staging;
        } else {
            resource->AddRef();
            guard.handles[i] = resource;
        }
        guard.owned = i + 1;
    }
    return api.finish() ? memCount : 0;
}

/// Wraps an array of D3D native handles (ID3D11Texture2D*/ID3D12Resource*) as QRhi-importable textures; releases them
/// in the dtor, so the caller must own a fresh ref before constructing.
template <class HandleT>
class FrameTextures final : public GstHwFrameTexturesBase
{
public:
    FrameTextures(HwVideoBufferPath path, QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<HandleT*, kMaxPlanes> handles, int count)
        : _handles(handles), _path(path), _rhi(rhi), _size(size), _pixelFormat(pixelFormat)
    {
        std::array<quint64, kMaxPlanes> native{};
        std::transform(handles.begin(), handles.end(), native.begin(),
                       [](HandleT* handle) { return reinterpret_cast<quint64>(handle); });
        wrapPlanes(rhi, size, pixelFormat,
                   std::span<const quint64>(native.data(), std::size_t(std::clamp(count, 0, kMaxPlanes))));
    }

    ~FrameTextures() override
    {
        for (int i = 0; i < _count; ++i) {
            if (_handles[i])
                _handles[i]->Release();
        }
    }

    HwVideoBufferPath sourcePath() const override { return _path; }

    /// Keeps backend-owned state (D3D12 staging slot leases) alive for as long as QRhi may sample this bundle.
    void setKeepAlive(std::shared_ptr<void> keepAlive) noexcept { _keepAlive = std::move(keepAlive); }

    /// Reuse-eligible when the pooled staging handles are identical: the pool rotates a stable set of resources, so the
    /// existing QRhiTexture views transparently sample the newly-copied frame.
    bool matches(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                 const std::array<HandleT*, kMaxPlanes>& handles, int count) const
    {
        if (_rhi != rhi || _size != size || _pixelFormat != pixelFormat || _count != count) {
            return false;
        }
        for (int i = 0; i < _count; ++i) {
            if (!_handles[i] || _handles[i] != handles[i] || !_textures[i]) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<HandleT*, kMaxPlanes> _handles;
    std::shared_ptr<void> _keepAlive;
    HwVideoBufferPath _path = HwVideoBufferPath::None;
    QRhi* _rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
};

/// Shared D3D11/D3D12 render-thread tail run after the streaming-thread resolve: aliases a single NV12/P010 resource
/// across its planes, then reuses the prior bundle when the pooled staging handles match or builds a fresh one over
/// AddRef'd handles. `HandleT` is deduced from the resolved handle array (ID3D11Texture2D / ID3D12Resource).
template <class HandleT>
QVideoFrameTexturesUPtr mapResolvedTextures(GstHwVideoBuffer& self, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                            const GstHwMapTail& tail,
                                            const std::array<HandleT*, kMaxPlanes>& resolvedHandles, int resolvedCount,
                                            const std::shared_ptr<void>& keepAlive = {})
{
    using BundleT = FrameTextures<HandleT>;

    const auto* desc = QVideoTextureHelper::textureDescription(tail.pixelFormat);
    if (!desc || desc->nplanes <= 0) {
        return GstHwPathTelemetry::fail(tail.path);
    }

    const int textureCount = (std::min)(desc->nplanes, kMaxPlanes);
    if (resolvedCount <= 0 || resolvedCount > kMaxPlanes) {
        return GstHwPathTelemetry::fail(tail.path);
    }

    std::array<HandleT*, kMaxPlanes> handles{};
    for (int i = 0; i < (std::min)(resolvedCount, textureCount); ++i) {
        handles[i] = resolvedHandles[i];
    }

    if (resolvedCount < textureCount) {
        if (resolvedCount != 1 || !handles[0] || !canAliasSingleResourceAcrossPlanes(tail.pixelFormat)) {
            return GstHwPathTelemetry::fail(tail.path);
        }
        // D3D NV12/P010 output can expose one texture with multiple shader-resource planes.
        for (int i = resolvedCount; i < textureCount; ++i) {
            handles[i] = handles[0];
        }
    }

    return GstHwFrameTexturesBase::finishMap<BundleT>(
        self, rhi, old, tail,
        [&](BundleT& prev) {
            if (!prev.matches(&rhi, tail.frameSize, tail.pixelFormat, handles, textureCount)) {
                return false;
            }
            prev.setKeepAlive(keepAlive);
            return true;
        },
        [&] {
            // The bundle owns these AddRef'd refs; the buffer dtor releases the originals independently.
            for (int i = 0; i < textureCount; ++i) {
                if (handles[i])
                    handles[i]->AddRef();
            }
            auto textures =
                std::make_unique<BundleT>(tail.path, &rhi, tail.frameSize, tail.pixelFormat, handles, textureCount);
            textures->setKeepAlive(keepAlive);
            return textures;
        });
}

}  // namespace GstD3DVideoBufferCommon

#endif  // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
