#pragma once

#include <QtCore/QPointer>

#include "UnitTest.h"

class QIODevice;
class NMEASourceManager;

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
    void _nmeaLifecycleDiagnostics_data();
    void _nmeaLifecycleDiagnostics();
    void _simulatedPosition_data();
    void _simulatedPosition();
    void _facadeUsesInjectedScheduler();
    void _sourceSettingSelectsMode();
    void _simulatedHomeSelection_data();
    void _simulatedHomeSelection();

private:
    QIODevice* _nmeaDevice = nullptr;
    NMEASourceManager* _nmeaInput = nullptr;
    QPointer<NMEASourceManager> _previousNmeaInput;
};
