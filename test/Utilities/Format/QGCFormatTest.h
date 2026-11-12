#pragma once

#include "PortableTest.h"

class QGCFormatTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _numberToString_data();
    void _numberToString();
    void _bigSizeToString_data();
    void _bigSizeToString();
    void _bigSizeMBToString_data();
    void _bigSizeMBToString();
};
