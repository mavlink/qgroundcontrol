#include "VideoReceiver.h"

#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoReceiverLog, "Video.VideoReceiver")

void VideoReceiver::takeScreenshot(const QString& imageFile)
{
    // Every backend renders into the sink of the widget's VideoOutput, so its frame is the one on display.
    QVideoSink* const videoSink = _widget ? _widget->property("videoSink").value<QVideoSink*>() : nullptr;
    if (!videoSink) {
        qCWarning(VideoReceiverLog) << "No video sink to take a screenshot from" << _name;
        emit onTakeScreenshotComplete(STATUS_FAIL);
        return;
    }

    // toImage() maps a GPU-backed frame back to the CPU.
    const QVideoFrame frame = videoSink->videoFrame();
    const QImage image = frame.isValid() ? frame.toImage() : QImage();
    if (image.isNull()) {
        qCWarning(VideoReceiverLog) << "No video frame to take a screenshot from" << _name;
        emit onTakeScreenshotComplete(STATUS_FAIL);
        return;
    }

    if (!image.save(imageFile)) {
        qCWarning(VideoReceiverLog) << "Failed to save screenshot" << imageFile;
        emit onTakeScreenshotComplete(STATUS_FAIL);
        return;
    }

    qCDebug(VideoReceiverLog) << "Screenshot saved:" << imageFile;
    emit onTakeScreenshotComplete(STATUS_OK);
}
