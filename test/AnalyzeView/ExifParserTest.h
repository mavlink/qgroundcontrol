#pragma once

#include "UnitTest.h"

#include <QtCore/QByteArray>

/// Unit tests for ExifParser.
/// Tests reading and writing EXIF metadata from/to image files.
/// Pure parsing tests - no singletons or network required.
class ExifParserTest : public UnitTest
{
    Q_OBJECT

public:
    ExifParserTest() = default;

private slots:
    void init() override;

    // Read tests
    void _readTimeValidImageTest();
    void _readTimeInvalidJpegTest();
    void _readTimeEmptyBufferTest();
    void _readTimeTruncatedJpegTest();

    // Write tests
    void _writeBasicTest();
    void _writePreservesTimestampTest();
    void _writeNorthEastCoordinatesTest();
    void _writeSouthWestCoordinatesTest();
    void _writeEquatorPrimeMeridianTest();
    void _writeHighAltitudeTest();

private:
    static QByteArray _loadTestImage();

    /// Expected EXIF timestamp from test image DSCN0010.jpg
    static constexpr int kExpectedYear = 2008;
    static constexpr int kExpectedMonth = 10;
    static constexpr int kExpectedDay = 22;
    static constexpr int kExpectedHour = 16;
    static constexpr int kExpectedMinute = 28;
    static constexpr int kExpectedSecond = 39;

    QByteArray _imageBuffer;
};
