#pragma once

#include "UnitTest.h"

class GPSProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _transportLifetimeStaysOnWorker_data();
    void _transportLifetimeStaysOnWorker();
    void _missingTransportReportsOpenFailure_data();
    void _missingTransportReportsOpenFailure();
    void _cancelledProviderDoesNotCreateTransport();
    void _cancelledFactoryDoesNotOpenTransport();
    void _configuredReceiverReportsReadyThenLoss();
};
