#include "GstQtDmaBufImport.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

// Qt 6.12 private API (exported as Qt_6_PRIVATE_API): DmaBufEglContext, importDmaBufTextures,
// dmaBufFourccFromPixelFormat and createTexturesFromHandles. Re-check every call here when raising the Qt minimum.
#if QT_VERSION < QT_VERSION_CHECK(6, 12, 0)
#error "GstQtDmaBufImport needs the DMABuf importer Qt Multimedia exports from 6.12"
#endif

#include <algorithm>

#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
// Not <drm_fourcc.h>: its fourcc_code() macro breaks Qt's constexpr function of that name.
#include <private/qdmabuftextureimporter_p.h>
#include <private/qmultimedia_drm_support_p.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

static_assert(GstQtDmaBufImport::kImplicitModifier == QtMultimediaPrivate::DmaBufFormatModifierInvalid);

namespace GstQtDmaBufImport {

PlaneFourccs planeFourccs(QVideoFrameFormat::PixelFormat pixelFormat) noexcept
{
    PlaneFourccs out;
    switch (pixelFormat) {
        // Qt's table imports these as ABGR8888, which samples B as red: the bytes are B,G,R,A, i.e. DRM ARGB8888
        // (what Qt's GStreamer backend uses for BGRA, qgstvideobuffer.cpp).
        case QVideoFrameFormat::Format_BGRA8888:
        case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
        case QVideoFrameFormat::Format_BGRX8888:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            out.fourcc[0] = static_cast<quint32>(QtMultimediaPrivate::DRMFormat::ARGB8888);
#else
            out.fourcc[0] = static_cast<quint32>(QtMultimediaPrivate::DRMFormat::BGRA8888);
#endif
            out.count = 1;
            return out;
        default:
            break;
    }
    const auto formats = QtMultimediaPrivate::dmaBufFourccFromPixelFormat(pixelFormat);
    out.count = static_cast<int>((std::min) (formats.size(), qsizetype(out.fourcc.size())));
    for (int i = 0; i < out.count; ++i) {
        out.fourcc[std::size_t(i)] = static_cast<quint32>(formats[i]);
    }
    return out;
}

EGLDisplay importDisplay(QRhi& rhi) noexcept
{
    if (rhi.backend() != QRhi::OpenGLES2) {
        return EGL_NO_DISPLAY;
    }
    rhi.makeThreadLocalNativeContextCurrent();
    const QtMultimediaPrivate::DmaBufEglContext context(&rhi);
    return context.isValid() ? context.eglDisplay() : EGL_NO_DISPLAY;
}

ImportResult importTextures(QRhi& rhi, std::span<const Plane> planes, QVideoFrameFormat::PixelFormat pixelFormat,
                            QSize frameSize, std::shared_ptr<void> keepAlive)
{
    ImportResult result;
    // importDmaBufTextures() asserts one plane per texture of the format.
    const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
    if (!desc || planes.empty() || (std::ssize(planes) != desc->nplanes) || (rhi.backend() != QRhi::OpenGLES2)) {
        return result;
    }
    const QtMultimediaPrivate::DmaBufEglContext context(&rhi);
    if (!context.isValid()) {
        return result;
    }

    std::array<QtMultimediaPrivate::DmaBufPlane, 4> qtPlanes{};
    for (std::size_t i = 0; i < planes.size(); ++i) {
        qtPlanes[i] = {.fd = planes[i].fd,
                       .offset = planes[i].offset,
                       .pitch = planes[i].pitch,
                       .drmFormat = static_cast<QtMultimediaPrivate::DRMFormat>(planes[i].drmFourcc),
                       .modifier = planes[i].modifier};
    }
    auto handles = QtMultimediaPrivate::importDmaBufTextures(rhi, context, QSpan(qtPlanes.data(), planes.size()),
                                                             pixelFormat, frameSize, std::move(keepAlive));
    if (!handles) {
        result.unrecoverable = (handles.error() == QtMultimediaPrivate::FailureSeverity::unrecoverable);
        return result;
    }
    result.textures = QVideoTextureHelper::createTexturesFromHandles(std::move(*handles), rhi, pixelFormat, frameSize);
    return result;
}

}  // namespace GstQtDmaBufImport

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
