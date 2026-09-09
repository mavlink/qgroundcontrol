#pragma once

#include "UnitTest.h"

class GPSSatelliteStoreTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _constellationRetirement();
    void _viewAndUseExpireIndependently();
    void _timerKeepsFreshConstellation();
    void _sessionsAndReentrantDelivery();
};
