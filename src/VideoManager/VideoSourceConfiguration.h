#pragma once

#include <QtCore/QString>

struct VideoSourceConfiguration
{
    bool uriOverrideEnabled = false;
    QString uri;
    bool forceRtspTcp = false;

    bool operator==(const VideoSourceConfiguration&) const = default;
};
