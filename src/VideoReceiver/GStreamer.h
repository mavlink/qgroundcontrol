#pragma once

#include <QtCore/QLoggingCategory>
#include <QQuickItem>

#include "Settings/VideoDecoderOptions.h"
#include "VideoReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(GStreamerLog)
Q_DECLARE_LOGGING_CATEGORY(GStreamerAPILog)

class GStreamer {
public:
    static void blacklist(VideoDecoderOptions option);
    static void initialize(int argc, char* argv[], int debuglevel);
    static void* createVideoSink(QObject* parent, QQuickItem* widget);
    static void releaseVideoSink(void* sink);
    static VideoReceiver* createVideoReceiver(QObject* parent);
};
