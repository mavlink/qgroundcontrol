#pragma once

#include "UnitTest.h"

class QGCTileFallbackTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testScaleLevelDelta1Quadrants();
    void _testScaleLevelDelta2SubCell();
    void _testScaleInvalidInputs();
    void _testEncodeFallbackTile();
};
