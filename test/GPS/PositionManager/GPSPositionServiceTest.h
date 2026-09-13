#pragma once

#include "UnitTest.h"

class GPSPositionServiceTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _sourcesShareAcceptance_data();
    void _sourcesShareAcceptance();
    void _registrationReplacementAndSessions();
    void _automaticFailoverAndRecovery();
    void _sourceAndHealthLifetime();
    void _notificationsCanSwitchOrDelete();
    void _backendStatus();
    void _adapterReentrantReplacement();
    void _nestedObservationNotifications();
    void _rawRegistrationCarriesSession();
    void _registrationRetiresFromWorker();
    void _foreignSchedulerRejected();
};
