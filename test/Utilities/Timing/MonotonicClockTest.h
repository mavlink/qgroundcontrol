#pragma once

#include "PortableTest.h"

class MonotonicClockTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _fresh_data();
    void _fresh();
    void _age_data();
    void _age();
};
