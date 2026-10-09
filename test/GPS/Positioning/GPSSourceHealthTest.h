#pragma once

#include "UnitTest.h"

class GPSSourceHealthTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _invalidatedPositionTimeout();
    void _retainedMeasurementExpires_data();
    void _retainedMeasurementExpires();
    void _normalizesObservation_data();
    void _normalizesObservation();
    void _ageAndRecovery();
    void _logsOnlyHealthTransitions();
    void _consumerPolicies_data();
    void _consumerPolicies();
    void _remoteIdDatum_data();
    void _remoteIdDatum();
    void _ggaDoesNotRequireAccuracy();
    void _freshnessReconfiguration_data();
    void _freshnessReconfiguration();
    void _futureReceiptRemainsRejected_data();
    void _futureReceiptRemainsRejected();
    void _maximumAge_data();
    void _maximumAge();
    void _receiptDeadlineBoundaries();
    void _navigationObservation();
};
