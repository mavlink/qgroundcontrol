#pragma once

#include "UnitTest.h"

class LogCompressionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCompressDecompress();
    void _testCompressLevel();
    void _testMinSize();
    void _testIsCompressed();
    void _testEmptyData();
    void _testInvalidData();
    void _testCompressionRatio();
};
