#pragma once

#include "UnitTest.h"

class CityMapGeometryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaultModelName();
    void _testSetModelName();
    void _testSetMapProviderNull();
    void _testSetMapProvider();
    void _testLoadOsmMapWithoutParser();
    void _testClearViewer();
    void _testSetOsmFilePath();
    void _testUpdateViewerWithoutParser();
};
