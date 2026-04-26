#pragma once

#include <QtCore/QString>

#include "VideoSettings.h"

/// Stable source-name catalog for video settings.
class VideoSourceCatalog
{
public:
    [[nodiscard]] static VideoSettings::SourceType sourceTypeFromString(const QString& source);
    [[nodiscard]] static QLatin1String sourceNameForType(VideoSettings::SourceType sourceType);
    [[nodiscard]] static bool isKnownSourceName(const QString& source);
    [[nodiscard]] static bool isNetworkSourceName(const QString& source);
};
