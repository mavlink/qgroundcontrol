#include "ExifUtilityTest.h"

#include <QtCore/QScopeGuard>

#include "ExifUtility.h"
#include "FileFixtures.h"

namespace {

QByteArray jpegSegment(char marker, const QByteArray& payload)
{
    const qsizetype length = payload.size() + 2;
    QByteArray segment;
    segment.append('\xFF');
    segment.append(marker);
    segment.append(static_cast<char>((length >> 8) & 0xFF));
    segment.append(static_cast<char>(length & 0xFF));
    segment.append(payload);
    return segment;
}

QByteArray jpegSoi()
{
    return QByteArray::fromHex("ffd8");
}

QByteArray jfifSegment()
{
    return jpegSegment('\xE0', QByteArray::fromHex("4a46494600010100000100010000"));
}

QByteArray xmpSegment()
{
    return jpegSegment('\xE1', QByteArray("http://ns.adobe.com/xap/1.0/\0", 29) + QByteArray("<x:xmpmeta/>"));
}

/// A quantization table whose values are @p tableBytes repeated
QByteArray quantTableSegment(const QByteArray& tableBytes)
{
    return jpegSegment('\xDB', QByteArray(1, '\0') + tableBytes.repeated(64 / tableBytes.size()));
}

/// SOS segment, entropy-coded data and EOI
QByteArray jpegScan()
{
    return jpegSegment('\xDA', QByteArray::fromHex("010100003f00")) + QByteArray::fromHex("12ff0034ffd9");
}

QByteArray exifBlock(ExifData* data)
{
    unsigned char* bytes = nullptr;
    unsigned int size = 0;
    exif_data_save_data(data, &bytes, &size);
    const QByteArray block(reinterpret_cast<const char*>(bytes), size);
    free(bytes);
    return block;
}

/// The APP1 segment that saving @p data writes
QByteArray exifApp1Segment(ExifData* data)
{
    return jpegSegment('\xE1', exifBlock(data));
}

/// EXIF data whose block is @p size bytes (an even number), padded with a comment
ExifData* exifDataOfSize(qsizetype size)
{
    ExifData* data = ExifUtility::createNew();
    // A tag takes a 12 byte directory entry and its value
    const qsizetype commentSize = size - exifBlock(data).size() - 12;
    ExifUtility::createTag(data, EXIF_IFD_EXIF, EXIF_TAG_USER_COMMENT, EXIF_FORMAT_UNDEFINED, commentSize);
    return data;
}

}  // namespace

// ============================================================================
// EXIF Data Detection Tests
// ============================================================================

void ExifUtilityTest::_testHasExifData()
{
    const QByteArray buffer = TestFixtures::readFile(QStringLiteral(":/unittest/DSCN0010.jpg"));
    QVERIFY(!buffer.isEmpty());

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

void ExifUtilityTest::_testHasExifDataJpegSegments_data()
{
    QTest::addColumn<QByteArray>("jpeg");
    QTest::addColumn<bool>("hasExif");

    ExifData* data = ExifUtility::createNew();
    const QByteArray exif = exifApp1Segment(data);
    exif_data_unref(data);

    const QByteArray soi = jpegSoi();
    const QByteArray jfif = jfifSegment();
    const QByteArray xmp = xmpSegment();
    const QByteArray image = quantTableSegment(QByteArray::fromHex("10")) + jpegScan();
    // A comment whose text reads as the start of an EXIF segment
    const QByteArray exifLikeComment =
        jpegSegment('\xFE', QByteArray::fromHex("ffe10010") + QByteArray("Exif\0\0", 6) + QByteArray(8, 'x'));

    QTest::newRow("exif-first") << soi + exif + image << true;
    QTest::newRow("jfif-then-exif") << soi + jfif + exif + image << true;
    QTest::newRow("xmp-then-exif") << soi + xmp + exif + image << true;
    QTest::newRow("exif-then-xmp") << soi + exif + xmp + image << true;
    QTest::newRow("jfif-without-exif") << soi + jfif + jpegScan() << false;
    QTest::newRow("xmp-without-exif") << soi + jfif + xmp + image << false;
    QTest::newRow("exif-segment-cut") << soi + jfif + exif.chopped(1) << false;
    QTest::newRow("exif-bytes-in-comment") << soi + jfif + exifLikeComment + image << false;
    QTest::newRow("data-before-exif") << soi + QByteArray::fromHex("0000") + exif + image << false;
    QTest::newRow("exif-after-image-data") << soi + jfif + image + exif << false;
}

void ExifUtilityTest::_testHasExifDataJpegSegments()
{
    QFETCH(QByteArray, jpeg);
    QFETCH(bool, hasExif);

    QCOMPARE(ExifUtility::hasExifData(jpeg), hasExif);

    // For the images of these rows libexif agrees: it reads entries where the answer is true and none where it is
    // false
    ExifData* loaded = ExifUtility::loadFromBuffer(jpeg);
    QVERIFY(loaded != nullptr);
    const auto release = qScopeGuard([loaded] { exif_data_unref(loaded); });
    unsigned int entryCount = 0;
    for (const ExifContent* content : loaded->ifd) {
        entryCount += content->count;
    }
    QCOMPARE(entryCount > 0, hasExif);
}

// ============================================================================
// Loading and Saving Tests
// ============================================================================

void ExifUtilityTest::_testLoadFromBuffer()
{
    const QByteArray buffer = TestFixtures::readFile(QStringLiteral(":/unittest/DSCN0010.jpg"));
    QVERIFY(!buffer.isEmpty());

    ExifData* data = ExifUtility::loadFromBuffer(buffer);
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
    const QList<QByteArray> buffers{
        QByteArray(),                                    // Empty buffer
        QByteArray(1024, 'x'),                           // Random data
        QByteArray("\x89PNG") + QByteArray(1024, '\0'),  // PNG header - not JPEG, so no EXIF
    };
    for (const QByteArray& buffer : buffers) {
        ExifData* data = ExifUtility::loadFromBuffer(buffer);
        if (data) {
            QCOMPARE(data->ifd[EXIF_IFD_0]->count, 0U);
            exif_data_unref(data);
        }
    }
}

void ExifUtilityTest::_testSaveToBuffer()
{
    QByteArray buffer = TestFixtures::readFile(QStringLiteral(":/unittest/DSCN0010.jpg"));
    QVERIFY(!buffer.isEmpty());

    const int originalSize = buffer.size();

    ExifData* data = ExifUtility::loadFromBuffer(buffer);
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

void ExifUtilityTest::_testSaveToBufferKeepsOtherSegments_data()
{
    QTest::addColumn<QByteArray>("before");
    QTest::addColumn<QByteArray>("existingExif");
    QTest::addColumn<QByteArray>("after");

    ExifData* oldData = ExifUtility::createNew();
    ExifUtility::writeDateTimeOriginal(oldData, QDateTime(QDate(2001, 1, 1), QTime(0, 0), QTimeZone::UTC));
    const QByteArray oldExif = exifApp1Segment(oldData);
    exif_data_unref(oldData);

    const QByteArray soi = jpegSoi();
    const QByteArray jfif = jfifSegment();
    const QByteArray xmp = xmpSegment();
    const QByteArray image = quantTableSegment(QByteArray::fromHex("10")) + jpegScan();
    // Table values that read as an APP1 marker and its length
    const QByteArray markerLikeTable = quantTableSegment(QByteArray::fromHex("ffe10008"));
    const QByteArray fill(1, '\xFF');

    QTest::newRow("jfif-then-exif") << soi + jfif << oldExif << image;
    QTest::newRow("jfif-without-exif") << soi + jfif << QByteArray() << image;
    QTest::newRow("no-app-segments") << soi << QByteArray() << image;
    QTest::newRow("xmp-without-exif") << soi + jfif << QByteArray() << xmp + image;
    QTest::newRow("xmp-then-exif") << soi + xmp << oldExif << image;
    QTest::newRow("exif-then-xmp") << soi << oldExif << xmp + image;
    QTest::newRow("marker-bytes-in-table") << soi << QByteArray() << markerLikeTable + jpegScan();
    QTest::newRow("fill-bytes") << soi + jfif + fill << oldExif << fill + image;
}

void ExifUtilityTest::_testSaveToBufferKeepsOtherSegments()
{
    QFETCH(QByteArray, before);
    QFETCH(QByteArray, existingExif);
    QFETCH(QByteArray, after);

    ExifData* data = ExifUtility::createNew();
    QVERIFY(data != nullptr);
    const auto release = qScopeGuard([data] { exif_data_unref(data); });
    QVERIFY(ExifUtility::writeDateTimeOriginal(data, QDateTime(QDate(2025, 2, 14), QTime(12, 34, 56), QTimeZone::UTC)));

    QByteArray jpeg = before + existingExif + after;
    QVERIFY(ExifUtility::saveToBuffer(data, jpeg));
    QCOMPARE(jpeg, before + exifApp1Segment(data) + after);

    ExifData* saved = ExifUtility::loadFromBuffer(jpeg);
    QVERIFY(saved != nullptr);
    const QString dateTime = ExifUtility::readString(saved, EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_IFD_EXIF);
    exif_data_unref(saved);
    QCOMPARE(dateTime, QStringLiteral("2025:02:14 12:34:56"));
}

void ExifUtilityTest::_testSaveToBufferLargestExif()
{
    // A segment holds 65533 bytes; the EXIF blocks written here have even sizes
    constexpr qsizetype exifSize = 65532;
    ExifData* data = exifDataOfSize(exifSize);
    QVERIFY(data != nullptr);
    const auto release = qScopeGuard([data] { exif_data_unref(data); });
    QCOMPARE(exifBlock(data).size(), exifSize);

    QByteArray jpeg = jpegSoi() + jfifSegment() + jpegScan();
    QVERIFY(ExifUtility::saveToBuffer(data, jpeg));
    QCOMPARE(jpeg, jpegSoi() + jfifSegment() + exifApp1Segment(data) + jpegScan());

    ExifData* saved = ExifUtility::loadFromBuffer(jpeg);
    QVERIFY(saved != nullptr);
    const bool hasComment = exif_content_get_entry(saved->ifd[EXIF_IFD_EXIF], EXIF_TAG_USER_COMMENT) != nullptr;
    exif_data_unref(saved);
    QVERIFY(hasComment);
}

void ExifUtilityTest::_testSaveToBufferRejectsOversizedExif()
{
    constexpr qsizetype exifSize = 65534;
    ExifData* data = exifDataOfSize(exifSize);
    QVERIFY(data != nullptr);
    const auto release = qScopeGuard([data] { exif_data_unref(data); });
    QCOMPARE(exifBlock(data).size(), exifSize);

    const QByteArray original = jpegSoi() + jfifSegment() + jpegScan();
    QByteArray jpeg = original;
    expectLogMessage("Utilities.ExifUtility", QtWarningMsg, QRegularExpression("does not fit in one JPEG segment"));
    QVERIFY(!ExifUtility::saveToBuffer(data, jpeg));
    verifyExpectedLogMessage();
    QCOMPARE(jpeg, original);
}

void ExifUtilityTest::_testSaveToBufferRejectsInput_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<bool>("isTiff");
    QTest::addColumn<QString>("warning");

    const QByteArray soi = jpegSoi();
    const QByteArray jfif = jfifSegment();
    const QString malformed = QStringLiteral("Malformed JPEG segments");
    QTest::newRow("ends-before-scan") << soi + jfif << false << malformed;
    QTest::newRow("truncated-segment") << soi + jfif.chopped(1) << false << malformed;
    QTest::newRow("segment-length-below-minimum")
        << soi + QByteArray::fromHex("ffe00001") + jpegScan() << false << malformed;
    QTest::newRow("data-between-segments")
        << soi + jfif + QByteArray::fromHex("0000") + jpegScan() << false << malformed;
    QTest::newRow("not-a-jpeg") << QByteArray("plain-text-data") << false << QStringLiteral("Not a valid JPEG file");
    // Minimal little-endian TIFF header: "II" + 0x002A
    QTest::newRow("tiff") << QByteArray::fromHex("49492a00") + QByteArray(16, '\0') << true
                          << QStringLiteral("TIFF/DNG format not supported");
}

void ExifUtilityTest::_testSaveToBufferRejectsInput()
{
    QFETCH(QByteArray, original);
    QFETCH(bool, isTiff);
    QFETCH(QString, warning);

    ExifData* data = ExifUtility::createNew();
    QVERIFY(data != nullptr);
    const auto release = qScopeGuard([data] { exif_data_unref(data); });

    QCOMPARE(ExifUtility::isTiff(original), isTiff);
    QByteArray buffer = original;
    expectLogMessage("Utilities.ExifUtility", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(!ExifUtility::saveToBuffer(data, buffer));
    verifyExpectedLogMessage();
    QCOMPARE(buffer, original);
}

// ============================================================================
// Tag Reading Tests
// ============================================================================

void ExifUtilityTest::_testReadTags()
{
    const QByteArray buffer = TestFixtures::readFile(QStringLiteral(":/unittest/DSCN0010.jpg"));
    QVERIFY(!buffer.isEmpty());

    ExifData* data = ExifUtility::loadFromBuffer(buffer);
    QVERIFY(data != nullptr);

    // Read DateTimeDigitized from EXIF IFD
    QString dateTime = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
    QVERIFY(!dateTime.isEmpty());
    QVERIFY(dateTime.contains(":"));  // EXIF date format uses colons

    // The camera make in IFD0 may or may not be present: reading it must not crash
    (void) ExifUtility::readString(data, EXIF_TAG_MAKE, EXIF_IFD_0);

    // Read orientation (common EXIF tag)
    int orientation = ExifUtility::readShort(data, EXIF_TAG_ORIENTATION, EXIF_IFD_0);
    // Valid orientations are 1-8, or 0 if not present
    QVERIFY(orientation >= 0 && orientation <= 8);

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
    ExifData* data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    // Test initTag for a standard tag that can be auto-initialized
    ExifEntry* entry = ExifUtility::initTag(data, EXIF_IFD_0, EXIF_TAG_ORIENTATION);
    QVERIFY(entry != nullptr);
    QVERIFY(entry->data != nullptr);
    QCOMPARE(entry->tag, static_cast<ExifTag>(EXIF_TAG_ORIENTATION));

    // Calling again should return the same entry
    ExifEntry* entry2 = ExifUtility::initTag(data, EXIF_IFD_0, EXIF_TAG_ORIENTATION);
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
    ExifData* data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    // Create a GPS latitude entry (3 rationals for deg/min/sec)
    ExifEntry* entry = ExifUtility::createTag(data, EXIF_IFD_GPS, static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE),
                                              EXIF_FORMAT_RATIONAL, 3);

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

void ExifUtilityTest::_testWriteRational()
{
    ExifData* data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);

    // Create altitude entry (single rational)
    ExifEntry* entry = ExifUtility::createTag(data, EXIF_IFD_GPS, static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE),
                                              EXIF_FORMAT_RATIONAL, 1);
    QVERIFY(entry != nullptr);

    // Write altitude
    const double testAlt = 123.45;
    ExifUtility::writeRational(entry, order, testAlt, 100);

    // Read it back
    ExifRational rational = exif_get_rational(entry->data, order);
    double readBack = static_cast<double>(rational.numerator) / static_cast<double>(rational.denominator);

    QVERIFY2(qAbs(readBack - testAlt) < 0.1, qPrintable(QString("Expected %1, got %2").arg(testAlt).arg(readBack)));

    exif_data_unref(data);
}

void ExifUtilityTest::_testGpsRationalToDecimal()
{
    ExifData* data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    ExifByteOrder order = exif_data_get_byte_order(data);

    ExifEntry* entry = ExifUtility::createTag(data, EXIF_IFD_GPS, static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE),
                                              EXIF_FORMAT_RATIONAL, 3);
    QVERIFY(entry != nullptr);

    // Test various coordinates
    const double testCoords[] = {0.0, 37.7749, 45.5, 90.0, 122.4194, 179.9999};

    for (double coord : testCoords) {
        ExifUtility::writeGpsCoordinate(entry, order, coord);
        double result = ExifUtility::gpsRationalToDecimal(entry, order);
        // Within tolerance: the conversion to degrees, minutes and seconds loses some precision
        QVERIFY2(qAbs(result - coord) < 0.001,
                 qPrintable(QString("Coord %1: expected %2, got %3").arg(coord).arg(coord).arg(result)));
    }

    exif_data_unref(data);
}

void ExifUtilityTest::_testWriteDateTimeOriginalRejectsInvalidInput()
{
    const QDateTime invalidDateTime;
    QVERIFY(!ExifUtility::writeDateTimeOriginal(nullptr, invalidDateTime));

    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    QVERIFY(!ExifUtility::writeDateTimeOriginal(data, invalidDateTime));
    exif_data_unref(data);
}

void ExifUtilityTest::_testWriteDateTimeOriginalWritesBothTags()
{
    ExifData *data = ExifUtility::createNew();
    QVERIFY(data != nullptr);

    const QDateTime dateTime(QDate(2025, 2, 14), QTime(12, 34, 56), QTimeZone::UTC);
    QVERIFY(dateTime.isValid());

    QVERIFY(ExifUtility::writeDateTimeOriginal(data, dateTime));

    const QString expected = QStringLiteral("2025:02:14 12:34:56");
    QCOMPARE(ExifUtility::readString(data, EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_IFD_EXIF), expected);
    QCOMPARE(ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF), expected);

    exif_data_unref(data);
}

UT_REGISTER_TEST(ExifUtilityTest, TestLabel::Unit, TestLabel::Utilities)
