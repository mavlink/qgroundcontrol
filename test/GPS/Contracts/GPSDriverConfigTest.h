#pragma once

#include "UnitTest.h"

class GPSDriverConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _rejectUnsupportedConfiguration_data();
    void _rejectUnsupportedConfiguration();
};
