#pragma once

#include "UnitTest.h"

class GPSPositionFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _foreignStoresRejected();
    void _integrityStoreDestruction();
    void _reentrantAvailabilityNotification_data();
    void _reentrantAvailabilityNotification();
    void _receiverMetadata();
    void _independentIntegrityReports();
    void _integrityExpiryAndReentrancy();
    void _localObservation();
    void _resetDuringUpdate();
};
