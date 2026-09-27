#pragma once

#include "UnitTest.h"

class PositionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _qmlPositionProperties();
    void _destructionDoesNotPublishPosition();
    void _simulatedPosition_data();
    void _simulatedPosition();
    void _facadeUsesInjectedScheduler();
    void _sourceSettingSelectsMode();
    void _shutdownReleasesSources();
    void _simulatedHomeSelection_data();
    void _simulatedHomeSelection();
};
