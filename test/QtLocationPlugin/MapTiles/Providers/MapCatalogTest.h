#pragma once

#include "UnitTest.h"

class MapCatalogTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testIsSatelliteMap_data();
    void _testIsSatelliteMap();
    void _testIsDarkMap_data();
    void _testIsDarkMap();
    void _testIsImageryMap_data();
    void _testIsImageryMap();
    void _testMaxZoom_data();
    void _testMaxZoom();
    void _testProviderAndTypeSelectEveryMap();
    void _testMapWithoutServerIsNotDownloadable();
    void _testChosenMap_data();
    void _testChosenMap();
};
