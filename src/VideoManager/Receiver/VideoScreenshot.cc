#include "VideoScreenshot.h"

#include <QtGui/QImage>
#include <QtGui/QTransform>
#include <QtMultimedia/QVideoFrame>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoScreenshotLog, "Video.VideoScreenshot")

QImage VideoScreenshot::displayedImage(const QVideoFrame& frame)
{
    if (!frame.isValid()) {
        return {};
    }

    // toImage() applies only the surface format's own transformation: not the frame's rotation or mirroring, nor the
    // viewport that crops decoder padding.
    QImage image = frame.toImage();
    if (image.isNull()) {
        return image;
    }

    const QRect viewport = frame.surfaceFormat().viewport();
    if (viewport.isValid() && (image.size() == frame.size()) && (viewport != image.rect())) {
        image = image.copy(viewport.intersected(image.rect()));
    }

    // Same order as Qt's presentation (qNormalizedFrameTransformation): rotate, then mirror horizontally.
    const int degrees = static_cast<int>(frame.rotation());
    if (degrees != 0) {
        image = image.transformed(QTransform().rotate(degrees));
    }
    if (frame.mirrored()) {
        image = image.flipped(Qt::Horizontal);
    }
    return image;
}

bool VideoScreenshot::save(const QVideoFrame& frame, const QString& file)
{
    const QImage image = displayedImage(frame);
    if (image.isNull()) {
        qCWarning(VideoScreenshotLog) << "No video frame to take a screenshot from";
        return false;
    }

    if (!image.save(file)) {
        qCWarning(VideoScreenshotLog) << "Failed to save screenshot" << file;
        return false;
    }

    qCDebug(VideoScreenshotLog) << "Screenshot saved:" << file;
    return true;
}
