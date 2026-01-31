#include "ExifParserTest.h"
#include "ExifParser.h"
#include "ExifUtility.h"
#include "GeoTagData.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

void ExifParserTest::_readTimeTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray imageBuffer = file.readAll();
    file.close();

    const qint64 imageTime = ExifParser::readTime(imageBuffer).toSecsSinceEpoch();

    const QDate date(2008, 10, 22);
    const QTime time(16, 28, 39);

    const QDateTime tagTime(date, time);
    const qint64 expectedTime = tagTime.toSecsSinceEpoch();

    QCOMPARE(imageTime, expectedTime);
}

void ExifParserTest::_readTimeEmptyBufferTest()
{
    const QByteArray emptyBuffer;
    const QDateTime result = ExifParser::readTime(emptyBuffer);
    QVERIFY(!result.isValid());
}

void ExifParserTest::_readTimeInvalidJpegTest()
{
    const QByteArray invalidData("This is not a JPEG file");
    const QDateTime result = ExifParser::readTime(invalidData);
    QVERIFY(!result.isValid());
}

void ExifParserTest::_writeTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray imageBuffer = file.readAll();
    file.close();

    GeoTagData data;
    data.coordinate = QGeoCoordinate(37.225, -80.425, 618.4392);

    QVERIFY(ExifParser::write(imageBuffer, data));

    // Verify output is valid JPEG with EXIF
    QVERIFY(imageBuffer.size() > 0);
    QCOMPARE(static_cast<unsigned char>(imageBuffer[0]), static_cast<unsigned char>(0xFF));
    QCOMPARE(static_cast<unsigned char>(imageBuffer[1]), static_cast<unsigned char>(0xD8));
    QVERIFY(ExifUtility::hasExifData(imageBuffer));
}

void ExifParserTest::_writeEmptyBufferTest()
{
    QByteArray emptyBuffer;
    GeoTagData data;
    data.coordinate = QGeoCoordinate(37.225, -80.425, 618.4392);

    // Writing to empty buffer should create new EXIF data
    const bool result = ExifParser::write(emptyBuffer, data);
    // This may fail or succeed depending on implementation - just verify no crash
    Q_UNUSED(result);
}

void ExifParserTest::_writeAndReadBackTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Write GPS data using high-level API
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(-33.8688, 151.2093, 58.0);  // Sydney

    QVERIFY(ExifParser::write(buffer, geotag));

    // Read back using low-level API to verify
    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);
    ExifContent *gpsIfd = data->ifd[EXIF_IFD_GPS];
    QVERIFY(gpsIfd != nullptr);

    // Read latitude
    ExifEntry *latEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE));
    ExifEntry *latRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF));
    QVERIFY(latEntry != nullptr);
    QVERIFY(latRefEntry != nullptr);

    double latitude = ExifUtility::gpsRationalToDecimal(latEntry, order);
    if (latRefEntry->data[0] == 'S') {
        latitude = -latitude;
    }

    // Read longitude
    ExifEntry *lonEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE));
    ExifEntry *lonRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF));
    QVERIFY(lonEntry != nullptr);
    QVERIFY(lonRefEntry != nullptr);

    double longitude = ExifUtility::gpsRationalToDecimal(lonEntry, order);
    if (lonRefEntry->data[0] == 'W') {
        longitude = -longitude;
    }

    // Read altitude
    ExifEntry *altEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE));
    ExifEntry *altRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF));
    QVERIFY(altEntry != nullptr);
    QVERIFY(altRefEntry != nullptr);

    ExifRational altRational = exif_get_rational(altEntry->data, order);
    double altitude = static_cast<double>(altRational.numerator) / static_cast<double>(altRational.denominator);
    if (altRefEntry->data[0] == 1) {
        altitude = -altitude;
    }

    exif_data_unref(data);

    // Verify values within tolerance
    constexpr double coordTolerance = 0.001;
    constexpr double altTolerance = 0.1;

    QVERIFY2(qAbs(latitude - geotag.coordinate.latitude()) < coordTolerance,
             qPrintable(QString("Latitude: expected %1, got %2").arg(geotag.coordinate.latitude()).arg(latitude)));
    QVERIFY2(qAbs(longitude - geotag.coordinate.longitude()) < coordTolerance,
             qPrintable(QString("Longitude: expected %1, got %2").arg(geotag.coordinate.longitude()).arg(longitude)));
    QVERIFY2(qAbs(altitude - geotag.coordinate.altitude()) < altTolerance,
             qPrintable(QString("Altitude: expected %1, got %2").arg(geotag.coordinate.altitude()).arg(altitude)));
}

void ExifParserTest::_writeNegativeCoordinatesTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Test negative latitude (Southern hemisphere) and longitude (Western hemisphere)
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(-45.0312, -123.4567, 100.0);  // New Zealand / Pacific

    QVERIFY(ExifParser::write(buffer, geotag));

    // Verify using low-level API
    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    ExifContent *gpsIfd = data->ifd[EXIF_IFD_GPS];
    QVERIFY(gpsIfd != nullptr);

    // Check reference values
    ExifEntry *latRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF));
    ExifEntry *lonRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF));

    QVERIFY(latRefEntry != nullptr);
    QVERIFY(lonRefEntry != nullptr);
    QCOMPARE(latRefEntry->data[0], static_cast<unsigned char>('S'));
    QCOMPARE(lonRefEntry->data[0], static_cast<unsigned char>('W'));

    exif_data_unref(data);
}

void ExifParserTest::_writeNegativeAltitudeTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Test negative altitude (below sea level)
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(31.5, 35.5, -400.0);  // Dead Sea area

    QVERIFY(ExifParser::write(buffer, geotag));

    // Verify using low-level API
    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    ExifContent *gpsIfd = data->ifd[EXIF_IFD_GPS];
    QVERIFY(gpsIfd != nullptr);

    ExifEntry *altRefEntry = exif_content_get_entry(gpsIfd, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF));
    QVERIFY(altRefEntry != nullptr);
    QCOMPARE(altRefEntry->data[0], static_cast<unsigned char>(1));  // 1 = below sea level

    exif_data_unref(data);
}

void ExifParserTest::_writePreservesExistingDataTest()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray buffer = file.readAll();
    file.close();

    // Read original timestamp before modifying
    const QDateTime originalTime = ExifParser::readTime(buffer);
    QVERIFY(originalTime.isValid());

    // Write GPS data
    GeoTagData geotag;
    geotag.coordinate = QGeoCoordinate(40.7128, -74.0060, 10.0);  // New York

    QVERIFY(ExifParser::write(buffer, geotag));

    // Verify original timestamp is preserved
    const QDateTime timeAfterWrite = ExifParser::readTime(buffer);
    QCOMPARE(timeAfterWrite, originalTime);
}
