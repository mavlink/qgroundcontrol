#pragma once

#include "UnitTest.h"

#include <QtCore/QByteArray>

/// Unit test for ULogParser.
class ULogParserTest : public UnitTest
{
    Q_OBJECT

public:
    ULogParserTest() = default;

private slots:
    void init() override;

    // Valid log tests
    void _getTagsFromLogTest();
    void _getTagsFromLogStreamedTest();
    void _compareStreamedAndNonStreamedTest();

    // Edge case tests
    void _emptyBufferTest();
    void _invalidDataTest();
    void _truncatedHeaderTest();

    // Benchmark tests
    void _benchmarkNonStreamed();
    void _benchmarkStreamed();

private:
    static QByteArray _loadSampleULog();

    QByteArray _logBuffer;
};
