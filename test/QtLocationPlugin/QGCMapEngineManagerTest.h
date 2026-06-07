#pragma once

#include "UnitTest.h"

class QGCMapEngineManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void _testInstanceAndModelAvailable();
    void _testProviderListsAndMapTypes();
    void _testUpdateForCurrentViewUpdatesTotals();
    void _testFindNameAndSelectionHelpers();
    void _testRenameOptimisticallyUpdatesName();
    void _testTaskErrorLabels();

private:
    int _initialTileSetCount = 0;
};
