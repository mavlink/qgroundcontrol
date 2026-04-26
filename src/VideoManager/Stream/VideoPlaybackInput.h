#pragma once

#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "VideoReceiver.h"

class QIODevice;

struct VideoPlaybackInput
{
    enum class Kind : quint8
    {
        None,
        DirectUrl,
        StreamDevice,
        LocalCamera,
    };

    Kind kind = Kind::None;
    QString uri;
    QIODevice* device = nullptr;
    QUrl deviceUrl;
    VideoReceiver::PlaybackPolicy playbackPolicy;

    [[nodiscard]] bool isValid() const { return kind != Kind::None && !uri.isEmpty(); }
};
