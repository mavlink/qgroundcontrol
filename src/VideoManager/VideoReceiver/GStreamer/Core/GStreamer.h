#pragma once

#include <QtCore/QFuture>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GStreamerLog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerAPILog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerDecoderRanksLog)

class VideoReceiver;

namespace GStreamer {

enum VideoDecoderOptions
{
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

/// Prepare environment, run gst_init asynchronously, register plugins,
/// and apply decoder preferences.  Returns a future that resolves true
/// on success.  Encapsulates prepareEnvironment + initialize +
/// setCodecPriorities so callers never touch GStreamer internals.
QFuture<bool> initAsync(int decoderOption);

/// Low-level init steps — exposed for unit tests only.
void prepareEnvironment();
bool initialize();
bool completeInit();

void setDebugLevel(int level);
VideoReceiver* createVideoReceiver(QObject* parent = nullptr);

}  // namespace GStreamer
