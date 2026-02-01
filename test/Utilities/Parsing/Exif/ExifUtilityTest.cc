#include "ExifUtilityTest.h"
#include "ExifUtility.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

// ============================================================================
// EXIF Data Detection Tests
// ============================================================================

void ExifUtilityTest::_testHasExifData()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray buffer = file.readAll();
    file.close();

    QVERIFY(ExifUtility::hasExifData(buffer));
}

void ExifUtilityTest::_testHasExifDataInvalid()
{
    // Random data should not be detected as EXIF
    QByteArray randomData(1024, 'x');
    QVERIFY(!ExifUtility::hasExifData(randomData));

    // Empty buffer
    QByteArray empty;
    QVERIFY(!ExifUtility::hasExifData(empty));

    // Too small
    QByteArray small(4, '\x00');
    QVERIFY(!ExifUtility::hasExifData(small));
}

void ExifUtilityTest::_testHasExifDataNoExif()
{
    // Valid JPEG header but no EXIF (APP0/JFIF instead of APP1/EXIF)
    QByteArray jpegNoExif;
    jpegNoExif.append('\xFF');
    jpegNoExif.append('\xD8');  // SOI
    jpegNoExif.append('\xFF');
    jpegNoExif.append('\xE0');  // APP0 (JFIF, not EXIF)
    jpegNoExif.append('\x00');
    jpegNoExif.append('\x10');  // Length
    jpegNoExif.append("JFIF");
    jpegNoExif.append('\x00');
    jpegNoExif.append(10, '\x00');
    jpegNoExif.append('\xFF');
    jpegNoExif.append('\xD9');  // EOI

    QVERIFY(!ExifUtility::hasExifData(jpegNoExif));
}

// ============================================================================
// Loading and Saving Tests
// ============================================================================

void ExifUtilityTest::_testLoadFromBuffer()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray buffer = file.readAll();
    file.close();

    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    // Should have IFDs
    QVERIFY(data->ifd[EXIF_IFD_0] != nullptr);
    QVERIFY(data->ifd[EXIF_IFD_EXIF] != nullptr);

    exif_data_unref(data);
}

void ExifUtilityTest::_testLoadFromBufferInvalid()
{
    // Note: libexif's exif_data_new_from_data returns an ExifData even for
    // invalid input - it just won't contain any useful data. We verify that
    // the loaded data has no meaningful EXIF entries.

    // Empty buffer - libexif returns an ExifData but with no entries
    QByteArray empty;
    ExifData *emptyData = ExifUtility::loadFromBuffer(empty);
    if (emptyData) {
        // Should have no entries in IFD0
        QCOMPARE(emptyData->ifd[EXIF_IFD_0]->count, 0U);
        exif_data_unref(emptyData);
    }

    // Random data - may return ExifData but with no meaningful entries
    QByteArray random(1024, 'x');
    ExifData *randomData = ExifUtility::loadFromBuffer(random);
    if (randomData) {
        // Should have no entries
        QCOMPARE(randomData->ifd[EXIF_IFD_0]->count, 0U);
        exif_data_unref(randomData);
    }

    // PNG header - not JPEG, so no EXIF
    QByteArray pngData;
    pngData.append('\x89');
    pngData.append('P');
    pngData.append('N');
    pngData.append('G');
    pngData.append(1024, '\x00');
    ExifData *pngResult = ExifUtility::loadFromBuffer(pngData);
    if (pngResult) {
        QCOMPARE(pngResult->ifd[EXIF_IFD_0]->count, 0U);
        exif_data_unref(pngResult);
    }
}

void ExifUtilityTest::_testSaveToBuffer()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray buffer = file.readAll();
    file.close();

    const int originalSize = buffer.size();

    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    // Save back (should succeed even without modifications)
    QVERIFY(ExifUtility::saveToBuffer(data, buffer));
    exif_data_unref(data);

    // Should still be valid JPEG
    QVERIFY(buffer.size() > 0);
    QCOMPARE(static_cast<unsigned char>(buffer[0]), static_cast<unsigned char>(0xFF));
    QCOMPARE(static_cast<unsigned char>(buffer[1]), static_cast<unsigned char>(0xD8));

    // Size should be similar (minor differences due to re-encoding)
    QVERIFY(qAbs(buffer.size() - originalSize) < 1000);
}

// ============================================================================
// Tag Reading Tests
// ============================================================================

void ExifUtilityTest::_testReadString()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray buffer = file.readAll();
    file.close();

    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    // Read DateTimeDigitized from EXIF IFD
    QString dateTime = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
    QVERIFY(!dateTime.isEmpty());
    QVERIFY(dateTime.contains(":"));  // EXIF date format uses colons

    // Read camera make/model from IFD0
    QString make = ExifUtility::readString(data, EXIF_TAG_MAKE, EXIF_IFD_0);
    // May or may not be present, just verify it doesn't crash

    exif_data_unref(data);
}

void ExifUtilityTest::_testReadShort()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray buffer = file.readAll();
    file.close();

    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    // Read orientation (common EXIF tag)
    int orientation = ExifUtility::readShort(data, EXIF_TAG_ORIENTATION, EXIF_IFD_0);
    // Valid orientations are 1-8, or 0 if not present
    QVERIFY(orientation >= 0 && orientation <= 8);

    exif_data_unref(data);
}

void ExifUtilityTest::_testReadRational()
{
    QFile file(":/unittest/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray buffer = file.readAll();
    file.close();

    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    // Try reading focal length (commonly a rational)
    double focalLength = ExifUtility::readRational(data, EXIF_TAG_FOCAL_LENGTH, EXIF_IFD_EXIF);
    // May be 0 if not present, otherwise should be positive
    QVERIFY(focalLength >= 0.0);

    exif_data_unref(data);
}

// ============================================================================
// Tag Writing Tests
// ============================================================================

void ExifUtilityTest::_testInitTag()
{
    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    // Test initTag for a standard tag that can be auto-initialized
    ExifEntry *entry = ExifUtility::initTag(data, EXIF_IFD_0, EXIF_TAG_ORIENTATION);
    QVERIFY(entry != nullptr);
    QVERIFY(entry->data != nullptr);
    QCOMPARE(entry->tag, static_cast<ExifTag>(EXIF_TAG_ORIENTATION));

    // Calling again should return the same entry
    ExifEntry *entry2 = ExifUtility::initTag(data, EXIF_IFD_0, EXIF_TAG_ORIENTATION);
    QCOMPARE(entry, entry2);

    // Write a value
    ExifByteOrder order = exif_data_get_byte_order(data);
    exif_set_short(entry->data, order, 6);  // Rotate 90 CW

    // Read it back
    int value = exif_get_short(entry->data, order);
    QCOMPARE(value, 6);

    exif_data_unref(data);
}

void ExifUtilityTest::_testCreateTag()
{
    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    // Create a GPS latitude entry (3 rationals for deg/min/sec)
    ExifEntry *entry = ExifUtility::createTag(
        data,
        EXIF_IFD_GPS,
        static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE),
        EXIF_FORMAT_RATIONAL,
        3
    );

    QVERIFY(entry != nullptr);
    QVERIFY(entry->data != nullptr);
    QCOMPARE(entry->format, EXIF_FORMAT_RATIONAL);
    QCOMPARE(entry->components, 3UL);
    QCOMPARE(entry->size, static_cast<unsigned long>(3 * 8));  // 3 rationals * 8 bytes each

    exif_data_unref(data);
}

// ============================================================================
// GPS Coordinate Tests
// ============================================================================

void ExifUtilityTest::_testWriteGpsCoordinate()
{
    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);

    ExifEntry *entry = ExifUtility::createTag(
        data,
        EXIF_IFD_GPS,
        static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE),
        EXIF_FORMAT_RATIONAL,
        3
    );
    QVERIFY(entry != nullptr);

    // Write a coordinate
    const double testLat = 37.7749;
    ExifUtility::writeGpsCoordinate(entry, order, testLat);

    // Read it back
    double readBack = ExifUtility::gpsRationalToDecimal(entry, order);

    // Should be within tolerance (DMS conversion loses some precision)
    QVERIFY2(qAbs(readBack - testLat) < 0.001,
             qPrintable(QString("Expected %1, got %2").arg(testLat).arg(readBack)));

    exif_data_unref(data);
}

void ExifUtilityTest::_testWriteRational()
{
    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);

    // Create altitude entry (single rational)
    ExifEntry *entry = ExifUtility::createTag(
        data,
        EXIF_IFD_GPS,
        static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE),
        EXIF_FORMAT_RATIONAL,
        1
    );
    QVERIFY(entry != nullptr);

    // Write altitude
    const double testAlt = 123.45;
    ExifUtility::writeRational(entry, order, testAlt, 100);

    // Read it back
    ExifRational rational = exif_get_rational(entry->data, order);
    double readBack = static_cast<double>(rational.numerator) / static_cast<double>(rational.denominator);

    QVERIFY2(qAbs(readBack - testAlt) < 0.1,
             qPrintable(QString("Expected %1, got %2").arg(testAlt).arg(readBack)));

    exif_data_unref(data);
}

void ExifUtilityTest::_testGpsRationalToDecimal()
{
    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);

    ExifEntry *entry = ExifUtility::createTag(
        data,
        EXIF_IFD_GPS,
        static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE),
        EXIF_FORMAT_RATIONAL,
        3
    );
    QVERIFY(entry != nullptr);

    // Test various coordinates
    const double testCoords[] = {0.0, 45.5, 90.0, 122.4194, 179.9999};

    for (double coord : testCoords) {
        ExifUtility::writeGpsCoordinate(entry, order, coord);
        double result = ExifUtility::gpsRationalToDecimal(entry, order);
        QVERIFY2(qAbs(result - coord) < 0.001,
                 qPrintable(QString("Coord %1: expected %2, got %3").arg(coord).arg(coord).arg(result)));
    }

    exif_data_unref(data);
}

UT_REGISTER_TEST(ExifUtilityTest, TestLabel::Unit, TestLabel::Utilities)
