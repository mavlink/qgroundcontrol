#pragma once

#include "PortableTest.h"

class QGCFormatTest : public PortableTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_numberToString);
    UT_PARAMETERIZED_TEST(_bigSizeToString);
    UT_PARAMETERIZED_TEST(_bigSizeMBToString);
};
