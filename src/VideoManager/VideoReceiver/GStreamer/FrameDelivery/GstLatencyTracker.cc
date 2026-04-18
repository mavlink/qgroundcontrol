#include "GstLatencyTracker.h"

#include <gst/gst.h>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(GstLatencyTrackerLog, "Video.GstLatencyTracker")

GstLatencyTracker::~GstLatencyTracker()
{
    reset();
}

void GstLatencyTracker::record(GstBuffer* buffer, GstAppSink* sink, VideoFrameDelivery* delivery)
{
    if (!buffer || !sink || !delivery)
        return;

    QMutexLocker lock(&_mutex);

    if (!_cachedClock)
        _cachedClock = gst_element_get_clock(GST_ELEMENT(sink));

    bool latencyRecorded = false;

    if (_cachedClock) {
        static GstCaps* ntpCaps = gst_caps_from_string("timestamp/x-ntp");
        GstReferenceTimestampMeta* rtsMeta =
            gst_buffer_get_reference_timestamp_meta(buffer, ntpCaps);
        if (rtsMeta) {
            const GstClockTime nowAbs = gst_clock_get_time(_cachedClock);
            if (nowAbs > rtsMeta->timestamp) {
                const float latMs = static_cast<float>(nowAbs - rtsMeta->timestamp) / GST_MSECOND;
                if (latMs < 30000.0F) {
                    delivery->noteLatencySample(latMs);
                    latencyRecorded = true;
                }
            }
        }
    }

    if (!latencyRecorded && GST_BUFFER_PTS_IS_VALID(buffer) && _cachedClock) {
        const GstClockTime now = gst_clock_get_time(_cachedClock);
        const GstClockTime base = gst_element_get_base_time(GST_ELEMENT(sink));
        if (now <= base) {
            qCDebug(GstLatencyTrackerLog)
                << "Clock reset or pre-base: now=" << now << "base=" << base << "- skipping latency sample";
        } else {
            const GstClockTime running = now - base;
            const GstClockTime pts = GST_BUFFER_PTS(buffer);
            if (running >= pts) {
                const float latMs = static_cast<float>(running - pts) / GST_MSECOND;
                delivery->noteLatencySample(latMs);
            }
        }
    }
}

void GstLatencyTracker::invalidateClock()
{
    QMutexLocker lock(&_mutex);
    if (_cachedClock) {
        gst_object_unref(_cachedClock);
        _cachedClock = nullptr;
    }
}

void GstLatencyTracker::reset()
{
    invalidateClock();
}
