#pragma once

#include "UnitTest.h"

class GPSSatelliteStoreTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _fullSnapshotsReplaceAndDeltasPreserve();
    void _constellationRetirement();
    void _viewAndUseExpireIndependently();
    void _timerKeepsFreshConstellation();
    void _sessionsAndReentrantDelivery();
    void _unknownUsageRetiresPreviousCount();
    void _independentRetirement_data();
    void _independentRetirement();
};
