#pragma once

#include "UnitTest.h"

class VideoStreamResolverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testResolve_data();
    void _testResolve();
    void _testStreamSettingsPassThrough();
    void _testDisplayConfig_data();
    void _testDisplayConfig();
    void _testNetworkSources_data();
    void _testNetworkSources();
};
