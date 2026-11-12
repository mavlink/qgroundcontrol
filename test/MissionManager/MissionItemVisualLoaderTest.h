#pragma once

#include "UnitTest.h"

/// Tests how the maps load the visual of each mission item: MissionItemMapVisual.qml on the Plan and Fly View maps,
/// GeoMapMissionItems.qml on the GeoMap.
class MissionItemVisualLoaderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _itemRemovedWhileItsVisualLoads_data();
    void _itemRemovedWhileItsVisualLoads();
};
