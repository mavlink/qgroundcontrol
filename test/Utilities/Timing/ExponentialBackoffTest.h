#pragma once

#include "PortableTest.h"

class ExponentialBackoffTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _growsToCapAndResets();
    void _fractionalFactor();
    UT_PARAMETERIZED_TEST(_retryAfter);
};
