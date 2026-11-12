#pragma once

#include "UnitTest.h"

class Viewer3DMapProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testOsmParserIsMapProvider();
    void _testInitialMapNotLoaded();
    void _testGpsRefInitiallyInvalid();
    void _testBoundingBoxDefault();
    void _testGpsRefSignalEmission();
    void _testMapChangedSignalEmission();
};
