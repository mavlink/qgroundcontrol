#pragma once

class QQuickItem;
class QVideoSink;
class VideoReceiver;

namespace GStreamer
{

enum VideoDecoderOptions {
    ForceVideoDecoderDefault = 0,
    ForceVideoDecoderSoftware,
    ForceVideoDecoderNVIDIA,
    ForceVideoDecoderVAAPI,
    ForceVideoDecoderDirectX3D,
    ForceVideoDecoderVideoToolbox,
    ForceVideoDecoderIntel,
    ForceVideoDecoderVulkan,
    ForceVideoDecoderHardware
};

void prepareEnvironment();
bool initialize();
bool completeInit();
void setDebugLevel(int level);
void *createVideoSink(QQuickItem *widget, QObject *parent = nullptr);
void releaseVideoSink(void *sink);
VideoReceiver *createVideoReceiver(QObject *parent = nullptr);

/// On macOS: connect the appsink inside the sinkbin to a QVideoSink.
/// Returns true on success. No-op (returns false) on other platforms.
bool setupAppleSinkAdapter(void *sinkBin, QVideoSink *videoSink, QObject *adapterParent);

}
