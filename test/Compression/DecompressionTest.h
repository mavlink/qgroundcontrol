#pragma once

#include "UnitTest.h"

class DecompressionTest : public UnitTest
{
    Q_OBJECT

public:
    DecompressionTest();

private slots:
    void _testDecompressGzip();
    void _testDecompressLZMA();
};
