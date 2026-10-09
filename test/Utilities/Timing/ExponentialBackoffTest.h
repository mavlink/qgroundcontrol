#pragma once

#include "PortableTest.h"

class ExponentialBackoffTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _growsToCapAndResets();
    void _fractionalFactor();
    void _retryAfter_data();
    void _retryAfter();
};
