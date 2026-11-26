#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D_GPU_PATH)

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

#include <QtCore/QLoggingCategory>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstD3DContextBridge.h"
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
        // gst-d3d makes a device of its own in a pipeline built before the bridge had Qt's; sampling a foreign-device
        // resource from QRhi corrupts silently.
        if (i == 0) {
            // bridgeDevice() is transfer-full — unref on every branch to avoid UAF after reset().
            Device* bridgeDev = Api::bridgeDevice();
            if (bridgeDev && !Api::sameDevice(d3dmem->device, bridgeDev)) {
                const gint64 bridgeLuid = GstD3DContextBridge::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridge::readAdapterLuid(d3dmem->device);
                gst_object_unref(bridgeDev);
                QGC_HW_WARN_ONCE(Api::log, "resolve:" << Api::kMemoryName << "on foreign device (bridge LUID="
                                                      << bridgeLuid << "buffer LUID=" << bufLuid
                                                      << "); not the bridge's device — rejecting frame");
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

/// One frame's Direct3D resources (ID3D11Texture2D*/ID3D12Resource*), one reference each, as the native handles of its
/// planes: QRhi wraps them in place, and they are released with the handles.
template <class HandleT>
class TextureHandles final : public GstHwTextureHandles
{
public:
    /// AddRefs the first @p count of @p resources.
    TextureHandles(GstSample* sample, const std::array<HandleT*, kMaxPlanes>& resources, int count)
        : GstHwTextureHandles(sample)
        , _resources(resources)
        , _count(std::clamp(count, 0, kMaxPlanes))
    {
        for (int i = 0; i < _count; ++i) {
            if (_resources[std::size_t(i)]) {
                _resources[std::size_t(i)]->AddRef();
            }
            setHandle(i, _resources[std::size_t(i)]);
        }
    }

    ~TextureHandles() override
    {
        for (int i = 0; i < _count; ++i) {
            if (_resources[std::size_t(i)]) {
                _resources[std::size_t(i)]->Release();
            }
        }
    }

private:
    std::array<HandleT*, kMaxPlanes> _resources;
    int _count;
};

/// The D3D11/D3D12 QHwVideoBuffer shell: the planes resolvePlanes() resolved on the streaming thread (AddRef'd, and
/// released with the buffer), their check before import, and the start and end of importTextures(). Each API's buffer
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

    HwVideoBufferPath path() const override { return _path; }

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

    /// importTextures()'s preconditions on @p backend: the buffer to import, or null when the frame goes to the CPU.
    GstBuffer* beginMap(QRhi& rhi, QRhi::Implementation backend)
    {
        GstBuffer* buffer = nullptr;
        if (!checkMapPreconditions(rhi, static_cast<int>(backend), Resolver::log(), buffer) || (_count == 0)) {
            return nullptr;
        }
        return buffer;
    }

    /// Imports the first @p count of @p planes, the resolved planes or copies of them: one NV12/P010 resource stands
    /// for each of its planes, which Qt samples the chroma of through a two-channel view. @p successTag names the
    /// import in the line logged at its first success.
    template <class HandleT>
    QVideoFrameTexturesUPtr importPlanes(QRhi& rhi, const std::array<HandleT*, kMaxPlanes>& planes, int count,
                                         const char* successTag)
    {
        const auto* desc = QVideoTextureHelper::textureDescription(_format.pixelFormat());
        if (!desc || (desc->nplanes <= 0) || (count <= 0) || (count > kMaxPlanes)) {
            return {};
        }
        const int textureCount = (std::min) (desc->nplanes, kMaxPlanes);
        std::array<HandleT*, kMaxPlanes> handles{};
        for (int i = 0; i < (std::min) (count, textureCount); ++i) {
            handles[std::size_t(i)] = planes[std::size_t(i)];
        }
        if (count < textureCount) {
            using Description = QVideoTextureHelper::TextureDescription;
            const bool chromaIsTwoChannel =
                (desc->textureFormat[1] == Description::RG_8) || (desc->textureFormat[1] == Description::RG_16);
            if ((count != 1) || !handles[0] || !chromaIsTwoChannel) {
                return {};
            }
            for (int i = count; i < textureCount; ++i) {
                handles[std::size_t(i)] = handles[0];
            }
        }
        return wrapTextures(rhi, std::make_unique<TextureHandles<HandleT>>(refSample(), handles, textureCount),
                            Resolver::log(), successTag);
    }

    /// Imports the resolved planes, or copies of them in @p planes.
    QVideoFrameTexturesUPtr importResolved(QRhi& rhi, const std::array<Handle*, kMaxPlanes>& planes)
    {
        return importPlanes(rhi, planes, _count, GstHw::toString(_path));
    }

    std::array<Handle*, kMaxPlanes> _handles{};
    int _count = 0;

private:
    const HwVideoBufferPath _path;
};

}  // namespace GstD3DVideoBufferCommon

#endif  // QGC_HAS_GST_D3D_GPU_PATH
