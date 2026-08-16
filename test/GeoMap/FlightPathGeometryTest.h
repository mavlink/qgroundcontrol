#pragma once

#include "UnitTest.h"

class FlightPathGeometryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _emptyAndShortPaths();
    void _ribbonLayout();
    void _directionsAndSides();
    void _pathMutations();
    void _incrementalMatchesRebuild();
};
