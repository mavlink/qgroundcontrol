#pragma once

#include "UnitTest.h"

class QGCMapCatalogTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testIsSatelliteMap_data();
    void _testIsSatelliteMap();
    void _testIsImageryMap_data();
    void _testIsImageryMap();
    void _testProviderAndTypeSelectEveryMap();
    void _testChosenMap_data();
    void _testChosenMap();
};
