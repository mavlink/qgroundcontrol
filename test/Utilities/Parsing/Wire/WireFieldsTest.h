#pragma once

#include "PortableTest.h"

class WireFieldsTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _roundTrip();
    UT_PARAMETERIZED_TEST(_truncatedInput);
    void _littleEndianScalars();
};
