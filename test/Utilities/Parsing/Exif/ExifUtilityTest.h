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
    void _testHasExifDataJpegSegments_data();
    void _testHasExifDataJpegSegments();

    // Loading and saving
    void _testLoadFromBuffer();
    void _testLoadFromBufferInvalid();
    void _testSaveToBuffer();
    void _testSaveToBufferKeepsOtherSegments_data();
    void _testSaveToBufferKeepsOtherSegments();
    void _testSaveToBufferLargestExif();
    void _testSaveToBufferRejectsOversizedExif();
    void _testSaveToBufferRejectsInput_data();
    void _testSaveToBufferRejectsInput();

    // Tag reading
    void _testReadTags();

    // Tag writing
    void _testInitTag();
    void _testCreateTag();

    // GPS coordinate helpers
    void _testWriteRational();
    void _testGpsRationalToDecimal();
    void _testWriteDateTimeOriginalRejectsInvalidInput();
    void _testWriteDateTimeOriginalWritesBothTags();
};
