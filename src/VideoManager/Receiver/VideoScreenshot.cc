#include "VideoScreenshot.h"

#include <QtCore/QBuffer>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtGui/QImage>
#include <QtGui/QTransform>
#include <QtMultimedia/QVideoFrame>

#include "ExifParser.h"
#include "ExifUtility.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoScreenshotLog, "Video.VideoScreenshot")

namespace {

bool isJpegFile(const QString& file)
{
    const QString suffix = QFileInfo(file).suffix();
    return (suffix.compare(QStringLiteral("jpg"), Qt::CaseInsensitive) == 0) ||
           (suffix.compare(QStringLiteral("jpeg"), Qt::CaseInsensitive) == 0);
}

/// Adds @p exif to the JPEG in @p jpeg.
bool addExif(QByteArray& jpeg, const VideoScreenshot::Exif& exif)
{
    if (exif.position.isValid()) {
        GeoTagData geoTag;
        geoTag.coordinate = exif.position;
        if (!ExifParser::write(jpeg, geoTag)) {
            return false;
        }
    }
    if (exif.taken.isValid()) {
        ExifData* data = ExifUtility::loadFromBuffer(jpeg);
        if (!data) {
            data = ExifUtility::createNew();
        }
        const bool written =
            data && ExifUtility::writeDateTimeOriginal(data, exif.taken) && ExifUtility::saveToBuffer(data, jpeg);
        if (data) {
            exif_data_unref(data);
        }
        if (!written) {
            return false;
        }
    }
    return true;
}

}  // namespace

QImage VideoScreenshot::displayedImage(const QVideoFrame& frame, QtVideo::Rotation outputRotation, bool outputMirrored)
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

    // Same order as Qt's presentation (qNormalizedFrameTransformation): the frame's rotation and mirroring, then the
    // output's.
    const auto rotateAndMirror = [&image](QtVideo::Rotation rotation, bool mirrored) {
        if (const int degrees = static_cast<int>(rotation); degrees != 0) {
            image = image.transformed(QTransform().rotate(degrees));
        }
        if (mirrored) {
            image = image.flipped(Qt::Horizontal);
        }
    };
    rotateAndMirror(frame.rotation(), frame.mirrored());
    rotateAndMirror(outputRotation, outputMirrored);
    return image;
}

bool VideoScreenshot::save(const QVideoFrame& frame, const QString& file, const Exif& exif,
                           QtVideo::Rotation outputRotation, bool outputMirrored)
{
    const QImage image = displayedImage(frame, outputRotation, outputMirrored);
    if (image.isNull()) {
        qCWarning(VideoScreenshotLog) << "No video frame to take a screenshot from";
        return false;
    }

    if (!isJpegFile(file) || (!exif.taken.isValid() && !exif.position.isValid())) {
        if (!image.save(file)) {
            qCWarning(VideoScreenshotLog) << "Failed to save screenshot" << file;
            return false;
        }
        qCDebug(VideoScreenshotLog) << "Screenshot saved:" << file;
        return true;
    }

    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "JPG")) {
        qCWarning(VideoScreenshotLog) << "Failed to encode screenshot" << file;
        return false;
    }
    buffer.close();
    // The picture matters more than its tags.
    if (!addExif(jpeg, exif)) {
        qCWarning(VideoScreenshotLog) << "Failed to add EXIF data to screenshot" << file;
    }
    QSaveFile out(file);
    if (!out.open(QIODevice::WriteOnly) || (out.write(jpeg) != jpeg.size()) || !out.commit()) {
        qCWarning(VideoScreenshotLog) << "Failed to save screenshot" << file << out.errorString();
        return false;
    }

    qCDebug(VideoScreenshotLog) << "Screenshot saved:" << file;
    return true;
}
