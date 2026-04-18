#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <gst/app/gstappsink.h>
#include <gst/gstbuffer.h>
#include <gst/gstclock.h>

Q_DECLARE_LOGGING_CATEGORY(GstLatencyTrackerLog)

class VideoFrameDelivery;

/// Records stream latency samples from GStreamer timestamps.
class GstLatencyTracker
{
    Q_DISABLE_COPY_MOVE(GstLatencyTracker)

public:
    GstLatencyTracker() = default;
    ~GstLatencyTracker();

    void record(GstBuffer* buffer, GstAppSink* sink, VideoFrameDelivery* delivery);
    void invalidateClock();
    void reset();

private:
    QMutex _mutex;
    GstClock* _cachedClock = nullptr;
};
