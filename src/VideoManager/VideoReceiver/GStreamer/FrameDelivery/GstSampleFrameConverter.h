#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtMultimedia/QVideoFrame>
#include <atomic>
#include <optional>
#include <gst/app/gstappsink.h>
#include <gst/gstevent.h>

#include "GstFrameFormatNegotiator.h"
#include "GstLatencyTracker.h"

Q_DECLARE_LOGGING_CATEGORY(GstSampleFrameConverterLog)

class VideoFrameDelivery;

/// Converts GStreamer samples/events into Qt video frames.
///
/// GstAppsinkBridge owns the appsink callbacks and teardown. This converter
/// owns caps parsing, cached QVideoFrameFormat construction, latency sampling,
/// orientation tags, crop handling, and GstBuffer-to-QVideoFrame wrapping.
class GstSampleFrameConverter
{
    Q_DISABLE_COPY_MOVE(GstSampleFrameConverter)

public:
    using AllocationInfo = GstFrameFormatNegotiator::AllocationInfo;

    GstSampleFrameConverter();
    ~GstSampleFrameConverter();

    [[nodiscard]] AllocationInfo allocationInfo() const;
    [[nodiscard]] std::optional<QVideoFrame> convert(GstSample* sample,
                                                     GstAppSink* sink,
                                                     VideoFrameDelivery* delivery);
    void handleTagEvent(GstEvent* event);
    void invalidateClock();
    void reset();

private:
    mutable QMutex _mutex;
    GstFrameFormatNegotiator _formatNegotiator;
    GstLatencyTracker _latencyTracker;
    std::atomic<bool> _mirrored{false};
    std::atomic<int> _rotation{0};  // QtVideo::Rotation
};
