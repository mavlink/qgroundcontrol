#pragma once

#include "UnitTest.h"

class PositionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sourcesShareAcceptance_data();
    void _sourcesShareAcceptance();
    void _deviceAltitudeDatum_data();
    void _deviceAltitudeDatum();
    void _registrationReplacementAndSessions();
    void _producerLoss_data();
    void _producerLoss();
    void _selectionStatusMatchesPublication_data();
    void _selectionStatusMatchesPublication();
    void _automaticRejectionMatchesPublication_data();
    void _automaticRejectionMatchesPublication();
    void _automaticFailoverAndRecovery();
    void _standbyReportsDoNotRepublish();
    void _consumerMaximumAge();
    void _policySelectionGates();
    void _sourceAndHealthLifetime();
    void _backendStatus();
    void _backendTimeoutPreservesFreshness_data();
    void _backendTimeoutPreservesFreshness();
    void _deviceRunsUnlessReceiverOnly();
    void _accuracyNotifiesOnlyChanges();
    void _destructionStopsDevice();
    void _qmlPositionProperties();
    void _initUsesPlatformSourceFactory();
    void _shutdownReleasesSources();
};
