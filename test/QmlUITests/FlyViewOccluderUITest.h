#pragma once

#include "QmlUITestBase.h"

/// UI test for Fly View map following around the widgets covering the map (occluders),
/// run against both map engines (QtLocation and GeoMap).
class FlyViewOccluderUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testVehicleUnderWidgetRecenters_data();
    void _testVehicleUnderWidgetRecenters();
    void _testCollapsedPipOccludesRestoreButtonOnly();
};
