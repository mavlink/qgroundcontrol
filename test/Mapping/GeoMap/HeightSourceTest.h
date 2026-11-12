#pragma once

#include "UnitTest.h"

class HeightSourceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _debugTileDeliveredThroughEventLoop();
    void _debugTileRowMajorFromNorthWest();
    void _debugHillsWithinAmplitude();
    void _flatSourceGivesNoTile();
    void _tileRequestRejected();
};
