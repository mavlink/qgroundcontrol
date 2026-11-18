#pragma once

#include "UnitTest.h"

class QGCMapEngineManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;

    void _testTerrainSet_data();
    void _testTerrainSet();
    void _testElevationTypeSet_data();
    void _testElevationTypeSet();
    void _testTerrainSetServedFromCache();
    void _testNewSetTellsManagerOfItsSelection();
    void _testFailedImportEndsTheImport();
};
