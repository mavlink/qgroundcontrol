#include "VideoSourceCatalog.h"

namespace {

struct SourceMapEntry
{
    VideoSettings::SourceType sourceType;
    const char* sourceName;
    bool isNetworkSource = true;
};

static constexpr SourceMapEntry kSourceMap[] = {
    {VideoSettings::SourceType::Disabled, VideoSettings::videoDisabled, false},
    {VideoSettings::SourceType::NoVideo, VideoSettings::videoSourceNoVideo, false},
    {VideoSettings::SourceType::RTSP, VideoSettings::videoSourceRTSP, true},
    {VideoSettings::SourceType::UDPH264, VideoSettings::videoSourceUDPH264, true},
    {VideoSettings::SourceType::UDPH265, VideoSettings::videoSourceUDPH265, true},
    {VideoSettings::SourceType::TCP, VideoSettings::videoSourceTCP, true},
    {VideoSettings::SourceType::MPEGTS, VideoSettings::videoSourceMPEGTS, true},
    {VideoSettings::SourceType::GstPipeline, VideoSettings::videoSourceGstPipeline, true},
    {VideoSettings::SourceType::Solo3DR, VideoSettings::videoSource3DRSolo, true},
    {VideoSettings::SourceType::ParrotDiscovery, VideoSettings::videoSourceParrotDiscovery, true},
    {VideoSettings::SourceType::YuneecMantisG, VideoSettings::videoSourceYuneecMantisG, true},
    {VideoSettings::SourceType::HerelinkAirUnit, VideoSettings::videoSourceHerelinkAirUnit, true},
    {VideoSettings::SourceType::HerelinkHotspot, VideoSettings::videoSourceHerelinkHotspot, true},
};

const SourceMapEntry* sourceEntryForName(const QString& source)
{
    for (const auto& entry : kSourceMap) {
        if (source == QLatin1String(entry.sourceName))
            return &entry;
    }
    return nullptr;
}

const SourceMapEntry* sourceEntryForType(VideoSettings::SourceType sourceType)
{
    for (const auto& entry : kSourceMap) {
        if (entry.sourceType == sourceType)
            return &entry;
    }
    return nullptr;
}

}  // namespace

VideoSettings::SourceType VideoSourceCatalog::sourceTypeFromString(const QString& source)
{
    if (const SourceMapEntry* entry = sourceEntryForName(source))
        return entry->sourceType;

    return VideoSettings::SourceType::Unknown;
}

QLatin1String VideoSourceCatalog::sourceNameForType(VideoSettings::SourceType sourceType)
{
    if (const SourceMapEntry* entry = sourceEntryForType(sourceType))
        return QLatin1String(entry->sourceName);
    return QLatin1String();
}

bool VideoSourceCatalog::isKnownSourceName(const QString& source)
{
    return sourceEntryForName(source) != nullptr;
}

bool VideoSourceCatalog::isNetworkSourceName(const QString& source)
{
    if (const SourceMapEntry* entry = sourceEntryForName(source))
        return entry->isNetworkSource;
    return false;
}
