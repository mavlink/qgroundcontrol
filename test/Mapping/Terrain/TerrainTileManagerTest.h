#pragma once

#include "UnitTest.h"

class TerrainTileManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testUndecodableCachedTileIsFetchedAgain();
};
