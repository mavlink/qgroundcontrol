#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <private/qhwvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstD3DContextBridge.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

/// Shared scaffolding for D3D11 / D3D12 zero-copy QHwVideoBuffer wrappers.
namespace GstD3DVideoBufferCommon {

using GstHw::kMaxPlanes;

/// Scope guard for the native handles a resolvePlanes() loop builds up: releases every owned handle on scope
/// exit unless commit() transfers ownership on the success path.
template <class HandleT>
struct PlaneResourceGuard
{
    std::array<HandleT*, kMaxPlanes> handles{};
    int owned = 0;

    PlaneResourceGuard() = default;
    PlaneResourceGuard(const PlaneResourceGuard&) = delete;
    PlaneResourceGuard& operator=(const PlaneResourceGuard&) = delete;

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

/// Streaming-thread resolve shared by D3D11 and D3D12, run once per buffer from each path's buffer constructor. For
/// each memory: type check, native-device guard (plane 0) against the bridge's shared device, decoder sync, resource
/// lookup, and an AddRef of the decoder's resource, or of what the API makes of a texture-array slice
/// (QRhi::createFrom has no subresource selector). Fills @p guard and returns the resolved plane count; 0 after
/// warning the cause once.
///
/// @p Api supplies the API-specific pieces:
///   types Handle, Memory, Device; static kMemoryName, log(), isMemory(GstMemory*), bridgeDevice() (transfer-full),
///   sameDevice(Device*, Device*), resource(Memory*), subresource(Memory*, int plane), isArraySlice(Handle*, guint);
///   members prepare(Memory*, int plane) (decoder sync), copySlice(Handle*, guint subresource, int plane) (returns the
///   slice's own resource, AddRef'd, when the API copies it later, or null when it can't take a slice).
template <class Api>
int resolvePlanes(GstBuffer* buffer, Api& api, PlaneResourceGuard<typename Api::Handle>& guard)
{
    using Memory = typename Api::Memory;
    using Device = typename Api::Device;

    const int memCount = (std::min) (int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    for (int i = 0; i < memCount; ++i) {
        GstMemory* mem = gst_buffer_peek_memory(buffer, guint(i));
        if (!mem || !Api::isMemory(mem)) {
            QGC_HW_WARN_ONCE(Api::log, "resolve: plane" << i << "memory is not" << Api::kMemoryName << "(allocator="
                                                        << (mem && mem->allocator ? mem->allocator->mem_type : "null")
                                                        << ")");
            return 0;
        }
        auto* d3dmem = reinterpret_cast<Memory*>(mem);
        // gst-d3d may land on an isolated device when NEED_CONTEXT was preempted; sampling a foreign-device resource
        // from QRhi corrupts silently.
        if (i == 0) {
            // currentDevice() is transfer-full — unref on every branch to avoid UAF after reset().
            Device* bridgeDev = Api::bridgeDevice();
            if (bridgeDev && !Api::sameDevice(d3dmem->device, bridgeDev)) {
                const gint64 bridgeLuid = GstD3DContextBridge::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridge::readAdapterLuid(d3dmem->device);
                gst_object_unref(bridgeDev);
                QGC_HW_WARN_ONCE(Api::log, "resolve:" << Api::kMemoryName << "on foreign device (bridge LUID="
                                                      << bridgeLuid << "buffer LUID=" << bufLuid
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
            QGC_HW_WARN_ONCE(Api::log, "resolve:" << Api::kMemoryName << "has no resource handle for plane" << i);
            return 0;
        }
        const guint subIdx = Api::subresource(d3dmem, i);
        if (Api::isArraySlice(resource, subIdx)) {
            auto* slice = api.copySlice(resource, subIdx, i);
            if (!slice) {
                return 0;
            }
            guard.handles[i] = slice;
        } else {
            resource->AddRef();
            guard.handles[i] = resource;
        }
        guard.owned = i + 1;
    }
    return memCount;
}

/// Wraps an array of D3D native handles (ID3D11Texture2D*/ID3D12Resource*) as QRhi-importable textures; releases them
/// in the dtor, so the caller must own a fresh ref before constructing.
template <class HandleT>
class FrameTextures final : public GstHwFrameTexturesBase
{
public:
    FrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<HandleT*, kMaxPlanes> handles, int count)
        : _handles(handles)
    {
        wrapPlanes(rhi, size, pixelFormat,
                   std::span<HandleT* const>(_handles.data(), std::size_t(std::clamp(count, 0, kMaxPlanes))));
    }

    ~FrameTextures() override
    {
        for (int i = 0; i < _count; ++i) {
            if (_handles[i])
                _handles[i]->Release();
        }
    }

    /// The native resource plane @p plane wraps; null past the bundle's planes.
    HandleT* handle(int plane) const noexcept { return (plane >= 0 && plane < _count) ? _handles[plane] : nullptr; }

private:
    std::array<HandleT*, kMaxPlanes> _handles;
};

/// Shared D3D11/D3D12 render-thread tail run after the streaming-thread resolve: aliases a single NV12/P010 resource
/// across its planes, then reuses the prior bundle when it wraps the same handles or builds a fresh one over AddRef'd
/// handles. `HandleT` is deduced from the resolved handle array (ID3D11Texture2D / ID3D12Resource).
template <class HandleT>
QVideoFrameTexturesUPtr mapResolvedTextures(GstHwVideoBuffer& self, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                            const GstHwMapTail& tail,
                                            const std::array<HandleT*, kMaxPlanes>& resolvedHandles, int resolvedCount)
{
    using BundleT = FrameTextures<HandleT>;

    const auto* desc = QVideoTextureHelper::textureDescription(tail.pixelFormat);
    if (!desc || desc->nplanes <= 0) {
        return GstHwPathTelemetry::fail(tail.path);
    }

    const int textureCount = (std::min) (desc->nplanes, kMaxPlanes);
    if (resolvedCount <= 0 || resolvedCount > kMaxPlanes) {
        return GstHwPathTelemetry::fail(tail.path);
    }

    std::array<HandleT*, kMaxPlanes> handles{};
    for (int i = 0; i < (std::min) (resolvedCount, textureCount); ++i) {
        handles[i] = resolvedHandles[i];
    }

    if (resolvedCount < textureCount) {
        // D3D NV12/P010 output is one texture, which Qt samples the chroma of through a two-channel view.
        using Description = QVideoTextureHelper::TextureDescription;
        const bool chromaIsTwoChannel =
            (desc->textureFormat[1] == Description::RG_8) || (desc->textureFormat[1] == Description::RG_16);
        if (resolvedCount != 1 || !handles[0] || !chromaIsTwoChannel) {
            return GstHwPathTelemetry::fail(tail.path);
        }
        for (int i = resolvedCount; i < textureCount; ++i) {
            handles[i] = handles[0];
        }
    }

    // Reuse-eligible when the handles are identical: a decoder's pool and a path's copy targets rotate a stable set of
    // resources, so the existing QRhiTexture views sample the new frame.
    return GstHwFrameTexturesBase::finishMap<BundleT>(
        self, rhi, old, tail,
        [&](BundleT& prev) {
            return prev.matchesImport(&rhi, tail.frameSize, tail.pixelFormat,
                                      std::span<HandleT* const>(handles.data(), std::size_t(textureCount)));
        },
        [&] {
            // The bundle owns these AddRef'd refs; the buffer dtor releases the originals independently.
            for (int i = 0; i < textureCount; ++i) {
                if (handles[i])
                    handles[i]->AddRef();
            }
            return std::make_unique<BundleT>(&rhi, tail.frameSize, tail.pixelFormat, handles, textureCount);
        });
}

/// The D3D11/D3D12 QHwVideoBuffer shell: the planes resolvePlanes() resolved on the streaming thread (AddRef'd, and
/// released with the buffer), their check before import, and the start and end of mapTextures(). Each API's buffer
/// adds what differs: D3D11 copies array slices and padded textures on the render thread.
template <class Resolver>
class VideoBuffer : public GstHwVideoBuffer
{
public:
    using Handle = typename Resolver::Handle;

    ~VideoBuffer() override
    {
        for (int i = 0; i < _count; ++i) {
            if (_handles[std::size_t(i)]) {
                _handles[std::size_t(i)]->Release();
            }
        }
    }

    bool validatePlaneHandles() const override
    {
        // A failed constructor-time resolve (device mismatch, missing resource, sync or copy failure) sends the frame
        // to the CPU.
        if (_count == 0) {
            return false;
        }
        return validatePlanes([](GstMemory* mem) {
            return mem && Resolver::isMemory(mem) &&
                   (Resolver::resource(reinterpret_cast<typename Resolver::Memory*>(mem)) != nullptr);
        });
    }

protected:
    VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                HwVideoBufferPath path)
        : GstHwVideoBuffer(sample, videoInfo, format)
        , _path(path)
    {}

    /// Resolves the sample's planes through @p resolver into _handles; false, holding none, when none resolved.
    bool resolve(Resolver& resolver)
    {
        GstBuffer* const buffer = _sample ? gst_sample_get_buffer(_sample) : nullptr;
        if (!buffer) {
            return false;
        }
        PlaneResourceGuard<Handle> guard;
        const int count = resolvePlanes(buffer, resolver, guard);
        if (count == 0) {
            return false;
        }
        _handles = guard.handles;
        _count = count;
        guard.commit();
        return true;
    }

    /// mapTextures()'s preconditions on @p backend: the buffer to import, or null when the frame goes to the CPU.
    GstBuffer* beginMap(QRhi& rhi, QRhi::Implementation backend)
    {
        GstBuffer* buffer = nullptr;
        if (!checkMapPreconditions(rhi, static_cast<int>(backend), Resolver::log(), buffer) || (_count == 0)) {
            return nullptr;
        }
        return buffer;
    }

    /// Imports @p planes, the resolved planes or copies of them, through mapResolvedTextures().
    QVideoFrameTexturesUPtr finishMap(QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                      const std::array<Handle*, kMaxPlanes>& planes)
    {
        const GstHwMapTail tail{.path = _path,
                                .frameSize = _format.frameSize(),
                                .pixelFormat = _format.pixelFormat(),
                                .log = &Resolver::log(),
                                .successTag = GstHw::toString(_path)};
        return mapResolvedTextures(*this, rhi, old, tail, planes, _count);
    }

    std::array<Handle*, kMaxPlanes> _handles{};
    int _count = 0;

private:
    const HwVideoBufferPath _path;
};

}  // namespace GstD3DVideoBufferCommon

#endif  // QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH
