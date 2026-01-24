#include "ExifParserTest.h"
#include "TestHelpers.h"
#include "QtTestExtensions.h"
#include "ExifParser.h"
#include "GeoTagWorker.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

// ============================================================================
// Setup and Helpers
// ============================================================================

QByteArray ExifParserTest::_loadTestImage()
{
    QFile file(":/unittest/DSCN0010.jpg");
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    const QByteArray data = file.readAll();
    file.close();
    return data;
}

void ExifParserTest::init()
{
    UnitTest::init();

    _imageBuffer = _loadTestImage();
    QVERIFY2(!_imageBuffer.isEmpty(), "Failed to load test image DSCN0010.jpg");
}

// ============================================================================
// Read Time Tests
// ============================================================================

void ExifParserTest::_readTimeValidImageTest()
{
    const QDateTime imageTime = ExifParser::readTime(_imageBuffer);
    QVERIFY2(imageTime.isValid(), "ExifParser::readTime returned invalid datetime");

    const QDate expectedDate(kExpectedYear, kExpectedMonth, kExpectedDay);
    const QTime expectedTime(kExpectedHour, kExpectedMinute, kExpectedSecond);
    const QDateTime expectedDateTime(expectedDate, expectedTime);

    QCOMPARE_EQ(imageTime.toSecsSinceEpoch(), expectedDateTime.toSecsSinceEpoch());
}

void ExifParserTest::_readTimeInvalidJpegTest()
{
    // Not a JPEG - should return invalid datetime
    const QByteArray notJpeg("This is not a JPEG file");
    const QDateTime result = ExifParser::readTime(notJpeg);
    QVERIFY(!result.isValid());

    // PNG header instead of JPEG
    const QByteArray pngHeader("\x89PNG\r\n\x1a\n", 8);
    const QDateTime pngResult = ExifParser::readTime(pngHeader);
    QVERIFY(!pngResult.isValid());
}

void ExifParserTest::_readTimeEmptyBufferTest()
{
    const QByteArray empty;
    const QDateTime result = ExifParser::readTime(empty);
    QVERIFY(!result.isValid());
}

void ExifParserTest::_readTimeTruncatedJpegTest()
{
    // Valid JPEG header but truncated
    const QByteArray truncated("\xFF\xD8\xFF\xE1", 4);
    const QDateTime result = ExifParser::readTime(truncated);
    QVERIFY(!result.isValid());

    // First 100 bytes of valid image (truncated EXIF)
    const QByteArray partialImage = _imageBuffer.left(100);
    const QDateTime partialResult = ExifParser::readTime(partialImage);
    QVERIFY(!partialResult.isValid());
}

// ============================================================================
// Write Tests
// ============================================================================

void ExifParserTest::_writeBasicTest()
{
    QByteArray imageBufferCopy = _imageBuffer;

    GeoTagWorker::CameraFeedbackPacket geoData{};
    geoData.latitude = 37.225;
    geoData.longitude = -80.425;
    geoData.altitude = 618.4392;

    QVERIFY2(ExifParser::write(imageBufferCopy, geoData), "ExifParser::write failed");
    QGC_VERIFY_NOT_EMPTY(imageBufferCopy);

    // Buffer should still be a valid JPEG
    QVERIFY2(imageBufferCopy.startsWith("\xFF\xD8"), "Output is not a valid JPEG");

    // Size should have increased (GPS data added)
    QCOMPARE_GT(imageBufferCopy.size(), _imageBuffer.size());
}

void ExifParserTest::_writePreservesTimestampTest()
{
    QByteArray imageBufferCopy = _imageBuffer;

    GeoTagWorker::CameraFeedbackPacket geoData{};
    geoData.latitude = 47.6062;
    geoData.longitude = -122.3321;
    geoData.altitude = 56.0;

    QVERIFY(ExifParser::write(imageBufferCopy, geoData));

    // Verify original timestamp is preserved
    const QDateTime timeAfterWrite = ExifParser::readTime(imageBufferCopy);
    QVERIFY2(timeAfterWrite.isValid(), "Timestamp was corrupted by write");

    const QDate expectedDate(kExpectedYear, kExpectedMonth, kExpectedDay);
    const QTime expectedTime(kExpectedHour, kExpectedMinute, kExpectedSecond);
    const QDateTime expectedDateTime(expectedDate, expectedTime);

    QCOMPARE_EQ(timeAfterWrite.toSecsSinceEpoch(), expectedDateTime.toSecsSinceEpoch());
}

void ExifParserTest::_writeNorthEastCoordinatesTest()
{
    QByteArray imageBufferCopy = _imageBuffer;

    // Tokyo, Japan (Northern + Eastern hemisphere)
    GeoTagWorker::CameraFeedbackPacket geoData{};
    geoData.latitude = 35.6762;
    geoData.longitude = 139.6503;
    geoData.altitude = 40.0;

    QVERIFY2(ExifParser::write(imageBufferCopy, geoData),
             "Failed to write NE coordinates");
    QVERIFY(imageBufferCopy.startsWith("\xFF\xD8"));
}

void ExifParserTest::_writeSouthWestCoordinatesTest()
{
    QByteArray imageBufferCopy = _imageBuffer;

    // Buenos Aires, Argentina (Southern + Western hemisphere)
    GeoTagWorker::CameraFeedbackPacket geoData{};
    geoData.latitude = -34.6037;
    geoData.longitude = -58.3816;
    geoData.altitude = 25.0;

    QVERIFY2(ExifParser::write(imageBufferCopy, geoData),
             "Failed to write SW coordinates");
    QVERIFY(imageBufferCopy.startsWith("\xFF\xD8"));
}

void ExifParserTest::_writeEquatorPrimeMeridianTest()
{
    QByteArray imageBufferCopy = _imageBuffer;

    // Null Island (0,0) - edge case
    GeoTagWorker::CameraFeedbackPacket geoData{};
    geoData.latitude = 0.0;
    geoData.longitude = 0.0;
    geoData.altitude = 0.0;

    QVERIFY2(ExifParser::write(imageBufferCopy, geoData),
             "Failed to write equator/prime meridian coordinates");
    QVERIFY(imageBufferCopy.startsWith("\xFF\xD8"));
}

void ExifParserTest::_writeHighAltitudeTest()
{
    QByteArray imageBufferCopy = _imageBuffer;

    // Mount Everest summit
    GeoTagWorker::CameraFeedbackPacket geoData{};
    geoData.latitude = 27.9881;
    geoData.longitude = 86.9250;
    geoData.altitude = 8848.86;  // meters

    QVERIFY2(ExifParser::write(imageBufferCopy, geoData),
             "Failed to write high altitude coordinates");
    QVERIFY(imageBufferCopy.startsWith("\xFF\xD8"));

    // Verify timestamp still readable after high altitude write
    const QDateTime timeAfterWrite = ExifParser::readTime(imageBufferCopy);
    QVERIFY(timeAfterWrite.isValid());
}
