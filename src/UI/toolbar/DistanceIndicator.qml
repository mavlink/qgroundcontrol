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
            source:             "/qmlimages/obs_distance.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            opacity:            _activeVehicle ? 1 : 0.5
            color:              qgcPal.buttonText
        }

        Column {
            id:                     flowValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                _activeVehicle && _activeVehicle.distanceSensors ? true : false  // later bind to sensor available
            spacing:                0

            QGCLabel {
                text: _activeVehicle && !isNaN(_activeVehicle.distanceSensors.rotationNone.value)
                      ? _activeVehicle.distanceSensors.rotationNone.value.toFixed(2) + " m (Forward)"
                      : "--"
            }
            QGCLabel {
                text: control._activeVehicle && !isNaN(_activeVehicle.distanceSensors.rotationYaw180.value)
                      ? _activeVehicle.distanceSensors.rotationYaw180.value.toFixed(2) + " m (Rear)"
                      : "--"
            }
            QGCLabel {
                text: _activeVehicle && !isNaN(_activeVehicle.distanceSensors.rotationPitch270.value)
                      ? _activeVehicle.distanceSensors.rotationPitch270.value.toFixed(2) + " m (Down)"
                      : "--"
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(distanceIndicatorPage, control)
    }

    Component {
        id: distanceIndicatorPage

        DistanceIndicatorPage { }
    }
}
