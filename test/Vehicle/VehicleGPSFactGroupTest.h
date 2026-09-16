#pragma once

#include "UnitTest.h"

class VehicleGPSFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sharedFactsAndMetadata();
    void _rawTelemetryEquivalence_data();
    void _rawTelemetryEquivalence();
    void _highLatencyPartialUpdates_data();
    void _highLatencyPartialUpdates();
    void _flatIntegrityNotifications();
    void _typedObservations_data();
    void _typedObservations();
    void _observationReceiptAndInvalidation();
};
