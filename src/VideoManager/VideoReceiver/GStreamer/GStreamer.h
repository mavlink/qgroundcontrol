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

/// Connect the appsink inside @p sinkBin to @p videoSink. Returns true on success.
bool setupAppSinkAdapter(void *sinkBin, QVideoSink *videoSink, QObject *adapterParent);

/// Toggle every appsink adapter parented under @p adapterParent. Used to drop frames at
/// the appsink while the host window is hidden/minimized — saves CPU vs. running the
/// full decode→render path against a non-visible sink. Safe to call repeatedly; no-op
/// when no adapters exist.
void setAppSinkAdaptersActive(QObject *adapterParent, bool active);

}
