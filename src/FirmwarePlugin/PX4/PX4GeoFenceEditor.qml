import QtQuick          2.7
import QtQuick.Controls 1.4

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Controllers   1.0

Column {
    id:         editorColumn
    width:      availableWidth
    spacing:    _margin

    //property real availableWidth - Available width for control - Must be passed in from Loader

    readonly property real _margin:         ScreenTools.defaultFontPixelWidth / 2
    readonly property real _editFieldWidth: Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)

    property Fact _fenceAction:     factController.getParameterFact(-1, "GF_ACTION")
    property Fact _fenceRadius:     factController.getParameterFact(-1, "GF_MAX_HOR_DIST")
    property Fact _fenceMaxAlt:     factController.getParameterFact(-1, "GF_MAX_VER_DIST")

    FactPanelController {
        id:         factController
        factPanel:  qgcView.viewPanel
    }

    QGCLabel { text: qsTr("Fence Settings:") }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         1
        color:          qgcPal.text
    }

    QGCLabel { text: qsTr("Action on fence breach:") }
    FactComboBox {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        width:              _editFieldWidth
        fact:               _fenceAction
        indexModel:         false
    }

    Item { width:  1; height: 1 }

    QGCCheckBox {
        id:                 circleFenceCheckBox
        text:               qsTr("Circular fence around home pos")
        checked:            _fenceRadius.value > 0
        onClicked:          _fenceRadius.value = checked ? 100 : 0
    }

    Row {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        spacing:        _margin

        QGCLabel {
            anchors.baseline: fenceRadiusField.baseline
            text: qsTr("Radius:")
        }

        FactTextField {
            id:                 fenceRadiusField
            showUnits:          true
            fact:               _fenceRadius
            enabled:            circleFenceCheckBox.checked
            width:              _editFieldWidth
        }
    }

    Item { width:  1; height: 1 }

    QGCCheckBox {
        id:                 maxAltFenceCheckBox
        text:               qsTr("Maximum altitude fence")
        checked:            _fenceMaxAlt.value > 0
        onClicked:          _fenceMaxAlt.value = checked ? 100 : 0
    }

    Row {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        spacing:            _margin

        QGCLabel {
            anchors.baseline: fenceAltMaxField.baseline
            text: qsTr("Altitude:")
        }

        FactTextField {
            id:                 fenceAltMaxField
            showUnits:          true
            fact:               _fenceMaxAlt
            enabled:            maxAltFenceCheckBox.checked
            width:              _editFieldWidth
        }
    }
}
