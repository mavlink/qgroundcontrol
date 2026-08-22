import QtQuick
import QtLocation

import QGroundControl
import QGroundControl.Controls

/// Marker for displaying a camera trigger on the map
MapQuickItem {
    anchorPoint.x:  sourceItem.width / 2
    anchorPoint.y:  sourceItem.height / 2

    sourceItem: CameraTriggerIcon { }
}
