#pragma once

#include <QtCore/QString>
#include <QtCore/QVariantList>

#include "VideoSettings.h"

/// Runtime source availability provider. This is the layer that asks the
/// current build/runtime whether optional sources such as GStreamer pipelines
/// or local cameras are available; VideoSettings remains persistent state.
class VideoSourceAvailability
{
public:
    [[nodiscard]] static QVariantList availableSourceNames();
    [[nodiscard]] static VideoSettings::SourceType sourceTypeFromString(const QString& source);
    [[nodiscard]] static bool manualSourceConfigured(VideoSettings::SourceType sourceType,
                                                     const QString& rtspUrl,
                                                     const QString& udpUrl,
                                                     const QString& tcpUrl,
                                                     const QString& gstPipelineUrl);
};
