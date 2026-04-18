#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <atomic>
#include <gst/app/gstappsink.h>
#include <gst/gstelement.h>

#include "GstSampleFrameConverter.h"

Q_DECLARE_LOGGING_CATEGORY(GstAppsinkBridgeLog)

class VideoFrameDelivery;

/// Bridges a GStreamer appsink to Qt's QVideoSink via VideoFrameDelivery.
///
/// Converts GstSamples to QVideoFrames using GstCpuVideoBuffer
/// (zero-copy CPU mapping via gst_video_frame_map).
///
/// The appsink's caps are configured from GstFormatTable to accept all
/// supported CPU formats. HW-accelerated decoding still works upstream
/// (decoders output HW formats, decodebin auto-inserts videoconvert),
/// but frames arrive here as CPU-mapped memory, which Qt's FFmpeg backend
/// uploads to GPU textures via QVideoTextureHelper::createTextures().
class GstAppsinkBridge : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GstAppsinkBridge)

public:
    explicit GstAppsinkBridge(VideoFrameDelivery* delivery, QObject* parent = nullptr);
    ~GstAppsinkBridge() override;

    /// The underlying GstElement (appsink). This adapter owns one ref;
    /// caller may take additional refs (e.g. via GstObjectPtr / gst_bin_add).
    [[nodiscard]] GstElement* element() const;

    /// Disconnect frame delivery before teardown (safe to call multiple times).
    void detach();

private:
    static GstFlowReturn onNewSample(GstAppSink* sink, gpointer userData);
    static gboolean onNewEvent(GstAppSink* sink, gpointer userData);
    static gboolean onProposeAllocation(GstAppSink* sink, GstQuery* query, gpointer userData);
    void handleSample(GstSample* sample);
    GstAppSink* _sink = nullptr;
    QPointer<VideoFrameDelivery> _delivery;
    QMutex _mutex;  // protects _delivery

    /// Sentinel set in ~GstAppsinkBridge before the callback-clear + NULL-state drain.
    /// Plain atomic is sufficient because the destructor (a) clears callbacks via
    /// gst_app_sink_set_callbacks (thread-safe - no new callbacks queue after return),
    /// and (b) forces the sink to GST_STATE_NULL with a bounded wait so any callback
    /// already executing past the sentinel check returns before the member is torn down.
    std::atomic<bool> _destroyed{false};

    GstSampleFrameConverter _converter;

public:
    /// Drop the cached element clock reference. Safe to call from any thread.
    /// Call when the pipeline leaves PLAYING so the next frame re-fetches a fresh clock.
    void invalidateClock() { _converter.invalidateClock(); }
};
