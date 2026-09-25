#pragma once

#include "UnitTest.h"

class ExponentialBackoffTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _growsToCapAndResets();
    void _retryAfter_data();
    void _retryAfter();
};
