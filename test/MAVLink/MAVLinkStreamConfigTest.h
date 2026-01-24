#pragma once

#include "TestFixtures.h"

#include <QtCore/QList>
#include <QtCore/QPair>

/// Unit tests for MAVLinkStreamConfig state machine.
class MAVLinkStreamConfigTest : public OfflineTest
{
    Q_OBJECT

public:
    MAVLinkStreamConfigTest() = default;

private slots:
    void init();

    // State machine tests
    void _initialStateTest();
    void _setHighRateRateAndAttitudeTest();
    void _setHighRateVelAndPosTest();
    void _setHighRateAltAirspeedTest();
    void _restoreDefaultsTest();
    void _stateInterruptionTest();

private:
    QList<QPair<int, int>> _messageIntervalCalls;
    void _recordMessageInterval(int messageId, int rate);
};
