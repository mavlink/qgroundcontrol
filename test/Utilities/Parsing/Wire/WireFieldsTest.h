#pragma once

#include "UnitTest.h"

class WireFieldsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _roundTrip();
    void _truncatedInput_data();
    void _truncatedInput();
};
