#pragma once

#include "UnitTest.h"

/// Tests for the credit of a map and of the terrain. Those of the area that a view shows are in EsriAttributionTest.
class QGCMapCreditsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testMapAttribution_data();
    void _testMapAttribution();
    void _testTerrainAttribution();
};
