#pragma once

#include "UnitTest.h"

class QGCFormatTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _numberToStringZero_test();
    void _numberToStringLarge_test();
    void _bigSizeToStringBytes_test();
    void _bigSizeToStringKB_test();
    void _bigSizeToStringMB_test();
    void _bigSizeMBToStringMB_test();
    void _bigSizeMBToStringGB_test();
};
