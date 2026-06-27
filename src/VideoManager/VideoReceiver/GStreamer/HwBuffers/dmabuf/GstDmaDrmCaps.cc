#include "GstDmaDrmCaps.h"

#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
#include "GstDmaFourcc.h"
#include "GstEglHelpers.h"

#include "QGCLoggingCategory.h"

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtGui/QOpenGLContext>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cstdint>
#include <vector>

QGC_LOGGING_CATEGORY(GstDmaDrmCapsLog, "Video.GStreamer.HwBuffers.GstDmaDrmCaps")
#endif

namespace GstHw {

bool dmaDrmAwareVideoInfo(GstCaps* caps, GstVideoInfo* info)
{
    if (!caps || !info) {
        return false;
    }
#if GST_CHECK_VERSION(1, 24, 0)
    if (gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo;
        gst_video_info_dma_drm_init(&drmInfo);
        return gst_video_info_dma_drm_from_caps(&drmInfo, caps) && gst_video_info_dma_drm_to_video_info(&drmInfo, info);
    }
#endif
    return gst_video_info_from_caps(info, caps);
}

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)

namespace {

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";

// Renderable tokens from the kFormats brace list ("{ NV12, NV21, ... }"); intersected with EGL results below so we
// never advertise a format the Qt mapper would reject.
QSet<QByteArray> parseFormatTokens(const char* gstFormatList)
{
    QSet<QByteArray> tokens;
    if (!gstFormatList)
        return tokens;
    for (QByteArray tok : QByteArray(gstFormatList).split(',')) {
        tok.replace('{', "").replace('}', "");
        tok = tok.trimmed();
        if (!tok.isEmpty())
            tokens.insert(tok);
    }
    return tokens;
}

// LINEAR-only: tiled external DMABuf from gst-va/iHD can be negotiated and even imported, but binding the resulting
// EGLImage through Qt's render-thread GL path segfaults Mesa/Gallium on this stack. The default glupload path can
// handle those tiled buffers; direct-QGC DMABuf only advertises layouts it can safely bind itself.
bool isImportableModifier(EGLuint64KHR mod) noexcept
{
    return mod == 0;  // DRM_FORMAT_MOD_LINEAR
}

}  // namespace

std::string buildSupportedDmaDrmCaps(const char* gstFormatList)
{
    const QSet<QByteArray> allowed = parseFormatTokens(gstFormatList);
    if (allowed.isEmpty())
        return std::string();

    // Built on the GstVideo worker thread with no current GL context, so the share-context display is
    // EGL_NO_DISPLAY; fall back to the default display (modifiers are GPU-global). Never eglTerminate'd.
    EGLDisplay dpy = GstEglHelpers::resolveEglDisplay(QOpenGLContext::globalShareContext());
    if (dpy == EGL_NO_DISPLAY)
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy != EGL_NO_DISPLAY)
        eglInitialize(dpy, nullptr, nullptr);
    if (dpy == EGL_NO_DISPLAY || !GstEglHelpers::displaySupportsExtension(dpy, kModifiersExt)) {
        qCDebug(GstDmaDrmCapsLog) << "DMA_DRM modifier query unavailable (no EGL display or "
                                     "EGL_EXT_image_dma_buf_import_modifiers); offering no DMA_DRM caps";
        return std::string();
    }

    const auto queryFormats =
        reinterpret_cast<PFNEGLQUERYDMABUFFORMATSEXTPROC>(eglGetProcAddress("eglQueryDmaBufFormatsEXT"));
    const auto queryModifiers =
        reinterpret_cast<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>(eglGetProcAddress("eglQueryDmaBufModifiersEXT"));
    if (!queryFormats || !queryModifiers)
        return std::string();

    EGLint numFormats = 0;
    if (queryFormats(dpy, 0, nullptr, &numFormats) != EGL_TRUE || numFormats <= 0)
        return std::string();
    std::vector<EGLint> formats(static_cast<std::size_t>(numFormats));
    if (queryFormats(dpy, numFormats, formats.data(), &numFormats) != EGL_TRUE || numFormats <= 0)
        return std::string();
    formats.resize(static_cast<std::size_t>(numFormats));

    std::string entries;
    int formatCount = 0;
    int modifierCount = 0;
    int excludedCount = 0;
    for (const EGLint fourcc : formats) {
        const char* gstName = gstFormatNameForImportableFourcc(static_cast<uint32_t>(fourcc));
        if (!gstName || !allowed.contains(QByteArray(gstName)))
            continue;

        EGLint numMods = 0;
        if (queryModifiers(dpy, fourcc, 0, nullptr, nullptr, &numMods) != EGL_TRUE || numMods <= 0)
            continue;
        std::vector<EGLuint64KHR> mods(static_cast<std::size_t>(numMods));
        if (queryModifiers(dpy, fourcc, numMods, mods.data(), nullptr, &numMods) != EGL_TRUE || numMods <= 0)
            continue;
        mods.resize(static_cast<std::size_t>(numMods));

        bool counted = false;
        for (const EGLuint64KHR mod : mods) {
            if (!isImportableModifier(mod)) {
                ++excludedCount;
                continue;
            }
            // drm-format wants the DRM fourcc 4CC string, not the GStreamer format name; to_string emits a bare
            // fourcc for LINEAR and "FOURCC:0xMOD" otherwise ("FOURCC:0x0" fails to parse in gst 1.24).
            gchar* drmStr = gst_video_dma_drm_fourcc_to_string(static_cast<guint32>(fourcc), mod);
            if (!drmStr)
                continue;
            if (!entries.empty())
                entries += ", ";
            entries += drmStr;
            g_free(drmStr);
            ++modifierCount;
            counted = true;
        }
        if (counted)
            ++formatCount;
    }

    if (entries.empty())
        return std::string();

    qCInfo(GstDmaDrmCapsLog) << "EGL-derived DMA_DRM caps:" << formatCount << "formats," << modifierCount
                             << "modifiers offered," << excludedCount << "non-importable modifiers excluded";
    return std::string("video/x-raw(memory:DMABuf), format=DMA_DRM, drm-format={ ") + entries + " }; ";
}

std::string buildLinearDmaDrmCaps(const char* gstFormatList)
{
    std::string entries;
    for (const QByteArray& tok : parseFormatTokens(gstFormatList)) {
        const GstVideoFormat fmt = gst_video_format_from_string(tok.constData());
        if (fmt == GST_VIDEO_FORMAT_UNKNOWN)
            continue;
        const guint32 fourcc = gst_video_dma_drm_fourcc_from_format(fmt);
        if (fourcc == 0)  // DRM_FORMAT_INVALID
            continue;
        gchar* drmStr = gst_video_dma_drm_fourcc_to_string(fourcc, 0);  // 0 == DRM_FORMAT_MOD_LINEAR; bare fourcc
        if (!drmStr)
            continue;
        if (!entries.empty())
            entries += ", ";
        entries += drmStr;
        g_free(drmStr);
    }
    if (entries.empty())
        return std::string();
    return std::string("video/x-raw(memory:DMABuf), format=DMA_DRM, drm-format={ ") + entries + " }; ";
}

#ifdef QGC_GST_BUILD_TESTING
bool dmaDrmModifierAdvertisedForTest(guint64 modifier) noexcept
{
    return isImportableModifier(static_cast<EGLuint64KHR>(modifier));
}
#endif

#else

std::string buildSupportedDmaDrmCaps(const char*)
{
    return std::string();
}

std::string buildLinearDmaDrmCaps(const char*)
{
    return std::string();
}

#ifdef QGC_GST_BUILD_TESTING
bool dmaDrmModifierAdvertisedForTest(guint64) noexcept
{
    return false;
}
#endif

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && GST_CHECK_VERSION(1, 24, 0)

}  // namespace GstHw
