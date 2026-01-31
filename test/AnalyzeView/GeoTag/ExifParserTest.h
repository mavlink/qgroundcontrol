#pragma once

#include "UnitTest.h"

class ExifParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _readTimeTest();
    void _readTimeEmptyBufferTest();
    void _readTimeInvalidJpegTest();
    void _writeTest();
    void _writeEmptyBufferTest();
    void _writeAndReadBackTest();
    void _writeNegativeCoordinatesTest();
    void _writeNegativeAltitudeTest();
    void _writePreservesExistingDataTest();
};
