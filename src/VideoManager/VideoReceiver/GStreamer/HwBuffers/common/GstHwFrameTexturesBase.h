#pragma once

#include <QtMultimedia/QVideoFrame>
#include <gst/gst.h>
#include <memory>
#include <private/qhwvideobuffer_p.h>

#include "GstHwVideoBuffer.h"
#include "GstHwVideoBufferFactory.h"

class QRhiTexture;

/// Common base for per-platform `FrameTextures : QVideoFrameTextures` from `*VideoBuffer::mapTextures()`.
class GstHwFrameTexturesBase : public QVideoFrameTextures
{
public:
    ~GstHwFrameTexturesBase() override { g_clear_pointer(&_srcSample, gst_sample_unref); }

    void onFrameEndInvoked() override { g_clear_pointer(&_srcSample, gst_sample_unref); }

    QRhiTexture* texture(uint plane) const override { return int(plane) < _count ? _textures[plane].get() : nullptr; }

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

protected:
    int _count = 0;
    std::unique_ptr<QRhiTexture> _textures[GstHw::kMaxPlanes];
    GstSample* _srcSample = nullptr;
};
