#pragma once

#include "UnitTest.h"

class ULogParserTest : public UnitTest
{
    Q_OBJECT

public:
    ULogParserTest() = default;

private slots:
    void _getTagsFromLogTest();
    void _getTagsFromLogStreamedTest();
    void _compareStreamedAndNonStreamedTest();
    void _benchmarkNonStreamed();
    void _benchmarkStreamed();
};
