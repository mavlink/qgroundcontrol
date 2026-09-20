#pragma once

#include "../Driver/GPSDriverTestBase.h"

class GPSProviderTest : public GPSDriverTestBase
{
    Q_OBJECT

private slots:
    void _queuedPayloadsOwnSnapshots();
    void _ancillaryTraffic_data();
    void _ancillaryTraffic();
    void _satelliteExpiryDoesNotRenewLiveness();
    void _surveyReportProjection_data();
    void _surveyReportProjection();
#ifndef QGC_NO_SERIAL_LINK
    void _finishedReceiverReleasesReservation_data();
    void _finishedReceiverReleasesReservation();
#endif
    void _transportLifetimeStaysOnWorker_data();
    void _transportLifetimeStaysOnWorker();
    void _missingTransportReportsOpenFailure_data();
    void _missingTransportReportsOpenFailure();
    void _cancelledProviderDoesNotCreateTransport();
    void _cancelledFactoryDoesNotOpenTransport();
    void _configuredReceiverReportsReadyThenLoss_data();
    void _configuredReceiverReportsReadyThenLoss();
    void _unsupportedPositionRoleReportsConfigFailure_data();
    void _unsupportedPositionRoleReportsConfigFailure();
};
