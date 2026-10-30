#include "GstQgcGpuBin.h"

#include <atomic>

#include <QtCore/QByteArray>
#include <QtCore/QtEnvironmentVariables>

#include "GstQgcRenderTarget.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstQgcGpuBinLog, "Video.GStreamer.GstQgcGpuBin")

namespace GstQgc {

namespace {

#ifdef QGC_UNITTEST_BUILD
constexpr int kNoTestGpuBinMode = -1;
constinit std::atomic<int> s_testGpuBinMode{kNoTestGpuBinMode};
#endif

}  // namespace

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
    if (const int testMode = s_testGpuBinMode.load(std::memory_order_acquire); testMode != kNoTestGpuBinMode) {
        return {.mode = static_cast<GpuBinMode>(testMode)};
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

#ifdef QGC_UNITTEST_BUILD
ScopedGpuBinModeForTest::ScopedGpuBinModeForTest(GpuBinMode mode)
    : _previous(s_testGpuBinMode.exchange(static_cast<int>(mode), std::memory_order_acq_rel))
{}

ScopedGpuBinModeForTest::~ScopedGpuBinModeForTest()
{
    s_testGpuBinMode.store(_previous, std::memory_order_release);
}
#endif

}  // namespace GstQgc
