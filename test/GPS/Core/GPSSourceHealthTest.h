#pragma once

#include "UnitTest.h"

class GPSSourceHealthTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _retainedMeasurementExpires_data();
    void _retainedMeasurementExpires();
    void _normalizesObservation_data();
    void _normalizesObservation();
    void _ageAndRecovery();
    void _independentSatelliteExpiry();
    void _settingsStatus();
    void _consumerAcceptancePolicies_data();
    void _consumerAcceptancePolicies();
    void _fixSatelliteCountsTakePrecedence();
    void _resetDuringMetadataNotification();
    void _remoteIdUsesKnownEllipsoidAltitude();
    void _rawPoliciesPreserveMeasurementsAndRespectInvalidation();
};
