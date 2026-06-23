#pragma once

#include "UnitTest.h"

class SecureMemoryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSecureZeroRawMemory();
    void _testSecureZeroByteArray();
    void _testSecureZeroEmptyByteArray();
    void _testSecureZeroZeroSize();
    void _testSecureZeroStdArray();
};
