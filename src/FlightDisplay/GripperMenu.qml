
import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Layouts          1.2

import QGroundControl                           1.0
import QGroundControl.ScreenTools               1.0
import QGroundControl.Controls                  1.0
import QGroundControl.Palette                   1.0
import QGroundControl.Vehicle                   1.0
import QGroundControl.FlightMap                 1.0

Component {
    id: messageDialogComponent  
    QGCPopupDialog {
        title: "Select one action"
        property var  acceptFunction:     null
        buttons:  StandardButton.Cancel

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
                pointSize:              ScreenTools.defaultFontPointSize * 3.5 
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
                pointSize:              ScreenTools.defaultFontPointSize * 3.5
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
}
