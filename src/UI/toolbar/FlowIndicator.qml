import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools


import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
    id:             control
    width:          flowIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Row {
        id:             flowIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            id:                 flowIcon
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "/qmlimages/flow.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            opacity:            _activeVehicle ? 1 : 0.5
            color:              qgcPal.buttonText
        }

        Column {
            id:                     flowValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                true   // later bind to sensor available
            spacing:                0

           QGCLabel {
               anchors.horizontalCenter: flowValue.horizontalCenter
               color: qgcPal.buttonText
               text: _activeVehicle && !isNaN(_activeVehicle.flowSensor.flowRate.value)
                     ? _activeVehicle.flowSensor.flowRate.value.toFixed(2)
                     : "--"
            }

            QGCLabel {
               id: flowValue
               color: qgcPal.buttonText
               text: _activeVehicle && !isNaN(_activeVehicle.flowSensor.pulseCount.value)
                     ? _activeVehicle.flowSensor.pulseCount.value.toFixed(0):"--"
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(flowIndicatorPage, control)
    }

    Component {
        id: flowIndicatorPage

        FlowIndicatorPage { }
    }
}
