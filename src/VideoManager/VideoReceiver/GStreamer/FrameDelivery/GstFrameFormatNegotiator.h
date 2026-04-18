#pragma once

#include <QtCore/QLoggingCategory>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gstcaps.h>
#include <gst/video/video-info.h>

Q_DECLARE_LOGGING_CATEGORY(GstFrameFormatNegotiatorLog)

class VideoFrameDelivery;

/// Tracks caps negotiation and cached Qt frame format for an appsink stream.
class GstFrameFormatNegotiator
{
    Q_DISABLE_COPY_MOVE(GstFrameFormatNegotiator)

public:
    struct AllocationInfo
    {
        bool valid = false;
        gsize bufferSize = 0;
        gint fpsN = 0;
        gint fpsD = 0;
    };

    GstFrameFormatNegotiator();
    ~GstFrameFormatNegotiator();

    [[nodiscard]] bool negotiate(GstCaps* caps, VideoFrameDelivery* delivery);
    [[nodiscard]] bool valid() const { return _videoInfoValid; }
    [[nodiscard]] AllocationInfo allocationInfo() const;
    [[nodiscard]] GstVideoInfo videoInfo() const { return _videoInfo; }
    [[nodiscard]] QVideoFrameFormat frameFormat() const { return _cachedFrameFormat; }

    void reset();

private:
    GstVideoInfo _videoInfo{};
    GstCaps* _negotiatedCaps = nullptr;
    bool _videoInfoValid = false;
    QVideoFrameFormat _cachedFrameFormat;
};
