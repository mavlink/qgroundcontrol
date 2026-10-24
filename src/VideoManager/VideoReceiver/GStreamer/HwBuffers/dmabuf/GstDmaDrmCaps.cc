#include "GstDmaDrmCaps.h"

#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && GST_CHECK_VERSION(1, 24, 0)
#include <algorithm>
#include <drm_fourcc.h>
#include <gst/video/video.h>
#include <map>
#include <vector>

#include "GstDmaBufVideoBuffer.h"
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

// Qt's importer binds one EGLImage per format plane, so compressed layouts and those with auxiliary planes (CCS, DCC,
// AFBC, UBWC) can't be imported. Plain vendor tilings can: the modifier goes with each plane.
bool modifierImportable(guint64 modifier) noexcept
{
    const guint64 code = modifier & 0x00ffffffffffffffULL;
    switch (modifier >> 56) {
        case DRM_FORMAT_MOD_VENDOR_INTEL:
            // X, Y, Yf and 4 tiling; every other Intel modifier is a CCS variant.
            return (code == 1) || (code == 2) || (code == 3) || (code == 9);
        case DRM_FORMAT_MOD_VENDOR_AMD:
            return ((code >> 13) & 0x1) == 0;         // AMD_FMT_MOD DCC bit
        case DRM_FORMAT_MOD_VENDOR_ARM:
            return ((modifier >> 52) & 0xf) == 0x01;  // DRM_FORMAT_MOD_ARM_TYPE_MISC: not AFBC/AFRC compression
        case DRM_FORMAT_MOD_VENDOR_QCOM:
            return code != 1;                         // DRM_FORMAT_MOD_QCOM_COMPRESSED (UBWC)
        default:
            return modifier != DRM_FORMAT_MOD_INVALID;
    }
}

// What GstQtDmaBufImport::planeFourccs() picks for the Qt counterpart of @p format: one R8/GR88 (8-bit) or
// R16/GR1616 (16-bit) image per plane of a multi-plane format, the format's own fourcc for a single-plane one.
guint32 planeImportFourcc(GstVideoFormat format, guint plane) noexcept
{
    const GstVideoFormatInfo* finfo = gst_video_format_get_info(format);
    if (!finfo || (plane >= GST_VIDEO_FORMAT_INFO_N_PLANES(finfo))) {
        return 0;
    }
    if (GST_VIDEO_FORMAT_INFO_N_PLANES(finfo) == 1) {
        return gst_video_dma_drm_fourcc_from_format(format);
    }
    gint components[GST_VIDEO_MAX_COMPONENTS];
    gst_video_format_info_component(finfo, plane, components);
    const int count = static_cast<int>(std::ranges::count_if(components, [](gint c) { return c >= 0; }));
    const int pixelStride = (components[0] >= 0) ? GST_VIDEO_FORMAT_INFO_PSTRIDE(finfo, components[0]) : 0;
    if (count == 1) {
        return (pixelStride == 1) ? DRM_FORMAT_R8 : (pixelStride == 2) ? DRM_FORMAT_R16 : 0;
    }
    if (count == 2) {
        return (pixelStride == 2) ? DRM_FORMAT_GR88 : (pixelStride == 4) ? DRM_FORMAT_GR1616 : 0;
    }
    return 0;
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

/// EGL's modifier list for one fourcc.
class ModifierQuery
{
public:
    explicit ModifierQuery(EGLDisplay display)
        : _display(display)
        , _query(reinterpret_cast<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>(eglGetProcAddress("eglQueryDmaBufModifiersEXT")))
    {}

    bool valid() const noexcept { return _query != nullptr; }

    /// Every modifier EGL imports @p fourcc with.
    std::vector<EGLuint64KHR> all(EGLint fourcc) const { return query(fourcc, false); }

    /// The modifiers @p fourcc imports with as a GL_TEXTURE_2D, the target Qt's importer binds; cached.
    const std::vector<EGLuint64KHR>& sampleable(guint32 fourcc)
    {
        const auto [it, inserted] = _sampleable.try_emplace(fourcc);
        if (inserted) {
            it->second = query(static_cast<EGLint>(fourcc), true);
        }
        return it->second;
    }

private:
    std::vector<EGLuint64KHR> query(EGLint fourcc, bool texture2DOnly) const
    {
        EGLint count = 0;
        if ((_query(_display, fourcc, 0, nullptr, nullptr, &count) != EGL_TRUE) || (count <= 0)) {
            return {};
        }
        std::vector<EGLuint64KHR> mods(static_cast<std::size_t>(count));
        std::vector<EGLBoolean> externalOnly(static_cast<std::size_t>(count));
        if ((_query(_display, fourcc, count, mods.data(), externalOnly.data(), &count) != EGL_TRUE) || (count <= 0)) {
            return {};
        }
        mods.resize(static_cast<std::size_t>(count));
        if (!texture2DOnly) {
            return mods;
        }
        std::vector<EGLuint64KHR> texture2D;
        for (std::size_t i = 0; i < mods.size(); ++i) {
            if (!externalOnly[i]) {
                texture2D.push_back(mods[i]);
            }
        }
        return texture2D;
    }

    EGLDisplay _display;
    PFNEGLQUERYDMABUFMODIFIERSEXTPROC _query;
    std::map<guint32, std::vector<EGLuint64KHR>> _sampleable;
};

}  // namespace

std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat> formats)
{
    if (formats.empty()) {
        return std::string();
    }
    if (GstDmaBufVideoBuffer::importDisabled()) {
        qCDebug(GstDmaDrmCapsLog) << "DMABuf import disabled; offering no DMA_DRM caps";
        return std::string();
    }

    // Only Qt's own EGLDisplay, resolved on the GUI thread: this runs on a streaming or worker thread, where
    // initializing the default display would create a second EGL connection Qt never renders with.
    const EGLDisplay dpy = GstEglHelpers::qtPlatformDisplay();
    if ((dpy == EGL_NO_DISPLAY) || !GstEglHelpers::displaySupportsExtension(dpy, kModifiersExt)) {
        qCDebug(GstDmaDrmCapsLog) << "DMA_DRM modifier query unavailable (Qt renders without EGL, or no "
                                     "EGL_EXT_image_dma_buf_import_modifiers); offering no DMA_DRM caps";
        return std::string();
    }

    const auto queryFormats =
        reinterpret_cast<PFNEGLQUERYDMABUFFORMATSEXTPROC>(eglGetProcAddress("eglQueryDmaBufFormatsEXT"));
    ModifierQuery modifiers(dpy);
    if (!queryFormats || !modifiers.valid()) {
        return std::string();
    }

    EGLint numFormats = 0;
    if ((queryFormats(dpy, 0, nullptr, &numFormats) != EGL_TRUE) || (numFormats <= 0)) {
        return std::string();
    }
    std::vector<EGLint> fourccs(static_cast<std::size_t>(numFormats));
    if ((queryFormats(dpy, numFormats, fourccs.data(), &numFormats) != EGL_TRUE) || (numFormats <= 0)) {
        return std::string();
    }
    fourccs.resize(static_cast<std::size_t>(numFormats));

    std::string entries;
    int offeredCount = 0;
    int excludedCount = 0;
    for (const EGLint fourcc : fourccs) {
        // GStreamer's own fourcc mapping, the one it reads drm-format caps with.
        const GstVideoFormat format = gst_video_dma_drm_fourcc_to_format(static_cast<guint32>(fourcc));
        if (std::ranges::find(formats, format) == formats.end()) {
            continue;
        }
        const guint planes = GST_VIDEO_FORMAT_INFO_N_PLANES(gst_video_format_get_info(format));
        for (const EGLuint64KHR mod : modifiers.all(fourcc)) {
            bool importable = modifierImportable(mod);
            for (guint plane = 0; importable && (plane < planes); ++plane) {
                const guint32 planeFourcc = planeImportFourcc(format, plane);
                importable = (planeFourcc != 0) && std::ranges::any_of(modifiers.sampleable(planeFourcc),
                                                                       [mod](EGLuint64KHR m) { return m == mod; });
            }
            if (importable && appendDrmFormat(entries, static_cast<guint32>(fourcc), mod)) {
                ++offeredCount;
            } else {
                ++excludedCount;
            }
        }
    }

    if (entries.empty()) {
        return std::string();
    }
    qCDebug(GstDmaDrmCapsLog) << "EGL-derived DMA_DRM caps:" << offeredCount << "format/modifier pairs offered,"
                              << excludedCount << "not importable per plane";
    return "video/x-raw(memory:DMABuf), format=DMA_DRM, drm-format={ " + entries + " }; ";
}

#ifdef QGC_UNITTEST_BUILD
bool dmaDrmModifierImportableForTest(guint64 modifier) noexcept
{
    return modifierImportable(modifier);
}

guint32 dmaDrmPlaneImportFourccForTest(GstVideoFormat format, guint plane) noexcept
{
    return planeImportFourcc(format, plane);
}
#endif

#else

std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat>)
{
    return std::string();
}

#ifdef QGC_UNITTEST_BUILD
bool dmaDrmModifierImportableForTest(guint64) noexcept
{
    return false;
}

guint32 dmaDrmPlaneImportFourccForTest(GstVideoFormat, guint) noexcept
{
    return 0;
}
#endif

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && GST_CHECK_VERSION(1, 24, 0)

}  // namespace GstHw
