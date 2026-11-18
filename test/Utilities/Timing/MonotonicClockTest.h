#pragma once

#include "PortableTest.h"

class MonotonicClockTest : public PortableTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_fresh);
    UT_PARAMETERIZED_TEST(_age);
};
