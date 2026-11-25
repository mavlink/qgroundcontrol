#pragma once

#include "UnitTest.h"

class QGCNetworkAvailabilityMonitorTest : public UnitTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_notifiesChangesOnly);
};
