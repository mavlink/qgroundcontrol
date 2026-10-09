#pragma once

#include "PortableTest.h"

class WireFieldsTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _roundTrip();
    void _truncatedInput_data();
    void _truncatedInput();
    void _littleEndianScalars();
};
