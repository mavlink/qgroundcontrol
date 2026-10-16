#include "GstDmaDrmCaps.h"

#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
#include <algorithm>
#include <vector>

#include <QtGui/QOpenGLContext>

#include "GstEglHelpers.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>

QGC_LOGGING_CATEGORY(GstDmaDrmCapsLog, "Video.GStreamer.HwBuffers.GstDmaDrmCaps")
#endif

namespace GstHw {

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)

namespace {

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";

// LINEAR-only: tiled external DMABuf from gst-va/iHD can be negotiated and even imported, but binding the resulting
// EGLImage through Qt's render-thread GL path segfaults Mesa/Gallium on this stack. The default glupload path can
// handle those tiled buffers; direct-QGC DMABuf only advertises layouts it can safely bind itself.
bool isImportableModifier(EGLuint64KHR mod) noexcept
{
    return mod == 0;  // DRM_FORMAT_MOD_LINEAR
}

/// Appends the drm-format entry for (@p fourcc, @p modifier) to the comma-separated @p entries; false when GStreamer
/// can't name it.
bool appendDrmFormat(std::string& entries, guint32 fourcc, guint64 modifier)
{
    // drm-format wants the DRM fourcc 4CC string, not the GStreamer format name; to_string emits a bare fourcc for
    // LINEAR and "FOURCC:0xMOD" otherwise ("FOURCC:0x0" fails to parse in gst 1.24).
    const GStreamer::GCharPtr drmFormat = GStreamer::adoptGChar(gst_video_dma_drm_fourcc_to_string(fourcc, modifier));
    if (!drmFormat) {
        return false;
    }
    if (!entries.empty()) {
        entries += ", ";
    }
    entries += drmFormat.get();
    return true;
}

/// The DMA_DRM caps fragment offering @p entries; "" without any.
std::string dmaDrmCaps(const std::string& entries)
{
    if (entries.empty()) {
        return std::string();
    }
    return "video/x-raw(memory:DMABuf), format=DMA_DRM, drm-format={ " + entries + " }; ";
}

}  // namespace

std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat> formats)
{
    if (formats.empty())
        return std::string();

    // Prefer the display of Qt's EGL share context: Qt initialized it and owns its lifetime. Without one (GLX, or no
    // share context yet) this worker thread has no current display either, so initialize the default display
    // ourselves; modifiers are GPU-global. That one is never eglTerminate'd.
    EGLDisplay dpy = GstEglHelpers::resolveEglDisplay(QOpenGLContext::globalShareContext());
    if (dpy == EGL_NO_DISPLAY) {
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (dpy != EGL_NO_DISPLAY && eglInitialize(dpy, nullptr, nullptr) != EGL_TRUE) {
            dpy = EGL_NO_DISPLAY;
        }
    }
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
    std::vector<EGLint> fourccs(static_cast<std::size_t>(numFormats));
    if (queryFormats(dpy, numFormats, fourccs.data(), &numFormats) != EGL_TRUE || numFormats <= 0)
        return std::string();
    fourccs.resize(static_cast<std::size_t>(numFormats));

    std::string entries;
    int formatCount = 0;
    int modifierCount = 0;
    int excludedCount = 0;
    for (const EGLint fourcc : fourccs) {
        // GStreamer's own fourcc mapping, the one it reads drm-format caps with.
        const GstVideoFormat format = gst_video_dma_drm_fourcc_to_format(static_cast<guint32>(fourcc));
        if (std::ranges::find(formats, format) == formats.end())
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
            if (appendDrmFormat(entries, static_cast<guint32>(fourcc), mod)) {
                ++modifierCount;
                counted = true;
            }
        }
        if (counted)
            ++formatCount;
    }

    if (entries.empty())
        return std::string();

    qCInfo(GstDmaDrmCapsLog) << "EGL-derived DMA_DRM caps:" << formatCount << "formats," << modifierCount
                             << "modifiers offered," << excludedCount << "non-importable modifiers excluded";
    return dmaDrmCaps(entries);
}

std::string buildLinearDmaDrmCaps(std::span<const GstVideoFormat> formats)
{
    std::string entries;
    for (const GstVideoFormat fmt : formats) {
        const guint32 fourcc = gst_video_dma_drm_fourcc_from_format(fmt);
        if (fourcc != 0) {                               // DRM_FORMAT_INVALID
            (void) appendDrmFormat(entries, fourcc, 0);  // 0 == DRM_FORMAT_MOD_LINEAR
        }
    }
    return dmaDrmCaps(entries);
}

#ifdef QGC_UNITTEST_BUILD
bool dmaDrmModifierAdvertisedForTest(guint64 modifier) noexcept
{
    return isImportableModifier(static_cast<EGLuint64KHR>(modifier));
}
#endif

#else

std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat>)
{
    return std::string();
}

std::string buildLinearDmaDrmCaps(std::span<const GstVideoFormat>)
{
    return std::string();
}

#ifdef QGC_UNITTEST_BUILD
bool dmaDrmModifierAdvertisedForTest(guint64) noexcept
{
    return false;
}
#endif

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && GST_CHECK_VERSION(1, 24, 0)

}  // namespace GstHw
