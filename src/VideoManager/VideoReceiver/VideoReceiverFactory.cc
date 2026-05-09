#include "VideoReceiverFactory.h"

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif
#include "QtMultimediaReceiver.h"

VideoReceiver *VideoReceiverFactory::createReceiver(QObject *parent)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::createVideoReceiver(parent);
#else
    return QtMultimediaReceiver::createVideoReceiver(parent);
#endif
}

void *VideoReceiverFactory::createSink(QQuickItem *widget, QObject *parent)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::createVideoSink(widget, parent);
#else
    return QtMultimediaReceiver::createVideoSink(widget, parent);
#endif
}

void VideoReceiverFactory::releaseSink(void *sink)
{
#ifdef QGC_GST_STREAMING
    GStreamer::releaseVideoSink(sink);
#else
    QtMultimediaReceiver::releaseVideoSink(sink);
#endif
}
