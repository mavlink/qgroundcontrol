#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <array>
#include <memory>
#include <private/qhwvideobuffer_p.h>
#include <span>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include <EGL/egl.h>

class QRhi;

/// QGC's only door to Qt Multimedia's private DMABuf→GL importer (qdmabuftextureimporter_p.h). That API carries no
/// compatibility promise between Qt minor releases, so nothing else includes its headers.
namespace GstQtDmaBufImport {

/// No modifier attributes: the driver resolves the layout itself, as for buffers negotiated without DMA_DRM caps.
inline constexpr quint64 kImplicitModifier = (1ULL << 56) - 1;  // DRM_FORMAT_MOD_INVALID

/// One plane, imported as its own single-plane EGLImage.
struct Plane
{
    int fd = -1;
    quint32 offset = 0;
    quint32 pitch = 0;
    quint32 drmFourcc = 0;
    quint64 modifier = kImplicitModifier;
};

/// DRM fourccs the importer samples the planes of @p pixelFormat through; count 0 when it can't import the format.
struct PlaneFourccs
{
    std::array<quint32, 4> fourcc{};
    int count = 0;
};

PlaneFourccs planeFourccs(QVideoFrameFormat::PixelFormat pixelFormat) noexcept;

/// EGLDisplay of @p rhi's GL context when that context is EGL and can bind EGLImages, else EGL_NO_DISPLAY. Makes the
/// context current. Render thread, GL QRhi only.
EGLDisplay importDisplay(QRhi& rhi) noexcept;

struct ImportResult
{
    QVideoFrameTexturesUPtr textures;
    /// The driver rejected the import in a way no later frame can avoid (EGL_BAD_MATCH, QTBUG-112312).
    bool unrecoverable = false;
};

/// Imports @p planes, which must match planeFourccs(@p pixelFormat), as textures of @p rhi. @p keepAlive lives as
/// long as the textures do. Render thread, GL QRhi only.
ImportResult importTextures(QRhi& rhi, std::span<const Plane> planes, QVideoFrameFormat::PixelFormat pixelFormat,
                            QSize frameSize, std::shared_ptr<void> keepAlive);

}  // namespace GstQtDmaBufImport

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
