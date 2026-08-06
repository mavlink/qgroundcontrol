#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <gst/gst.h>
#include <memory>
#include <private/qhwvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <tuple>

#include "GstHwFrameTexturesBase.h"
#include "GstHwImportPreflight.h"
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

    bool operator<(const StagingKey& o) const
    {
        return std::tie(width, height, format, plane) < std::tie(o.width, o.height, o.format, o.plane);
    }

    bool operator==(const StagingKey& o) const noexcept = default;
};

struct StagingKeyHash
{
    std::size_t operator()(const StagingKey& key) const noexcept
    {
        std::size_t seed = std::hash<quint64>{}(key.width);
        seed ^= std::hash<quint32>{}(key.height) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<quint32>{}(key.format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(key.plane) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
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
        _count = count;
        const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc)
            return;
        for (int i = 0; i < _count; ++i) {
            const QSize planeSize = desc->rhiPlaneSize(size, i, rhi);
            _textures[i].reset(rhi->newTexture(desc->rhiTextureFormat(i, rhi), planeSize, 1, {}));
            if (_textures[i] && !_textures[i]->createFrom({reinterpret_cast<quint64>(handles[i]), 0})) {
                _textures[i].reset();
            }
        }
    }

    ~FrameTextures() override
    {
        for (int i = 0; i < _count; ++i) {
            if (_handles[i])
                _handles[i]->Release();
        }
    }

    HwVideoBufferPath sourcePath() const override { return _path; }

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
    HwVideoBufferPath _path = HwVideoBufferPath::None;
    QRhi* _rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
};

/// Shared D3D11/D3D12 render-thread tail run after the streaming-thread resolve: reuse the prior bundle when the pooled
/// staging handles match, else AddRef the handles, build a fresh FrameTextures, verify every plane, and log first
/// success. The caller guarantees `_resolved` and passes its resolved handle array + count. `HandleT` is deduced from
/// the array (ID3D11Texture2D / ID3D12Resource); `catFn` is the logging-category accessor (e.g. GstD3D11Log) so the
/// qCWarning-based once-logging macro keeps working. Monomorphized per backend; no virtual dispatch added.
template <class HandleT, class CatFn>
QVideoFrameTexturesUPtr mapResolvedTextures(GstHwVideoBuffer& self, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                            HwVideoBufferPath path,
                                            const std::array<HandleT*, kMaxPlanes>& resolvedHandles, int resolvedCount,
                                            QSize frameSize, QVideoFrameFormat::PixelFormat pixelFormat,
                                            std::atomic<bool>& loggedFirstSuccess,
                                            std::atomic<bool>& loggedTextureCreateFail, CatFn catFn, const char* tag)
{
    using BundleT = FrameTextures<HandleT>;

    const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
    if (!desc || desc->nplanes <= 0) {
        return GstHwPathTelemetry::fail(path);
    }

    const int textureCount = (std::min)(desc->nplanes, kMaxPlanes);
    if (resolvedCount <= 0 || resolvedCount > kMaxPlanes) {
        return GstHwPathTelemetry::fail(path);
    }

    std::array<HandleT*, kMaxPlanes> handles{};
    for (int i = 0; i < (std::min)(resolvedCount, textureCount); ++i) {
        handles[i] = resolvedHandles[i];
    }

    if (resolvedCount < textureCount) {
        if (resolvedCount != 1 || !handles[0] || !canAliasSingleResourceAcrossPlanes(pixelFormat)) {
            return GstHwPathTelemetry::fail(path);
        }
        // D3D NV12/P010 output can expose one texture with multiple shader-resource planes.
        for (int i = resolvedCount; i < textureCount; ++i) {
            handles[i] = handles[0];
        }
    }

    if (auto* prev = GstHwFrameTexturesBase::reusableBundle<BundleT>(old, path)) {
        if (prev->matches(&rhi, frameSize, pixelFormat, handles, textureCount)) {
            GstHwPathTelemetry::recordTextureReuse(path);
            prev->setSourceSample(self.takeSample());
            QVideoFrameTexturesUPtr reused = std::move(old);
            return reused;
        }
    }

    // Pre-flight RHI format/size support before createFrom() so an unsupported import demotes to CPU on a query.
    if (!GstHwImportPreflight::preflightOrRecord(&rhi, path, pixelFormat, frameSize)) {
        return GstHwPathTelemetry::fail(path);
    }

    // FrameTextures takes ownership of AddRef'd refs; the buffer dtor releases the originals independently.
    for (int i = 0; i < textureCount; ++i) {
        if (handles[i])
            handles[i]->AddRef();
    }

    auto textures = std::make_unique<BundleT>(path, &rhi, frameSize, pixelFormat, handles, textureCount);
    // Check all planes: NV12 chroma can fail while luma succeeds, and a partial bundle renders with missing planes.
    for (int i = 0; i < textureCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            QGC_HW_WARN_ONCE(catFn, loggedTextureCreateFail,
                             "mapTextures: QRhiTexture::createFrom failed plane="
                                 << i << " (size=" << frameSize << " format=" << int(pixelFormat)
                                 << " planes=" << textureCount << ")");
            return GstHwPathTelemetry::fail(path);
        }
    }

    GstHwVideoBuffer::logFirstSuccess(loggedFirstSuccess, catFn(), tag, frameSize, pixelFormat, textureCount);
    textures->setSourceSample(self.takeSample());
    return textures;
}

}  // namespace GstD3DVideoBufferCommon

#endif  // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
