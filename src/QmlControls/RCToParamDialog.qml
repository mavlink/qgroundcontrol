/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5
import QtQuick.Dialogs  1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0

QGCPopupDialog {
    property alias tuningFact: controller.tuningFact

    title:      qsTr("RC To Param")
    buttons:    StandardButton.Cancel | StandardButton.Ok

    function accept() {
        QGroundControl.multiVehicleManager.activeVehicle.sendParamMapRC(tuningFact.name, scale.text, centerValue.text, tuningID.currentIndex, minValue.text, maxValue.text);
        hideDialog()
    }

    RCToParamDialogController {
        id: controller
    }

    ColumnLayout {
        spacing: ScreenTools.defaultDialogControlSpacing

        QGCLabel {
            Layout.preferredWidth:  mainGrid.width
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            text:                   qsTr("Bind an RC Channel to a parameter value. Tuning IDs can be mapped to an RC Channel from Radio Setup page.")
        }

        QGCLabel {
            Layout.preferredWidth:  mainGrid.width
            Layout.fillWidth:       true
            text:                   qsTr("Waiting on parameter update from Vehicle.")
            visible:                !controller.ready
        }

        GridLayout {
            id:             mainGrid
            columns:        2
            rowSpacing:     ScreenTools.defaultDialogControlSpacing
            columnSpacing:  ScreenTools.defaultDialogControlSpacing
            enabled:        controller.ready

            QGCLabel { text: qsTr("Parameter") }
            QGCLabel { text: tuningFact.name }

            QGCLabel { text: qsTr("Tuning ID") }
            QGCComboBox {
                id:                 tuningID
                Layout.fillWidth:   true
                currentIndex:       0
                model:              [ 1, 2, 3 ]
            }

            QGCLabel { text: qsTr("Scale") }
            QGCTextField {
                id:     scale
                text:   controller.scale.valueString
            }

            QGCLabel { text: qsTr("Center Value") }
            QGCTextField {
                id:     centerValue
                text:   controller.center.valueString
            }

            QGCLabel { text: qsTr("Min Value") }
            QGCTextField {
                id:     minValue
                text:   controller.min.valueString
            }

            QGCLabel { text: qsTr("Max Value") }
            QGCTextField {
                id:     maxValue
                text:   controller.max.valueString
            }
        }

        QGCLabel {
            Layout.preferredWidth:  mainGrid.width
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            text:                   qsTr("Double check that all values are correct prior to confirming dialog.")
        }
    }
}
