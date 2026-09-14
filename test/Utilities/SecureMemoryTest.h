#pragma once

#include "PortableTest.h"

class SecureMemoryTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _testSecureZeroRawMemory();
    void _testSecureZeroByteArray();
    void _testSecureZeroEmptyByteArray();
    void _testSecureZeroZeroSize();
    void _testSecureZeroStdArray();
};
