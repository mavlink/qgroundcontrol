#pragma once

#include "UnitTest.h"

class QGCNetworkAvailabilityMonitorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _notifiesChangesOnly_data();
    void _notifiesChangesOnly();
};
