#pragma once

#include "BaseClasses/VehicleTest.h"

class AmovUomDeviceAdapterTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _activationRoundTrip_data();
    void _activationRoundTrip();
    void _identityGuard_data();
    void _identityGuard();
    void _receiptFailure_data();
    void _receiptFailure();
    void _multipleDevicesRequireSelection();
    void _concurrentRequestsAndAddressIsolation();
    void _connectionLossInvalidatesLateReply();
    void _responseCorrelation_data();
    void _responseCorrelation();
    void _readTimeout();
};
