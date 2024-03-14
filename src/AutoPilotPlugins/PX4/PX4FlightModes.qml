/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.ScreenTools

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
