#include "GstQgcGpuBin.h"

#ifdef QGC_UNITTEST_BUILD
#include <optional>
#endif

#include <QtCore/QByteArray>
#include <QtCore/QtEnvironmentVariables>

#include "GstQgcRenderTarget.h"
#include "QGCLoggingCategory.h"
#ifdef QGC_UNITTEST_BUILD
#include "GstQgcCaps.h"
#endif

QGC_LOGGING_CATEGORY(GstQgcGpuBinLog, "Video.GStreamer.GstQgcGpuBin")

namespace GstQgc {

GpuBinPreference parseGpuBinPreference(QByteArrayView value)
{
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const QByteArray choice = value.trimmed().toByteArray().toLower();
    if (choice == "dmabuf") {
        return {.mode = GpuBinMode::DirectDmaBuf, .explicitChoice = true};
    }
    if (choice == "glupload") {
        return {.mode = GpuBinMode::GlUpload, .explicitChoice = true};
    }
    if (!choice.isEmpty()) {
        qCWarning(GstQgcGpuBinLog) << "Unknown QGC_GST_GPU_BIN value" << choice
                                   << "(expected glupload or dmabuf); using the default glupload bin";
    }
    return {.mode = GpuBinMode::GlUpload};
#else
    Q_UNUSED(value);
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    return {.mode = GpuBinMode::GlUpload};
#elif defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    return {.mode = GpuBinMode::DirectDmaBuf};
#else
    return {};
#endif
#endif
}

GpuBinPreference gpuBinPreference() noexcept
{
#ifdef QGC_UNITTEST_BUILD
    if (const std::optional<SinkEnvironment> environment = sinkEnvironmentForTest()) {
        return environment->gpuBin;
    }
#endif
    static const GpuBinPreference preference = [] {
        const GpuBinPreference parsed = parseGpuBinPreference(qgetenv("QGC_GST_GPU_BIN"));
        qCDebug(GstQgcGpuBinLog).nospace() << "GPU bin preference: " << gpuBinName(parsed.mode) << " ("
                                           << (parsed.explicitChoice ? "QGC_GST_GPU_BIN" : "default") << ")";
        return parsed;
    }();
    return preference;
}

GpuBinMode selectGpuBin(GpuBinPreference preference, const RenderTarget& target) noexcept
{
    bool suited = true;
    switch (preference.mode) {
        case GpuBinMode::GlUpload:
            suited = target.sharesGlContext();
            break;
        case GpuBinMode::DirectDmaBuf:
            suited = target.isOpenGlOnEgl();
            break;
        case GpuBinMode::Passthrough:
            break;
    }
    return (suited || preference.explicitChoice) ? preference.mode : GpuBinMode::Passthrough;
}

const char* gpuBinName(GpuBinMode mode) noexcept
{
    switch (mode) {
        case GpuBinMode::GlUpload:
            return "glupload";
        case GpuBinMode::DirectDmaBuf:
            return "direct DMABuf";
        case GpuBinMode::Passthrough:
            break;
    }
    return "passthrough";
}

}  // namespace GstQgc
