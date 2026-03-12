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

void prepareEnvironment();
bool initialize();
bool completeInit();
void *createVideoSink(QQuickItem *widget, QObject *parent = nullptr);
void releaseVideoSink(void *sink);
VideoReceiver *createVideoReceiver(QObject *parent = nullptr);

}
