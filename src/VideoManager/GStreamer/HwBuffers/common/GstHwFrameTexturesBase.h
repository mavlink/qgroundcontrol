#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <gst/gst.h>
#include <memory>
#include <private/qhwvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <span>
#include <type_traits>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwImportPreflight.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"

/// Per-call inputs of GstHwFrameTexturesBase::finishMap().
struct GstHwMapTail
{
    HwVideoBufferPath path = HwVideoBufferPath::None;
    QSize frameSize{};
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    QRhiTexture::Flags textureFlags{};
    /// Required: where finishMap() logs, and its once-only flags.
    const QLoggingCategory* log = nullptr;
    GstHw::MapDiagnostics* diag = nullptr;
    /// False when the caller already preflighted before its costly native import work.
    bool preflight = true;
    /// Logs the one-shot first-success line under this tag when set.
    const char* successTag = nullptr;
};

/// Common base for per-platform `FrameTextures : QVideoFrameTextures` from `*VideoBuffer::mapTextures()`.
/// Qt invokes onFrameEndInvoked() on the current bundle after every window frame and keeps sampling it until its pool
/// slot is reused, so the source sample and every native resource must live until destruction — never release them
/// from onFrameEndInvoked().
class GstHwFrameTexturesBase : public QVideoFrameTextures
{
public:
    ~GstHwFrameTexturesBase() override { g_clear_pointer(&_srcSample, gst_sample_unref); }

    QRhiTexture* texture(uint plane) const override
    {
        return plane < static_cast<uint>(_count) ? _textures[plane].get() : nullptr;
    }

    /// True when the bundle has planes and every one of them wrapped its native handle.
    bool allPlanesValid() const noexcept
    {
        return _count > 0 && std::all_of(_textures, _textures + _count, [](const auto& t) { return t != nullptr; });
    }

    /// Transfers a ref into the bundle. Caller must have a fresh ref.
    void setSourceSample(GstSample* s) noexcept
    {
        g_clear_pointer(&_srcSample, gst_sample_unref);
        _srcSample = s;
    }

    /// True when this bundle wrapped @p handles for the same QRhi, size and format and every plane wrapped: its
    /// QRhiTexture views then sample a recycled native resource's new contents unchanged.
    template <class Handles>
    bool matchesImport(const QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                       const Handles& handles) const noexcept
    {
        if ((_rhi != rhi) || (_size != size) || (_pixelFormat != pixelFormat) ||
            (std::size(handles) != static_cast<std::size_t>(_count)) || !allPlanesValid()) {
            return false;
        }
        for (int i = 0; i < _count; ++i) {
            if (_nativeHandles[std::size_t(i)] != nativeHandle(handles[std::size_t(i)])) {
                return false;
            }
        }
        return true;
    }

    /// @p old as an @p FT bundle, else nullptr. Each path's bundle type is distinct, and Qt's texture pool can also
    /// pass CPU-fallback or Qt-owned bundles here.
    template <class FT>
    static FT* reusableBundle(QVideoFrameTexturesUPtr& old)
    {
        return dynamic_cast<FT*>(old.get());
    }

    /// Shared mapTextures() tail: hands @p source's sample to @p old when it is an @p FT bundle that @p reuse accepts
    /// (typically through matchesImport(); reuse may refresh per-frame state on it; pass nullptr for paths without
    /// reuse). Otherwise preflights the format, builds a bundle with @p make and keeps it only when every plane
    /// wrapped.
    template <class FT, class Reuse, class Make>
    static QVideoFrameTexturesUPtr finishMap(GstHwVideoBuffer& source, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                             const GstHwMapTail& tail, Reuse&& reuse, Make&& make)
    {
        if constexpr (!std::is_null_pointer_v<std::remove_cvref_t<Reuse>>) {
            if (FT* prev = reusableBundle<FT>(old); prev && reuse(*prev)) {
                GstHwPathTelemetry::record(tail.path, GstHwPathTelemetry::Counter::TextureReuse);
                prev->setSourceSample(source.refSample());
                return std::move(old);
            }
        }
        if (tail.preflight && !GstHwImportPreflight::preflightOrRecord(&rhi, tail.path, tail.pixelFormat,
                                                                       tail.frameSize, tail.textureFlags)) {
            return GstHwPathTelemetry::fail(tail.path);
        }
        std::unique_ptr<FT> textures = make();
        // NV12 chroma can fail while luma succeeds; a partial bundle would render with missing planes.
        if (!textures || !textures->allPlanesValid()) {
            GstHwPathTelemetry::recordFallbackReason(tail.path, GstHwPathTelemetry::HwFallbackReason::MapFailed);
            QGC_HW_WARN_ONCE(
                *tail.log, tail.diag->loggedTextureCreateFail,
                "QRhiTexture::createFrom failed (format=" << int(tail.pixelFormat) << "size=" << tail.frameSize << ")");
            return GstHwPathTelemetry::fail(tail.path);
        }
        if (tail.successTag && !tail.diag->loggedFirstSuccess.exchange(true, std::memory_order_relaxed)) {
            qCInfo(*tail.log) << "First" << tail.successTag << "zero-copy mapTextures success: size=" << tail.frameSize
                              << "format=" << int(tail.pixelFormat) << "planes=" << textures->_count;
        }
        textures->setSourceSample(source.refSample());
        return textures;
    }

protected:
    /// Wraps one native handle per plane (GL name, VkImage, ID3D1x resource, MTLTexture) as a QRhiTexture and records
    /// the import for matchesImport(). Fallback formats are disabled: a substituted format would misread the native
    /// layout. Planes QRhi rejects stay null, as do zero handles (a plane the source never produced), whatever the QRhi
    /// backend's createFrom() accepts.
    template <class Handles>
    void wrapPlanes(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, const Handles& handles,
                    int layout = 0, QRhiTexture::Flags flags = {})
    {
        _rhi = rhi;
        _size = size;
        _pixelFormat = pixelFormat;
        _count = static_cast<int>((std::min) (std::size(handles), std::size_t(GstHw::kMaxPlanes)));
        for (int i = 0; i < _count; ++i) {
            _nativeHandles[std::size_t(i)] = nativeHandle(handles[std::size_t(i)]);
        }
        const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            return;
        }
        for (int i = 0; i < _count; ++i) {
            const quint64 handle = _nativeHandles[std::size_t(i)];
            if (handle == 0) {
                continue;
            }
            _textures[i].reset(rhi->newTexture(
                desc->rhiTextureFormat(i, rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable),
                desc->rhiPlaneSize(size, i, rhi), 1, flags));
            if (_textures[i] && !_textures[i]->createFrom({handle, layout})) {
                _textures[i].reset();
            }
        }
    }

    QRhi* _rhi = nullptr;
    int _count = 0;
    std::unique_ptr<QRhiTexture> _textures[GstHw::kMaxPlanes];
    GstSample* _srcSample = nullptr;

private:
    /// @p handle as a QRhiTexture::NativeTexture object.
    template <class Handle>
    static quint64 nativeHandle(Handle handle) noexcept
    {
        if constexpr (std::is_pointer_v<Handle>) {
            return static_cast<quint64>(reinterpret_cast<quintptr>(handle));
        } else {
            return static_cast<quint64>(handle);
        }
    }

    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    std::array<quint64, GstHw::kMaxPlanes> _nativeHandles{};
};
