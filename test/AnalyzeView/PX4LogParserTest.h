#pragma once

#include "UnitTest.h"

class PX4LogParserTest : public UnitTest
{
    Q_OBJECT

public:
    PX4LogParserTest() = default;

private slots:
    void _getTagsFromLogTest();
};
