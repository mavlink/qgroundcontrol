#pragma once

#include <QObject>
#include <QQuickItem>

#include "VideoReceiver.h"

class GStreamer {
public:
    static void blacklist(bool forceSoftware = false, bool forceVAAPI = false, bool forceNVIDIA = false, bool forceD3D11 = false);
    static void initialize(int argc, char* argv[], int debuglevel);
    static void* createVideoSink(QObject* parent, QQuickItem* widget);
    static void releaseVideoSink(void* sink);
    static VideoReceiver* createVideoReceiver(QObject* parent);
};
