#include "VideoReceiverFactory.h"

#include "QGCCorePlugin.h"
#include "VideoBackendRegistry.h"
#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

namespace VideoReceiverFactory {

VideoReceiver* forSource(const VideoSourceResolver::VideoSource& source,
                         [[maybe_unused]] bool thermal,
                         QObject* parent)
{
    // `thermal` is intentionally not consulted yet: backend selection is
    // currently identical for Primary and Thermal streams. It is kept in the
    // signature as the future-hook for thermal-specific dispatch (e.g. FLIR
    // RJPEG, thermal-only caps filters, or a dedicated decoder pipeline) —
    // do NOT remove it to silence the warning.

    if (auto factory = VideoBackendRegistry::instance().factoryForSource(source)) {
        if (source.usesGStreamer() || source.isLocalCamera)
            return factory(parent);
    }

    // Preserve the QGCCorePlugin::createVideoReceiver extension point: custom
    // builds override the virtual to inject a proprietary QtMultimedia-style
    // receiver. Consult it here before falling back to the registered default.
    if (auto* core = QGCCorePlugin::instance()) {
        if (auto* override = core->createVideoReceiver(parent)) {
            return override;
        }
    }

    if (auto qtFactory = VideoBackendRegistry::instance().factory(VideoReceiver::BackendKind::QtMultimedia))
        return qtFactory(parent);
    return nullptr;  // QtMm always registers at startup, but guard defensively
}

VideoReceiver* gstOrQtMultimedia(const VideoSourceResolver::VideoSource& source, bool thermal, QObject* parent)
{
    return forSource(source, thermal, parent);
}

VideoReceiver* uvc(const VideoSourceResolver::VideoSource& source, bool thermal, QObject* parent)
{
    Q_UNUSED(source);
    Q_UNUSED(thermal);
    if (auto uvcFactory = VideoBackendRegistry::instance().factory(VideoReceiver::BackendKind::UVC))
        return uvcFactory(parent);
    return nullptr;
}

}  // namespace VideoReceiverFactory
