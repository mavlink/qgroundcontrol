#pragma once

#include "UnitTest.h"

class GPSPositionServiceTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _sourcesShareAcceptance_data();
    void _sourcesShareAcceptance();
    void _registrationReplacementAndSessions();
    void _sharedProducerRoles();
    void _sharedHealthLoss_data();
    void _sharedHealthLoss();
    void _rawSourceSharingRejected_data();
    void _rawSourceSharingRejected();
    void _selectionStatusMatchesPublication_data();
    void _selectionStatusMatchesPublication();
    void _automaticRejectionMatchesPublication_data();
    void _automaticRejectionMatchesPublication();
    void _automaticFailoverAndRecovery();
    void _sourceAndHealthLifetime();
    void _notificationsCanSwitchOrDelete_data();
    void _notificationsCanSwitchOrDelete();
    void _backendStatus();
    void _backendTimeoutPreservesFreshness_data();
    void _backendTimeoutPreservesFreshness();
    void _adapterReentrantReplacement();
    void _adapterNestedBackendEvent_data();
    void _adapterNestedBackendEvent();
    void _adapterReentrantDeactivation_data();
    void _adapterReentrantDeactivation();
    void _nestedObservationNotifications();
    void _accuracyNotifiesOnlyChanges();
    void _rawRegistrationCarriesSession();
    void _registrationRetiresFromWorker();
    void _foreignSchedulerRejected();
};
