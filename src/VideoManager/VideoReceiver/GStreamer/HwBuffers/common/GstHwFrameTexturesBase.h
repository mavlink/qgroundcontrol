#pragma once

#include <algorithm>
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

    /// GPU path that produced this bundle; used after a type-safe downcast to decide path-local reuse.
    virtual HwVideoBufferPath sourcePath() const { return HwVideoBufferPath::None; }

    /// Transfers a ref into the bundle. Caller must have a fresh ref.
    void setSourceSample(GstSample* s) noexcept
    {
        g_clear_pointer(&_srcSample, gst_sample_unref);
        _srcSample = s;
    }

    /// Reuse probe: @p old downcasts to @p FT iff it is one of our bundles and came from path @p p.
    /// Qt 6.10's texture pool can pass CPU-fallback or Qt-owned texture bundles here, so the base cast must be checked
    /// before consulting sourcePath().
    /// Caller still runs FT::matches() — the equality predicate differs per path.
    template <class FT>
    static FT* reusableBundle(QVideoFrameTexturesUPtr& old, HwVideoBufferPath p)
    {
        auto* base = old ? dynamic_cast<GstHwFrameTexturesBase*>(old.get()) : nullptr;
        return (base && base->sourcePath() == p) ? dynamic_cast<FT*>(base) : nullptr;
    }

    /// Shared mapTextures() tail: hands @p source's sample to @p old when it is an @p FT bundle that @p reuse accepts
    /// (reuse may refresh per-frame state on it; pass nullptr for paths without reuse). Otherwise preflights the
    /// format, builds a bundle with @p make and keeps it only when every plane wrapped.
    template <class FT, class Reuse, class Make>
    static QVideoFrameTexturesUPtr finishMap(GstHwVideoBuffer& source, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                             const GstHwMapTail& tail, Reuse&& reuse, Make&& make)
    {
        if constexpr (!std::is_null_pointer_v<std::remove_cvref_t<Reuse>>) {
            if (FT* prev = reusableBundle<FT>(old, tail.path); prev && reuse(*prev)) {
                GstHwPathTelemetry::recordTextureReuse(tail.path);
                prev->setSourceSample(source.takeSample());
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
            if (tail.log && tail.diag) {
                QGC_HW_WARN_ONCE(*tail.log, tail.diag->loggedTextureCreateFail,
                                 "QRhiTexture::createFrom failed (format=" << int(tail.pixelFormat)
                                                                           << "size=" << tail.frameSize << ")");
            }
            return GstHwPathTelemetry::fail(tail.path);
        }
        if (tail.successTag && tail.log && tail.diag) {
            GstHwVideoBuffer::logFirstSuccess(tail.diag->loggedFirstSuccess, *tail.log, tail.successTag, tail.frameSize,
                                              tail.pixelFormat, textures->_count);
        }
        textures->setSourceSample(source.takeSample());
        return textures;
    }

protected:
    /// Wraps one native handle per plane (GL name, VkImage, ID3D1x resource, MTLTexture) as a QRhiTexture. Fallback
    /// formats are disabled: a substituted format would misread the native layout. Planes QRhi rejects stay null, as do
    /// zero handles (a plane the source never produced), whatever the QRhi backend's createFrom() accepts.
    void wrapPlanes(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, std::span<const quint64> handles,
                    int layout = 0, QRhiTexture::Flags flags = {})
    {
        _count = static_cast<int>((std::min) (handles.size(), std::size_t(GstHw::kMaxPlanes)));
        const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            return;
        }
        for (int i = 0; i < _count; ++i) {
            if (handles[std::size_t(i)] == 0) {
                continue;
            }
            _textures[i].reset(rhi->newTexture(
                desc->rhiTextureFormat(i, rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable),
                desc->rhiPlaneSize(size, i, rhi), 1, flags));
            if (_textures[i] && !_textures[i]->createFrom({handles[std::size_t(i)], layout})) {
                _textures[i].reset();
            }
        }
    }

    int _count = 0;
    std::unique_ptr<QRhiTexture> _textures[GstHw::kMaxPlanes];
    GstSample* _srcSample = nullptr;
};
