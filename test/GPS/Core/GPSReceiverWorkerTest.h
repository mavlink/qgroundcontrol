#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

class GPSReceiverWorkerTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _positionFixTransitions();
    void _ancillaryTraffic_data();
    void _ancillaryTraffic();
    void _satelliteExpiryDoesNotRenewLiveness();
    void _passiveInputProblems_data();
    void _passiveInputProblems();
    void _inputMonitorClassification();
#ifndef QGC_NO_SERIAL_LINK
    void _finishedReceiverReleasesReservation_data();
    void _finishedReceiverReleasesReservation();
#endif
    void _transportLifetimeStaysOnWorker_data();
    void _transportLifetimeStaysOnWorker();
    void _missingTransportReportsOpenFailure_data();
    void _missingTransportReportsOpenFailure();
    void _workerLifecycle();
    void _configuredReceiverReportsReadyThenLoss_data();
    void _configuredReceiverReportsReadyThenLoss();
    void _quectelConsentRefusal_data();
    void _quectelConsentRefusal();
};
