import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

ToolStrip {
    id: _root

    FlyViewToolStripActionList {
        id: flyViewToolStripActionList
    }

    model: flyViewToolStripActionList.model
}
