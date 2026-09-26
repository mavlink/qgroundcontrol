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
    void _simulatedReferencePosition();
    void _facadeUsesInjectedScheduler();
    void _configurationSelectsMode();
    void _initSelectsSource_data();
    void _initSelectsSource();
    void _shutdownReleasesSources();
};
