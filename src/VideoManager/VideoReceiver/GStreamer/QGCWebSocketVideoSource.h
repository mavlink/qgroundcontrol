#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QUrl>
#include <QtCore/QtGlobal>
#include <memory>

typedef struct _GstElement GstElement;

/// Bridges one-JPEG-per-binary-message WebSocket streams into a bounded GStreamer appsrc.
///
/// The controller owns a dedicated Qt event-loop thread because GstVideoReceiver's worker is a
/// task queue, not a Qt event loop. It does not reconnect; GstVideoReceiver owns that policy.
class QGCWebSocketVideoSource final
{
public:
    static constexpr qsizetype kMaximumJpegBytes = 16 * 1024 * 1024;
    static constexpr int kMaximumJpegDimension = 16384;
    static constexpr quint64 kMaximumDecodedPixels = 64 * 1024 * 1024;

    QGCWebSocketVideoSource(const QUrl& url, GstElement* appsrc);
    ~QGCWebSocketVideoSource();

    Q_DISABLE_COPY_MOVE(QGCWebSocketVideoSource)

    bool start();
    void stop();

    static bool isCompleteJpeg(QByteArrayView message);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
