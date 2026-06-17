#include "QGCGraphicsSetup.h"

#include <QtCore/QDir>
#include <QtCore/QRunnable>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickGraphicsConfiguration>
#include <QtQuick/QQuickGraphicsDevice>
#include <QtQuick/QQuickWindow>
#include <memory>
#include <rhi/qrhi.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCGraphicsSetupLog, "API.QGCGraphicsSetup")

namespace QGCGraphicsSetup {

namespace {

bool envFlag(const char* name)
{
    return qEnvironmentVariableIsSet(name) && (qEnvironmentVariable(name) != QLatin1String("0"));
}

// (#5) Parse "low:high" hex LUID from QGC_FORCE_VIDEO_GPU into a DXGI adapter LUID pair.
bool parseLuid(const QString& spec, quint32& low, qint32& high)
{
    const QStringList parts = spec.split(QLatin1Char(':'));
    if (parts.size() != 2) {
        return false;
    }
    bool okLow = false;
    bool okHigh = false;
    low = parts.at(0).toUInt(&okLow, 16);
    high = static_cast<qint32>(parts.at(1).toInt(&okHigh, 16));
    return okLow && okHigh;
}

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
        default:
            return "Unknown";
    }
}

// (#1) Diagnostic-only HDR probe. Runs on the render thread where window->rhi() is valid. Creates a
// throwaway, non-resized swapchain bound to the window solely to query isFormatSupported()/hdrInfo()
// (both documented as callable before createOrResize() once the window is set).
void probeHdr(QQuickWindow* window)
{
    QRhi* rhi = window->rhi();
    if (!rhi) {
        qCWarning(QGCGraphicsSetupLog) << "HDR probe: no QRhi on render thread";
        return;
    }

    std::unique_ptr<QRhiSwapChain> sc(rhi->newSwapChain());
    if (!sc) {
        qCWarning(QGCGraphicsSetupLog) << "HDR probe: backend has no swapchain support";
        return;
    }
    sc->setWindow(window);

    const QRhiSwapChainHdrInfo info = sc->hdrInfo();
    qCDebug(QGCGraphicsSetupLog).nospace()
        << "HDR display info: limitsType=" << static_cast<int>(info.limitsType)
        << " luminanceBehavior=" << static_cast<int>(info.luminanceBehavior) << " sdrWhiteLevel=" << info.sdrWhiteLevel;

    for (const QRhiSwapChain::Format f :
         {QRhiSwapChain::HDRExtendedSrgbLinear, QRhiSwapChain::HDR10, QRhiSwapChain::HDRExtendedDisplayP3Linear}) {
        qCDebug(QGCGraphicsSetupLog) << "HDR format" << hdrFormatName(f) << "supported:" << sc->isFormatSupported(f);
    }

    // (#1b) Qt Quick owns its swapchain internally and exposes no public per-window API in 6.10 to
    // set its HDR format (verified: QQuickWindow has no setSwapChainFormat / HDR-format setter).
    // Even if it did, Qt Quick / QtMultimedia HDR passthrough still tone-maps upstream. So when the
    // user requests HDR we can only report the gap rather than enable it.
    if (envFlag("QGC_HDR_OUTPUT")) {
        qCWarning(QGCGraphicsSetupLog)
            << "QGC_HDR_OUTPUT set but Qt Quick exposes no public swapchain-format setter in this"
            << "Qt version; staying on SDR. (Diagnostic only.)";
    }
}

}  // namespace

void configureMainWindow(QQuickWindow* window)
{
    if (!window) {
        qCWarning(QGCGraphicsSetupLog) << "configureMainWindow: null window";
        return;
    }

    // Graphics config and forced device MUST precede SG init. If the SG is already live we cannot
    // safely apply them, so warn and skip rather than silently no-op.
    if (window->isSceneGraphInitialized()) {
        qCWarning(QGCGraphicsSetupLog)
            << "Scene graph already initialized before configureMainWindow; skipping RHI config";
    } else {
        QQuickGraphicsConfiguration config = window->graphicsConfiguration();

        // (#2) Debug layer / markers / GPU timestamps. Off by default; zero-cost in release.
        if (envFlag("QGC_RHI_DEBUG")) {
            config.setDebugLayer(true);
            config.setDebugMarkers(true);
            config.setTimestamps(true);
            qCDebug(QGCGraphicsSetupLog) << "RHI debug layer/markers/timestamps enabled";
        }

        // (#3) Pipeline cache warm-start. On by default (low risk); QGC_RHI_PIPELINE_CACHE=0 opts out.
        if (!qEnvironmentVariableIsSet("QGC_RHI_PIPELINE_CACHE") ||
            qEnvironmentVariable("QGC_RHI_PIPELINE_CACHE") != QLatin1String("0")) {
            const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
            if (!cacheDir.isEmpty()) {
                QDir().mkpath(cacheDir);
                const QString cacheFile = cacheDir + QStringLiteral("/qgc_rhi_pipeline.cache");
                config.setPipelineCacheLoadFile(cacheFile);
                config.setPipelineCacheSaveFile(cacheFile);
                qCDebug(QGCGraphicsSetupLog) << "RHI pipeline cache:" << cacheFile;
            }
        }

        window->setGraphicsConfiguration(config);

        // (#5) Power-user escape hatch: force Qt Quick onto a specific D3D adapter. WHY this must run
        // pre-SG-init: setGraphicsDevice adopts an existing device, so the scene graph never gets to
        // choose its adapter -- doing this after SG init has no effect, and choosing the wrong
        // adapter breaks the entire UI render. The LUID must come from outside QGCRhiCapture (whose
        // snapshot only exists post-SG-init), hence a user-provided LUID via the env var.
        if (qEnvironmentVariableIsSet("QGC_FORCE_VIDEO_GPU")) {
            const QString spec = qEnvironmentVariable("QGC_FORCE_VIDEO_GPU");
            quint32 luidLow = 0;
            qint32 luidHigh = 0;
            if (parseLuid(spec, luidLow, luidHigh)) {
#if defined(Q_OS_WIN)
                // fromAdapter() is a Windows/DXGI-only factory (D3D11/D3D12). On other platforms
                // there is no LUID-based adapter selection, so the env var is a no-op there.
                window->setGraphicsDevice(QQuickGraphicsDevice::fromAdapter(luidLow, luidHigh));
                qCWarning(QGCGraphicsSetupLog).nospace()
                    << "QGC_FORCE_VIDEO_GPU: forcing adapter LUID low=0x" << QString::number(luidLow, 16) << " high=0x"
                    << QString::number(luidHigh, 16);
#else
                qCWarning(QGCGraphicsSetupLog) << "QGC_FORCE_VIDEO_GPU is Windows/D3D-only; ignoring LUID" << spec;
#endif
            } else {
                qCWarning(QGCGraphicsSetupLog) << "QGC_FORCE_VIDEO_GPU malformed (expected hex 'low:high'):" << spec;
            }
        }
    }

    // (#1a) HDR diagnostic must run where rhi() is valid -- defer to a render job. Gated to dev/HDR
    // requests so normal launches skip the throwaway probe swapchain and its log noise.
    if (!envFlag("QGC_RHI_DEBUG") && !envFlag("QGC_HDR_OUTPUT")) {
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

}  // namespace QGCGraphicsSetup
