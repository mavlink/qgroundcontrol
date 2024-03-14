
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtLocation
import QtPositioning
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Vehicle
import QGroundControl.FlightMap

QGCPopupDialog {
    title: "Select one action"
    property var  acceptFunction:     null
    buttons:  Dialog.Cancel

    onRejected:{
        _guidedController._gripperFunction = Vehicle.Invalid_option
        _guidedController.closeAll()
        close()
    }

    onAccepted: {
        if (acceptFunction) {
            _guidedController._gripperFunction = Vehicle.Invalid_option
            close()
        }
    }

    RowLayout {
        QGCColumnButton {
            id: grabButton
            text:                   "Grab"
            iconSource:             "/res/GripperGrab.svg"
            font.pointSize:         ScreenTools.defaultFontPointSize * 3.5
            backRadius:             width / 40
            heightFactor:           0.75
            Layout.preferredHeight: releaseButton.height
            Layout.preferredWidth:  releaseButton.width

            onClicked: {
                _guidedController._gripperFunction = Vehicle.Gripper_grab
                close()
            }
        }

        QGCColumnButton {
            id: releaseButton
            text:                   "Release"
            iconSource:             "/res/GripperRelease.svg"
            font.pointSize:         ScreenTools.defaultFontPointSize * 3.5
            backRadius:             width / 40
            heightFactor:           0.75
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 27
            Layout.preferredHeight: Layout.preferredWidth / 1.20

            onClicked: {
                _guidedController._gripperFunction = Vehicle.Gripper_release
                close()
            }
        }
    }
}
