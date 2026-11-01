#include "VideoScreenshotTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>

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
