#pragma once

#include "UnitTest.h"

#include <QtCore/QByteArray>

/// Unit tests for PX4LogParser.
/// Tests parsing PX4 binary log format for camera trigger and GPS data.
/// No sample PX4 log file available, so tests focus on edge cases.
class PX4LogParserTest : public UnitTest
{
    Q_OBJECT

public:
    PX4LogParserTest() = default;

private slots:
    // Edge case tests (no sample log available)
    void _emptyBufferTest();
    void _invalidDataTest();
    void _truncatedHeaderTest();
    void _headerOnlyTest();
    void _randomDataTest();
};
