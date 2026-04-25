#include "VideoSourceAvailability.h"

#include "QtMultimediaReceiver.h"
#include "VideoSourceCatalog.h"
#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif

namespace {

bool gstreamerIngestAvailable()
{
#ifdef QGC_GST_STREAMING
    return GStreamer::isAvailable();
#else
    return false;
#endif
}

}  // namespace

QVariantList VideoSourceAvailability::availableSourceNames()
{
    QVariantList sources;

    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::RTSP));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::UDPH264));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::UDPH265));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::TCP));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::MPEGTS));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::Solo3DR));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::ParrotDiscovery));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::YuneecMantisG));

    if (gstreamerIngestAvailable())
        sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::GstPipeline));

    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::HerelinkAirUnit));
    sources.append(VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType::HerelinkHotspot));

    if (QtMultimediaReceiver::localCameraAvailable()) {
        const QStringList uvcDevices = QtMultimediaReceiver::localCameraDeviceNameList();
        for (const QString& device : uvcDevices)
            sources.append(device);
    }

    return sources;
}

VideoSettings::SourceType VideoSourceAvailability::sourceTypeFromString(const QString& source)
{
    if (VideoSourceCatalog::isKnownSourceName(source)) {
        // Preserve catalog as the stable string <-> enum map.
        return VideoSourceCatalog::sourceTypeFromString(source);
    }

    if (QtMultimediaReceiver::localCameraDeviceExists(source))
        return VideoSettings::SourceType::UVC;

    return VideoSettings::SourceType::Unknown;
}

bool VideoSourceAvailability::manualSourceConfigured(VideoSettings::SourceType sourceType,
                                                    const QString& rtspUrl,
                                                    const QString& udpUrl,
                                                    const QString& tcpUrl,
                                                    const QString& gstPipelineUrl)
{
    switch (sourceType) {
        case VideoSettings::SourceType::NoVideo:
        case VideoSettings::SourceType::Disabled:
        case VideoSettings::SourceType::Unknown:
            return false;

        case VideoSettings::SourceType::UDPH264:
        case VideoSettings::SourceType::UDPH265:
        case VideoSettings::SourceType::MPEGTS:
            return !udpUrl.isEmpty();

        case VideoSettings::SourceType::RTSP:
            return !rtspUrl.isEmpty();

        case VideoSettings::SourceType::TCP:
            return !tcpUrl.isEmpty();

        case VideoSettings::SourceType::GstPipeline:
            return !gstPipelineUrl.isEmpty();

        case VideoSettings::SourceType::Solo3DR:
        case VideoSettings::SourceType::ParrotDiscovery:
        case VideoSettings::SourceType::YuneecMantisG:
        case VideoSettings::SourceType::HerelinkAirUnit:
        case VideoSettings::SourceType::HerelinkHotspot:
        case VideoSettings::SourceType::UVC:
            return true;
    }

    return false;
}
