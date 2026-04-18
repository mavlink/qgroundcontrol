#pragma once

#include <QtCore/QObject>

#include <gst/gst.h>

class QVideoSink;

/// Bridges a GStreamer appsink to a Qt QVideoSink.
///
/// Each decoded frame arriving at the appsink is copied into a QVideoFrame
/// and pushed to the QVideoSink, which renders through Qt's native RHI
/// backend (Metal on macOS, Vulkan/D3D elsewhere).
class GstAppSinkAdapter : public QObject
{
    Q_OBJECT

public:
    explicit GstAppSinkAdapter(QObject *parent = nullptr);
    ~GstAppSinkAdapter() override;

    /// Connect to the named appsink inside @p sinkBin and push frames to @p videoSink.
    /// Returns true on success.
    bool setup(GstElement *sinkBin, QVideoSink *videoSink);

    /// Disconnect the callback (safe to call multiple times).
    void teardown();

private:
    static GstFlowReturn onNewSample(GstElement *appsink, gpointer userData);

    QVideoSink *_videoSink = nullptr;
    GstElement *_appsink = nullptr;
    gulong _signalId = 0;
};
