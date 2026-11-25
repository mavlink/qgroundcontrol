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
    UT_PARAMETERIZED_TEST(_testHasExifDataJpegSegments);

    // Loading and saving
    void _testLoadFromBuffer();
    void _testLoadFromBufferInvalid();
    void _testSaveToBuffer();
    UT_PARAMETERIZED_TEST(_testSaveToBufferKeepsOtherSegments);
    void _testSaveToBufferLargestExif();
    void _testSaveToBufferRejectsOversizedExif();
    UT_PARAMETERIZED_TEST(_testSaveToBufferRejectsInput);

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
