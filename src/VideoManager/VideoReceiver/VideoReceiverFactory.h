#pragma once

class QObject;
class QQuickItem;
class VideoReceiver;

/// Backend-neutral video receiver/sink dispatch. Selects between GStreamer and
/// QtMultimedia at compile time via QGC_GST_STREAMING so callers don't need to
/// know which backend is built in.
namespace VideoReceiverFactory
{
VideoReceiver *createReceiver(QObject *parent);
void *createSink(QQuickItem *widget, QObject *parent);
void releaseSink(void *sink);
}
