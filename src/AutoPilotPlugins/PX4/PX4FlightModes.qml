/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

/// PX4 Flight Mode configuration. This control will load either the Simple or Advanced Flight Mode config
/// based on current parameter settings.
SetupPage {
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        Loader {
            width:  availableWidth
            height: availableHeight
            source: "qrc:/qml/PX4SimpleFlightModes.qml"

            property Fact _nullFact
            property bool _rcMapFltmodeExists:  controller.parameterExists(-1, "RC_MAP_FLTMODE")
            property Fact _rcMapFltmode:        _rcMapFltmodeExists ? controller.getParameterFact(-1, "RC_MAP_FLTMODE") : _nullFact

            FactPanelController {
                id:         controller
            }
        }
    }
}
