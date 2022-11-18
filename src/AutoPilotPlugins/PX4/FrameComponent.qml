import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs  1.2 // Necessary for `StandardButton` enums, defined inside `QMessageBox`.

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id: framePage
    // This is what the loader 'loads'
    pageComponent: framePageComponent

    // Master controller for the frames interface
    property var framesBase:        globals.activeVehicle.framesBase

    // Design constants
    property real _boxSpacing:    ScreenTools.defaultFontPixelWidth

    // Component that will be loaded via `SetupPage`
    Component {
        id: framePageComponent

        // Main Column
        Column {
            id: toolbar
            // Use available w/h specified in `SetupPage.qml`
            width: availableWidth; height: availableHeight

            Button {
                id: gotoParentButton
                text: "Go to parent"
                onClicked: {
                    framesBase.gotoParentFrame();
                }
            }

            // Frames collage view
            Flow {
                id: framesCollageView
                width: parent.width
                spacing: _boxSpacing

                Repeater {
                    id: framesRepeater
                    model: framesBase.selectedFrames // show Selected Frames only

                    PX4Frame {
                        id: frameId
                        frame: object // why do we call it 'object', not 'modelData'?
//                        selected: frame.frame_param_values == framesBase.finalSelectionFrameParamValues

                        // Click border
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                // We directly call the `framesBase` object that exists in the `FrameComponent.qml`.
                                // This is a bad practice, but since having a MouseArea defined in the `Main` QML
                                // somehow disables clicking on the product URL, this decision had to be made.
                                framesBase.selectFrame(frame)

                                // Pop-up dialog if frame is end-node
                                if (frame.isEndNode) {
                                    finalFrameSelectionComponent.createObject(mainWindow, { title: qsTr("Final Frame Selection") }).open()
                                }
                            }
                        }

                    }
                }
            }
        }
    }

    // Pop-up component for final frame selection
    Component {
        id: finalFrameSelectionComponent

        QGCPopupDialog {
            buttons: StandardButton.Cancel | StandardButton.Apply

            onAccepted: {
                // TODO: Handle frame selection logic
            }

            PX4Frame {
                id: finalSelectedFrame
                frame: framesBase.finalSelectedFrame
            }

//            ColumnLayout {
//                spacing: ScreenTools.defaultFontPixelHeight

//                QGCLabel {
//                    Layout.minimumWidth:    ScreenTools.defaultFontPixelWidth * 50
//                    Layout.preferredWidth:  innerColumn.width
//                    wrapMode:               Text.WordWrap
//                    text:                   preCalibrationDialogHelp
//                }

//                Column {
//                    id:         innerColumn
//                    spacing:    parent.spacing

//                    QGCLabel {
//                        id:         boardRotationHelp
//                        wrapMode:   Text.WordWrap
//                        visible:    !_sensorsHaveFixedOrientation && (preCalibrationDialogType == "accel" || preCalibrationDialogType == "compass")
//                        text:       qsTr("Set autopilot orientation before calibrating.")
//                    }

//                    Column {
//                        visible:    boardRotationHelp.visible
//                        QGCLabel { text: qsTr("Autopilot Orientation") }

//                        FactComboBox {
//                            sizeToContents: true
//                            model:          rotations
//                            fact:           sens_board_rot
//                        }

//                        QGCLabel {
//                            wrapMode:   Text.WordWrap
//                            text:       qsTr("ROTATION_NONE indicates component points in direction of flight.")
//                        }
//                    }

//                    QGCLabel {
//                        wrapMode:   Text.WordWrap
//                        text:       qsTr("Click Ok to start calibration.")
//                    }
//                }
//            }
        }
    }
}
