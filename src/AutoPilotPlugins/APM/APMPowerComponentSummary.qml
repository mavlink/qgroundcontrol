/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

FactPanel {
    id:     panel
    width:  grid.width
    height: grid.height
    color:  qgcPal.windowShade

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact battCapacity: controller.getParameterFact(-1, "BATT_CAPACITY")
    property Fact battMonitor:  controller.getParameterFact(-1, "BATT_MONITOR")

    Grid {
        id:         grid
        rows:       2
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Battery monitor:" }
        QGCLabel { text: battMonitor.enumStringValue }

        QGCLabel { text: "Battery capacity:" }
        FactLabel { fact: battCapacity }
    }
}
