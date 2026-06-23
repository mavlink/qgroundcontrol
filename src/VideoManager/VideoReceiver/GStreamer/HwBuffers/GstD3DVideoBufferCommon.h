#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <private/qhwvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <array>
#include <atomic>
#include <memory>

/// Shared scaffolding for the D3D11 / D3D12 zero-copy QHwVideoBuffer wrappers.
///
/// Both wrappers walk the same shape: pull native textures out of a GstSample,
/// hand them to QRhi via `QRhiTexture::createFrom`, release the refs in the
/// QVideoFrameTextures dtor. They differ only in the native handle type
/// (`ID3D11Texture2D*` vs `ID3D12Resource*`) and in the slice-copy machinery
/// the loop in `mapTextures()` uses — those slice-copy helpers stay in the
/// per-API .cc files because the D3D11 immediate-context model and the D3D12
/// command-list+fence model are not unifiable.
namespace GstD3DVideoBufferCommon {

constexpr int kMaxPlanes = 4;

/// Per-translation-unit failure counters and one-shot warning flags. Each .cc
/// keeps its own static instance so the diagnostics stay separated per API.
struct MapDiagnostics {
    std::atomic<quint64> mapFailureCount{0};
    std::atomic<bool> loggedFirstSuccess{false};
    // One-shot log flags per failure reason — keeps CI logs informative without
    // spamming at framerate. Each path warns the first time it trips, then bumps
    // the counter silently; teardown emits the running total.
    std::atomic<bool> loggedNullSample{false};
    std::atomic<bool> loggedBadBackend{false};
    std::atomic<bool> loggedNullBuffer{false};
    std::atomic<bool> loggedNonD3DMemory{false};
    std::atomic<bool> loggedNullResource{false};
    std::atomic<bool> loggedTextureCreateFail{false};
    // Tripped when GstD3DXMemory carries a device that doesn't match the bridge's shared
    // device — sampling from another device's textures corrupts or crashes silently, so the
    // wrapper rejects the frame instead. Indicates a NEED_CONTEXT race the bridge lost.
    std::atomic<bool> loggedDeviceMismatch{false};
};

inline QVideoFrameTexturesUPtr fail(MapDiagnostics &d)
{
    d.mapFailureCount.fetch_add(1, std::memory_order_relaxed);
    return {};
}

inline quint64 takeMapFailureCount(MapDiagnostics &d)
{
    return d.mapFailureCount.exchange(0, std::memory_order_relaxed);
}

inline quint64 peekMapFailureCount(MapDiagnostics &d)
{
    return d.mapFailureCount.load(std::memory_order_relaxed);
}

/// Wraps an array of D3D native handles (ID3D11Texture2D* or ID3D12Resource*)
/// as QRhi-importable textures. Releases the handles in the destructor — the
/// caller must `AddRef()` (or own a fresh ref) before constructing.
template<class HandleT>
class FrameTextures final : public QVideoFrameTextures
{
public:
    FrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<HandleT *, kMaxPlanes> handles, int count)
        : _count(count)
        , _handles(handles)
    {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) return;
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
            if (_handles[i]) _handles[i]->Release();
        }
    }

    QRhiTexture *texture(uint plane) const override
    {
        return (int(plane) < _count) ? _textures[plane].get() : nullptr;
    }

private:
    int _count = 0;
    std::array<HandleT *, kMaxPlanes> _handles;
    std::unique_ptr<QRhiTexture> _textures[kMaxPlanes];
};

} // namespace GstD3DVideoBufferCommon

/// Logs an arbitrary qCWarning the first time the flag flips; subsequent
/// trips bump the per-diagnostic counter silently.
#define QGC_D3D_WARN_ONCE(LOGCAT, FLAG, ...)                                              \
    do {                                                                                  \
        if (!(FLAG).exchange(true, std::memory_order_relaxed)) {                          \
            qCWarning(LOGCAT) << __VA_ARGS__;                                             \
        }                                                                                 \
    } while (0)

#endif // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
