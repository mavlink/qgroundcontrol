#pragma once

#include "UnitTest.h"

class ExifUtilityTest : public UnitTest
{
    Q_OBJECT

public:
    ExifUtilityTest() = default;

private slots:
    // EXIF data detection
    void _testHasExifData();
    void _testHasExifDataInvalid();
    void _testHasExifDataNoExif();

    // Loading and saving
    void _testLoadFromBuffer();
    void _testLoadFromBufferInvalid();
    void _testSaveToBuffer();
    void _testSaveToBufferRejectsNonJpeg();
    void _testSaveToBufferRejectsTiff();

    // Tag reading
    void _testReadString();
    void _testReadShort();
    void _testReadRational();

    // Tag writing
    void _testInitTag();
    void _testCreateTag();

    // GPS coordinate helpers
    void _testWriteGpsCoordinate();
    void _testWriteRational();
    void _testGpsRationalToDecimal();
    void _testWriteDateTimeOriginalRejectsInvalidInput();
    void _testWriteDateTimeOriginalWritesBothTags();
};
