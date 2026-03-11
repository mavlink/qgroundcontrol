#pragma once

#include "UnitTest.h"

class QGCMathTest : public UnitTest
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
    void _limitAngleToPMPIfZero_test();
    void _limitAngleToPMPIfBoundary_test();
    void _limitAngleToPMPIfWrap_test();
    void _limitAngleToPMPIfLargeAngle_test();
    void _limitAngleToPMPIdZero_test();
    void _limitAngleToPMPIdBoundary_test();
    void _limitAngleToPMPIdWrap_test();
    void _limitAngleToPMPIdLargeAngle_test();
    void _crc32Empty_test();
    void _crc32KnownVector_test();
    void _crc32Incremental_test();
};
