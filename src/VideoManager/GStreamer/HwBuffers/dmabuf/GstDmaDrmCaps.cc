#include "GstDmaDrmCaps.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <algorithm>
#include <drm_fourcc.h>
#include <gst/video/video-info-dma.h>
#include <gst/video/video.h>
#include <map>
#include <vector>

#include "GstDmaBufVulkanImport.h"
#include "GstEglHelpers.h"
#include "GstQtDmaBufImport.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>

QGC_LOGGING_CATEGORY(GstDmaDrmCapsLog, "Video.GStreamer.HwBuffers.GstDmaDrmCaps")

namespace GstHw {

namespace {

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

// The DRM fourcc the importer samples plane @p plane of a @p pixelFormat frame through; 0 when it imports no such
// plane.
guint32 planeImportFourcc(QVideoFrameFormat::PixelFormat pixelFormat, guint plane)
{
    const GstQtDmaBufImport::PlaneFourccs fourccs = GstQtDmaBufImport::planeFourccs(pixelFormat);
    return (plane < static_cast<guint>(fourccs.count)) ? fourccs.fourcc[plane] : 0;
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

std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat> formats, QtPixelFormatOf pixelFormatOf,
                                     void* display)
{
    if (formats.empty() || !pixelFormatOf) {
        return std::string();
    }

    const auto dpy = static_cast<EGLDisplay>(display);
    if ((dpy == EGL_NO_DISPLAY) || !GstEglHelpers::displaySupportsExtension(dpy, GstEglHelpers::kDmaBufModifiersExt)) {
        qCDebug(GstDmaDrmCapsLog) << "DMA_DRM modifier query unavailable (no DMABuf import display, or no "
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
                const guint32 planeFourcc = planeImportFourcc(pixelFormatOf(format), plane);
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

std::string buildVulkanDmaDrmCaps([[maybe_unused]] std::span<const GstVideoFormat> formats,
                                  [[maybe_unused]] QtPixelFormatOf pixelFormatOf)
{
    std::string entries;
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    if (!pixelFormatOf) {
        return std::string();
    }
    for (const GstVideoFormat format : formats) {
        const guint32 fourcc = gst_video_dma_drm_fourcc_from_format(format);
        const guint planes = GST_VIDEO_FORMAT_INFO_N_PLANES(gst_video_format_get_info(format));
        if ((fourcc == DRM_FORMAT_INVALID) || (planes == 0)) {
            continue;
        }
        // The modifiers the first plane imports with, kept when every other plane imports with them too.
        const QVideoFrameFormat::PixelFormat pixelFormat = pixelFormatOf(format);
        for (const quint64 modifier : GstDmaBufVulkan::importableModifiers(planeImportFourcc(pixelFormat, 0))) {
            bool importable = modifierImportable(modifier);
            for (guint plane = 1; importable && (plane < planes); ++plane) {
                const std::vector<quint64> planeModifiers =
                    GstDmaBufVulkan::importableModifiers(planeImportFourcc(pixelFormat, plane));
                importable = std::ranges::find(planeModifiers, modifier) != planeModifiers.end();
            }
            if (importable) {
                appendDrmFormat(entries, fourcc, modifier);
            }
        }
    }
#endif
    if (entries.empty()) {
        return std::string();
    }
    return "video/x-raw(memory:DMABuf), format=DMA_DRM, drm-format={ " + entries + " }; ";
}

#ifdef QGC_UNITTEST_BUILD
bool dmaDrmModifierImportableForTest(guint64 modifier) noexcept
{
    return modifierImportable(modifier);
}
#endif

}  // namespace GstHw

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
