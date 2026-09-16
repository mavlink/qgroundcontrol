#pragma once

#include "UnitTest.h"

class QIODevice;

class PositionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _nmeaSourceProducesGcsPosition();
    void _resetNmeaSourceTearsDownAndClearsState();
    void _nmeaUpdatesStayHealthyUntilStale();
    void _qmlPositionProperties();
    void _destructionDoesNotPublishPosition();
    void _deviceDestructionRetiresNmea();
    void _simulatedPosition_data();
    void _simulatedPosition();
    void _facadeUsesInjectedScheduler();
    void _facadeSchedulerDestruction();
    void _simulatedHomeSelection_data();
    void _simulatedHomeSelection();

private:
    QIODevice *_nmeaDevice = nullptr;
};
