#include "VideoScreenshotTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtGui/QTransform>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/private/qmultimediautils_p.h>

#include "ExifUtility.h"
#include "VideoScreenshot.h"

namespace {

QVideoFrame testFrame()
{
    QImage image(QSize(64, 48), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    return QVideoFrame(image);
}

QByteArray fileBytes(const QString& file)
{
    QFile in(file);
    return in.open(QIODevice::ReadOnly) ? in.readAll() : QByteArray();
}

/// The EXIF data of the JPEG @p file, which the caller unrefs; nullptr without.
ExifData* loadExif(const QString& file)
{
    return ExifUtility::loadFromBuffer(fileBytes(file));
}

double gpsValue(ExifData* data, int tag)
{
    ExifEntry* const entry = exif_content_get_entry(data->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(tag));
    return entry ? ExifUtility::gpsRationalToDecimal(entry, exif_data_get_byte_order(data)) : qQNaN();
}

}  // namespace

void VideoScreenshotTest::init()
{
    UnitTest::init();
    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
}

void VideoScreenshotTest::_testJpegCarriesExif()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString file = tempDir.filePath(QStringLiteral("shot.jpg"));

    VideoScreenshot::Exif exif;
    exif.taken = QDateTime(QDate(2026, 10, 24), QTime(10, 15, 30));
    exif.position = QGeoCoordinate(47.3977, -8.5456, 488.25);
    QVERIFY(VideoScreenshot::save(testFrame(), file, exif));
    QCOMPARE(QImage(file).size(), QSize(64, 48));

    ExifData* const data = loadExif(file);
    QVERIFY(data);
    const auto release = qScopeGuard([data] { exif_data_unref(data); });
    QCOMPARE(ExifUtility::readString(data, EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_IFD_EXIF),
             QStringLiteral("2026:10:24 10:15:30"));
    QVERIFY(qAbs(gpsValue(data, EXIF_TAG_GPS_LATITUDE) - 47.3977) < 1e-4);
    QCOMPARE(ExifUtility::readString(data, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF), EXIF_IFD_GPS),
             QStringLiteral("N"));
    QVERIFY(qAbs(gpsValue(data, EXIF_TAG_GPS_LONGITUDE) - 8.5456) < 1e-4);
    QCOMPARE(ExifUtility::readString(data, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF), EXIF_IFD_GPS),
             QStringLiteral("W"));
    QCOMPARE(ExifUtility::readRational(data, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE), EXIF_IFD_GPS), 488.25);
}

void VideoScreenshotTest::_testPositionWithoutAltitude()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString file = tempDir.filePath(QStringLiteral("shot.jpeg"));

    VideoScreenshot::Exif exif;
    exif.position = QGeoCoordinate(-33.8688, 151.2093);
    QVERIFY(VideoScreenshot::save(testFrame(), file, exif));

    ExifData* const data = loadExif(file);
    QVERIFY(data);
    const auto release = qScopeGuard([data] { exif_data_unref(data); });
    QVERIFY(qAbs(gpsValue(data, EXIF_TAG_GPS_LATITUDE) - 33.8688) < 1e-4);
    QVERIFY(!exif_content_get_entry(data->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE)));
    QVERIFY(!exif_content_get_entry(data->ifd[EXIF_IFD_EXIF], EXIF_TAG_DATE_TIME_ORIGINAL));
}

void VideoScreenshotTest::_testDisplayedImageAsQtPresentsIt()
{
    // Every pixel differs, so any turn or flip changes the picture.
    QImage image(QSize(4, 2), QImage::Format_RGBA8888);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixelColor(x, y, QColor(x * 60, y * 120, 255 - (x * 60), 255));
        }
    }

    // A screenshot must show what the VideoOutput does: the frame's rotation and mirroring (a phone camera's sensor,
    // a stream's orientation tag) with the output's on top (a USB camera's rotation and mirror settings, the rotation a
    // camera reports for its stream), composed as Qt composes them to present the frame.
    constexpr QtVideo::Rotation kRotations[] = {QtVideo::Rotation::None, QtVideo::Rotation::Clockwise90,
                                                QtVideo::Rotation::Clockwise180, QtVideo::Rotation::Clockwise270};
    for (const QtVideo::Rotation frameRotation : kRotations) {
        for (const bool frameMirrored : {false, true}) {
            QVideoFrame frame(image);
            frame.setRotation(frameRotation);
            frame.setMirrored(frameMirrored);
            for (const QtVideo::Rotation outputRotation : kRotations) {
                for (const bool outputMirrored : {false, true}) {
                    const VideoTransformation presented =
                        qNormalizedFrameTransformation(frame, VideoTransformation{outputRotation, outputMirrored});
                    QImage expected = image.transformed(QTransform().rotate(static_cast<int>(presented.rotation)));
                    if (presented.mirroredHorizontallyAfterRotation) {
                        expected = expected.flipped(Qt::Horizontal);
                    }
                    const QImage displayed = VideoScreenshot::displayedImage(frame, outputRotation, outputMirrored);
                    QVERIFY2(displayed.convertToFormat(QImage::Format_ARGB32) ==
                                 expected.convertToFormat(QImage::Format_ARGB32),
                             qPrintable(QStringLiteral("frame %1%2, output %3%4")
                                            .arg(static_cast<int>(frameRotation))
                                            .arg(frameMirrored ? QStringLiteral(" mirrored") : QString())
                                            .arg(static_cast<int>(outputRotation))
                                            .arg(outputMirrored ? QStringLiteral(" mirrored") : QString())));
                }
            }
        }
    }
}

void VideoScreenshotTest::_testStretchedLosesNoPixel_data()
{
    // What a picture of 64 × 48 becomes: widened or heightened to the ratio, never shrunk.
    QTest::addColumn<double>("aspectRatio");
    QTest::addColumn<QSize>("size");

    QTest::newRow("no ratio") << 0.0 << QSize(64, 48);
    QTest::newRow("its own ratio") << (4.0 / 3.0) << QSize(64, 48);
    QTest::newRow("wider") << (16.0 / 9.0) << QSize(85, 48);
    QTest::newRow("narrower") << 1.0 << QSize(64, 64);
}

void VideoScreenshotTest::_testStretchedLosesNoPixel()
{
    QFETCH(double, aspectRatio);
    QFETCH(QSize, size);

    QImage image(QSize(64, 48), QImage::Format_RGBA8888);
    image.fill(Qt::blue);
    QCOMPARE(VideoScreenshot::stretched(image, aspectRatio).size(), size);
}

void VideoScreenshotTest::_testNoExifWithoutValues()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString file = tempDir.filePath(QStringLiteral("shot.jpg"));

    QVERIFY(VideoScreenshot::save(testFrame(), file));
    QCOMPARE(QImage(file).size(), QSize(64, 48));
    QVERIFY(!ExifUtility::hasExifData(fileBytes(file)));
}

void VideoScreenshotTest::_testOtherFormatsWithoutExif()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString file = tempDir.filePath(QStringLiteral("shot.png"));

    VideoScreenshot::Exif exif;
    exif.taken = QDateTime::currentDateTime();
    exif.position = QGeoCoordinate(47.3977, 8.5456, 488.0);
    QVERIFY(VideoScreenshot::save(testFrame(), file, exif));
    QCOMPARE(QImage(file).size(), QSize(64, 48));
}

UT_REGISTER_TEST(VideoScreenshotTest, TestLabel::Unit)
