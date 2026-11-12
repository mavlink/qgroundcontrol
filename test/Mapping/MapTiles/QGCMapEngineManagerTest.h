#pragma once

#include "UnitTest.h"

class QGCMapEngineManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTerrainSet_data();
    void _testTerrainSet();
    void _testTerrainSetServedFromCache();
    void _testNewSetTellsManagerOfItsSelection();
    void _testMapAttribution_data();
    void _testMapAttribution();
    void _testProviderAndTypeSelectEveryMap();
};
