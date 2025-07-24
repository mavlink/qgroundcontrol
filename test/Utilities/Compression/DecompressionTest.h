#pragma once

#include "UnitTest.h"

class DecompressionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDecompressGzip();
    void _testDecompressLZMA();
    void _testUnzip();
};
