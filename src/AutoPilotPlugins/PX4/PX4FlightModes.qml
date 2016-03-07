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

import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.1

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

/// PX4 Flight Mode configuration. This control will load either the Simple or Advanced Flight Mode config
/// based on current parameter settings.
QGCView {
    id:         rootQGCView
    viewPanel:  panel

    property Fact _nullFact
    property bool _rcMapFltmodeExists:  controller.parameterExists(-1, "RC_MAP_FLTMODE")
    property Fact _rcMapFltmode:        _rcMapFltmodeExists ? controller.getParameterFact(-1, "RC_MAP_FLTMODE") : _nullFact
    property Fact _rcMapModeSw:         controller.getParameterFact(-1, "RC_MAP_MODE_SW")
    property bool _simpleMode:          _rcMapFltmodeExists ? _rcMapFltmode.value > 0 || _rcMapModeSw.value == 0 : false

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    FactPanelController {
        id:         controller
        factPanel:  panel
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Loader {
            anchors.fill:   parent
            source:         _simpleMode ? "qrc:/qml/PX4SimpleFlightModes.qml" : "qrc:/qml/PX4AdvancedFlightModes.qml"

            property var qgcView:       rootQGCView
            property var qgcViewPanel:  panel
        }
    } // QGCViewPanel
} // QGCView
