/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.7
import QtQuick.Controls 2.1
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.3

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             airframePage
    pageComponent:  _useOldFrameParam ?  oldFramePageComponent: newFramePageComponent

    property real _margins:             ScreenTools.defaultFontPixelWidth
    property bool _useOldFrameParam:    controller.parameterExists(-1, "FRAME")
    property Fact _oldFrameParam:       controller.getParameterFact(-1, "FRAME", false)
    property Fact _newFrameParam:       controller.getParameterFact(-1, "FRAME_CLASS", false)
    property Fact _frameTypeParam:      controller.getParameterFact(-1, "FRAME_TYPE", false)
    property var  _flatParamList:       ListModel {
        ListElement {
            name: "3DR Aero M"
            file: "3DR_AERO_M.param"
        }
        ListElement {
            name: "3DR Aero RTF"
            file: "3DR_Aero_RTF.param"
        }
        ListElement {
            name: "3DR Rover"
            file: "3DR_Rover.param"
        }
        ListElement {
            name: "3DR Tarot"
            file: "3DR_Tarot.bgsc"
        }
        ListElement {
            name: "Parrot Bebop"
            file: "Parrot_Bebop.param"
        }
        ListElement {
            name: "Storm32"
            file: "SToRM32-MAVLink.param"
        }
        ListElement {
            name: "3DR X8-M RTF"
            file: "3DR_X8-M_RTF.param"
        }
        ListElement {
            name: "3DR Y6A"
            file: "3DR_Y6A_RTF.param"
        }
        ListElement {
            name: "3DR X8+ RTF"
            file: "3DR_X8+_RTF.param"
        }
        ListElement {
            name: "3DR QUAD X4 RTF"
            file: "3DR_QUAD_X4_RTF.param"
        }
        ListElement {
            name: "3DR X8"
            file: "3DR_X8_RTF.param"
        }
        ListElement {
            name: "Iris with GoPro"
            file: "Iris with Front Mount Go Pro.param"
        }
        ListElement {
            name: "Iris with Tarot"
            file: "Iris with Tarot Gimbal.param"
        }
        ListElement {
            name: "3DR Iris+"
            file: "3DR_Iris+.param"
        }
        ListElement {
            name: "Iris"
            file: "Iris.param"
        }
        ListElement {
            name: "3DR Y6B"
            file: "3DR_Y6B_RTF.param"
        }
    }


    APMAirframeComponentController {
        id:         controller
        factPanel:  airframePage.viewPanel
    }

    ExclusiveGroup {
        id: airframeTypeExclusive
    }

    Component {
        id: applyRestartDialogComponent

        QGCViewDialog {
            id: applyRestartDialog

            Connections {
                target: controller
                onCurrentAirframeTypeChanged: {
                    airframePicker.model = controller.currentAirframeType.airframes;
                }
            }

            QGCLabel {
                id:                 applyParamsText
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.margins:    _margins
                wrapMode:           Text.WordWrap
                text:               qsTr("Select your drone to load the default parameters for it. ")
            }

            Flow {
                anchors.margins:    _margins
                anchors.top:        applyParamsText.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                spacing :           _margins
                layoutDirection:    Qt.Vertical;

                Repeater {
                    id:     airframePicker
                    model:  controller.currentAirframeType.airframes;

                    delegate: QGCButton {
                        id:     btnParams
                        width:  parent.width / 2.1
                        height: (ScreenTools.defaultFontPixelHeight * 14) / 5
                        text:   controller.currentAirframeType.airframes[index].name;

                        onClicked : {
                            controller.loadParameters(controller.currentAirframeType.airframes[index].params)
                            hideDialog()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: selectParamFileDialogComponent

        QGCViewDialog {
            QGCLabel {
                id:                 applyParamsText
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.margins:    _margins
                wrapMode:           Text.WordWrap
                text:               qsTr("Select your drone to load the default parameters for it. ")
            }

            Flow {
                anchors.margins:    _margins
                anchors.top:        applyParamsText.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                spacing :           _margins
                layoutDirection:    Qt.Vertical;

                Repeater {
                    id:     airframePicker
                    model:  _flatParamList

                    delegate: QGCButton {
                        width:  parent.width / 2.1
                        height: (ScreenTools.defaultFontPixelHeight * 14) / 5
                        text:   name

                        onClicked : {
                            controller.loadParameters(file)
                            hideDialog()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: oldFramePageComponent

        Column {
            width:      availableWidth
            height:     1000
            spacing:    _margins

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margins

                QGCLabel {
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Please select your airframe type")
                    Layout.fillWidth:   true
                }

                QGCButton {
                    text:       qsTr("Load common parameters")
                    onClicked:  showDialog(applyRestartDialogComponent, qsTr("Load common parameters"), qgcView.showDialogDefaultWidth, StandardButton.Close)
                }
            }

            Repeater {
                model: controller.airframeTypesModel

                QGCRadioButton {
                    text: object.name
                    checked: controller.currentAirframeType == object
                    exclusiveGroup: airframeTypeExclusive

                    onCheckedChanged: {
                        if (checked) {
                            controller.currentAirframeType = object
                        }
                    }
                }
            }
        } // Column
    } // Component - oldFramePageComponent

    Component {
        id: newFramePageComponent

        Grid {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margins
            columns:        2

            QGCLabel {
                text:               qsTr("Frame Class:")
            }

            FactComboBox {
                fact:       _newFrameParam
                indexModel: false
                width:      ScreenTools.defaultFontPixelWidth * 15
            }

            QGCLabel {
                text:               qsTr("Frame Type:")
            }

            FactComboBox {
                fact:       _frameTypeParam
                indexModel: false
                width:      ScreenTools.defaultFontPixelWidth * 15
            }

            QGCButton {
                text:       qsTr("Load common parameters")
                onClicked:  showDialog(selectParamFileDialogComponent, qsTr("Load common parameters"), qgcView.showDialogDefaultWidth, StandardButton.Close)
            }
        }
    }
} // SetupPage
