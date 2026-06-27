#include "GraphicsSetup.h"

#include <QtCore/QDir>
#include <QtCore/QRunnable>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQuick/QQuickGraphicsConfiguration>
#include <QtQuick/QQuickGraphicsDevice>
#include <QtQuick/QQuickWindow>
#include <memory>
#include <rhi/qrhi.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GraphicsSetupLog, "API.GraphicsSetup")

namespace GraphicsSetup {

namespace {

constexpr const char* kEnvRhiDebug = "QGC_RHI_DEBUG";
constexpr const char* kEnvRhiPipelineCache = "QGC_RHI_PIPELINE_CACHE";
constexpr const char* kEnvHdrOutput = "QGC_HDR_OUTPUT";
constexpr const char* kEnvForceVideoGpu = "QGC_FORCE_VIDEO_GPU";

bool envFlag(const char* name)
{
    return qEnvironmentVariableIsSet(name) && (qEnvironmentVariable(name) != QLatin1String("0"));
}

bool envEnabledByDefault(const char* name)
{
    return !qEnvironmentVariableIsSet(name) || (qEnvironmentVariable(name) != QLatin1String("0"));
}

void configurePipelineCache(QQuickGraphicsConfiguration& config)
{
    if (!envEnabledByDefault(kEnvRhiPipelineCache)) {
        return;
    }

    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) {
        qCWarning(GraphicsSetupLog) << "RHI pipeline cache disabled: no writable cache location";
        return;
    }

    QDir dir;
    if (!dir.mkpath(cacheDir)) {
        qCWarning(GraphicsSetupLog) << "RHI pipeline cache disabled: failed to create" << cacheDir;
        return;
    }

    const QString cacheFile = cacheDir + QStringLiteral("/qgc_rhi_pipeline.cache");
    config.setPipelineCacheLoadFile(cacheFile);
    config.setPipelineCacheSaveFile(cacheFile);
    qCDebug(GraphicsSetupLog) << "RHI pipeline cache:" << cacheFile;
}

#if defined(Q_OS_WIN)
bool parseHex32(const QString& spec, quint32& value)
{
    QString token = spec.trimmed();
    if (token.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        token.remove(0, 2);
    }

    if (token.isEmpty()) {
        return false;
    }

    bool ok = false;
    value = token.toUInt(&ok, 16);
    return ok;
}

qint32 signExtend32(quint32 value)
{
    const qint64 signedValue =
        (value <= 0x7FFFFFFFU) ? static_cast<qint64>(value) : (static_cast<qint64>(value) - 0x100000000LL);
    return static_cast<qint32>(signedValue);
}

bool parseLuid(const QString& spec, quint32& low, qint32& high)
{
    const QStringList parts = spec.split(QLatin1Char(':'));
    if (parts.size() != 2) {
        return false;
    }

    quint32 highRaw = 0;
    if (!parseHex32(parts.at(0), low) || !parseHex32(parts.at(1), highRaw)) {
        return false;
    }

    high = signExtend32(highRaw);
    return true;
}

void configureForcedD3DAdapter(QQuickWindow* window)
{
    if (!qEnvironmentVariableIsSet(kEnvForceVideoGpu)) {
        return;
    }

    const QString spec = qEnvironmentVariable(kEnvForceVideoGpu);
    quint32 luidLow = 0;
    qint32 luidHigh = 0;
    if (!parseLuid(spec, luidLow, luidHigh)) {
        qCWarning(GraphicsSetupLog) << "QGC_FORCE_VIDEO_GPU malformed (expected hex 'low:high'):" << spec;
        return;
    }

    // Must run before QRhi creation; Qt Quick owns the adapter after scene graph init.
    window->setGraphicsDevice(QQuickGraphicsDevice::fromAdapter(luidLow, luidHigh));
    qCWarning(GraphicsSetupLog).nospace()
        << "QGC_FORCE_VIDEO_GPU: forcing adapter LUID low=0x" << QString::number(luidLow, 16) << " high=0x"
        << QString::number(static_cast<quint32>(luidHigh), 16);
}
#else
void warnForcedGpuUnsupported()
{
    if (qEnvironmentVariableIsSet(kEnvForceVideoGpu)) {
        qCWarning(GraphicsSetupLog)
            << "QGC_FORCE_VIDEO_GPU is Windows/D3D-only; ignoring LUID" << qEnvironmentVariable(kEnvForceVideoGpu);
    }
}
#endif

const char* hdrFormatName(QRhiSwapChain::Format f)
{
    switch (f) {
        case QRhiSwapChain::SDR:
            return "SDR";
        case QRhiSwapChain::HDRExtendedSrgbLinear:
            return "HDRExtendedSrgbLinear";
        case QRhiSwapChain::HDR10:
            return "HDR10";
        case QRhiSwapChain::HDRExtendedDisplayP3Linear:
            return "HDRExtendedDisplayP3Linear";
    }
    return "Unknown";
}

void probeHdr(QQuickWindow* window)
{
    QRhi* rhi = window->rhi();
    if (!rhi) {
        qCWarning(GraphicsSetupLog) << "HDR probe: no QRhi on render thread";
        return;
    }

    std::unique_ptr<QRhiSwapChain> sc(rhi->newSwapChain());
    if (!sc) {
        qCWarning(GraphicsSetupLog) << "HDR probe: backend has no swapchain support";
        return;
    }
    sc->setWindow(window);

    const QRhiSwapChainHdrInfo info = sc->hdrInfo();
    qCDebug(GraphicsSetupLog).nospace()
        << "HDR display info: limitsType=" << static_cast<int>(info.limitsType)
        << " luminanceBehavior=" << static_cast<int>(info.luminanceBehavior) << " sdrWhiteLevel=" << info.sdrWhiteLevel;

    for (const QRhiSwapChain::Format f :
         {QRhiSwapChain::HDRExtendedSrgbLinear, QRhiSwapChain::HDR10, QRhiSwapChain::HDRExtendedDisplayP3Linear}) {
        qCDebug(GraphicsSetupLog) << "HDR format" << hdrFormatName(f) << "supported:" << sc->isFormatSupported(f);
    }

    if (envFlag(kEnvHdrOutput)) {
        qCWarning(GraphicsSetupLog)
            << "QGC_HDR_OUTPUT set but Qt Quick exposes no public swapchain-format setter in this"
            << "Qt version; staying on SDR. (Diagnostic only.)";
    }
}

}  // namespace

void configureMainWindow(QQuickWindow* window)
{
    if (!window) {
        qCWarning(GraphicsSetupLog) << "configureMainWindow: null window";
        return;
    }

    if (window->isSceneGraphInitialized()) {
        qCWarning(GraphicsSetupLog)
            << "Scene graph already initialized before configureMainWindow; skipping RHI config";
    } else {
        QQuickGraphicsConfiguration config = window->graphicsConfiguration();

        if (envFlag(kEnvRhiDebug)) {
            config.setDebugLayer(true);
            config.setDebugMarkers(true);
            config.setTimestamps(true);
            qCDebug(GraphicsSetupLog) << "RHI debug layer/markers/timestamps enabled";
        }

        configurePipelineCache(config);
        window->setGraphicsConfiguration(config);

#if defined(Q_OS_WIN)
        configureForcedD3DAdapter(window);
#else
        warnForcedGpuUnsupported();
#endif
    }

    if (!envFlag(kEnvRhiDebug) && !envFlag(kEnvHdrOutput)) {
        return;
    }
    QQuickWindow* win = window;
    if (win->isSceneGraphInitialized()) {
        win->scheduleRenderJob(QRunnable::create([win]() { probeHdr(win); }), QQuickWindow::BeforeSynchronizingStage);
        win->update();
    } else {
        QObject::connect(
            win, &QQuickWindow::sceneGraphInitialized, win, [win]() { probeHdr(win); }, Qt::DirectConnection);
    }
}

}  // namespace GraphicsSetup
