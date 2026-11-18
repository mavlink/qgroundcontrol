#pragma once

#include "PortableTest.h"

class QGCMathTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _fuzzyCompareDoubleEqual_test();
    void _fuzzyCompareDoubleZero_test();
    void _fuzzyCompareDoubleNaN_test();
    void _fuzzyCompareDoubleInf_test();
    void _fuzzyCompareFloat_test();
    void _fuzzyCompareCustomTolerance_test();
    void _fuzzyCompareFloatCustomTolerance_test();
    void _limitAngleToPMPI_test_data();
    void _limitAngleToPMPI_test();
    void _crc32Empty_test();
    void _crc32KnownVector_test();
};
