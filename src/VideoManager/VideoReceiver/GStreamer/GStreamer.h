#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GStreamerLog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerAPILog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerDecoderRanksLog)

class QQuickItem;
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

// Call prepareEnvironment() on the main thread before dispatching initialize()
// to a worker thread. It sets process-wide env vars that gst_init reads.
void prepareEnvironment();

// Desktop: call initialize() which runs gst_init_check() + post-init steps.
// Android: Java calls gst_init(); call completeInit() after it succeeds.
// Both run on a worker thread — avoid accessing QObject singletons inside.
bool initialize();
bool completeInit();
void *createVideoSink(QQuickItem *widget, QObject *parent = nullptr);
void releaseVideoSink(void *sink);
VideoReceiver *createVideoReceiver(QObject *parent = nullptr);

}
