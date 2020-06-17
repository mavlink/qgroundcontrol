/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtQuick.Dialogs      1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

SetupPage {
    id:                 subFramePage
    pageComponent:      subFramePageComponent

    property bool _oldFW:   activeVehicle.versionCompare(3 ,5 ,2) < 0

    APMAirframeComponentController { id: controller; }

    Component {
        id: subFramePageComponent

        Column {
            id:     mainColumn
            width:  availableWidth

            QGCPalette { id: palette; colorGroupEnabled: true }

            property Fact _frameConfig: controller.getParameterFact(-1, "FRAME_CONFIG")

            function setFrameConfig(frame) {
                _frameConfig.value = frame;
            }

            property real _minW:        ScreenTools.defaultFontPixelWidth * 30
            property real _boxWidth:    _minW
            property real _boxSpace:    ScreenTools.defaultFontPixelWidth

            onWidthChanged: {
                computeDimensions()
            }

            Component.onCompleted: computeDimensions()

            function computeDimensions() {
                var sw  = 0
                var rw  = 0
                var idx = Math.floor(mainColumn.width / (_minW + ScreenTools.defaultFontPixelWidth))
                if(idx < 1) {
                    _boxWidth = mainColumn.width
                    _boxSpace = 0
                } else {
                    _boxSpace = 0
                    if(idx > 1) {
                        _boxSpace = ScreenTools.defaultFontPixelWidth
                        sw = _boxSpace * (idx - 1)
                    }
                    rw = mainColumn.width - sw
                    _boxWidth = rw / idx
                }
            }

            ListModel {
                id: subFrameModel

                ListElement {
                    name: "BlueROV1"
                    resource: "qrc:///qmlimages/Frames/BlueROV1.png"
                    paramValue: 0
                }

                ListElement {
                    name: "BlueROV2/Vectored"
                    resource: "qrc:///qmlimages/Frames/Vectored.png"
                    paramValue: 1
                }

                ListElement {
                    name: "Vectored-6DOF"
                    resource: "qrc:///qmlimages/Frames/Vectored6DOF.png"
                    paramValue: 2
                }

                ListElement {
                    name: "SimpleROV-3"
                    resource: "qrc:///qmlimages/Frames/SimpleROV-3.png"
                    paramValue: 4
                }

                ListElement {
                    name: "SimpleROV-4"
                    resource: "qrc:///qmlimages/Frames/SimpleROV-4.png"
                    paramValue: 5
                }

                ListElement {
                    name: "SimpleROV-5"
                    resource: "qrc:///qmlimages/Frames/SimpleROV-5.png"
                    paramValue: 6
                }
            }

            Item {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height: defaultsButton.height

                QGCButton {
                    id: defaultsButton
                    anchors.left: parent.left
                    text:       qsTr("Load Vehicle Default Parameters")
                    onClicked:  mainWindow.showComponentDialog(selectParamFileDialogComponent, qsTr("Load Vehicle Default Parameters"), mainWindow.showDialogDefaultWidth, StandardButton.Close)
                }
            }

            Flow {
                id:         flowView
                width:      parent.width
                spacing:    _boxSpace

                Repeater {
                    model: subFrameModel

                    Rectangle {
                        width:  _boxWidth
                        height: ScreenTools.defaultFontPixelHeight * 14
                        color:  qgcPal.window

                        QGCLabel {
                            id:     title
                            text:   subFrameModel.get(index).name
                        }

                        Rectangle {
                            anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
                            anchors.top:        title.bottom
                            anchors.bottom:     parent.bottom
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            color:              subFrameModel.get(index).paramValue == _frameConfig.value ? qgcPal.buttonHighlight: qgcPal.windowShade

                            Image {
                                anchors.margins:    ScreenTools.defaultFontPixelWidth
                                anchors.top:        parent.top
                                anchors.bottom:     parent.bottom
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                fillMode:           Image.PreserveAspectFit
                                smooth:             true
                                mipmap:             true
                                source:             subFrameModel.get(index).resource
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: setFrameConfig(subFrameModel.get(index).paramValue)
                            }
                        }
                    }
                }// Repeater
            }// Flow
        } // Column
    } // Component


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
                text:               qsTr("Select your vehicle to load the default parameters:")
            }

            Flow {
                anchors.margins:    _margins
                anchors.top:        applyParamsText.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                spacing :           _margins
                layoutDirection:    Qt.Vertical;

                QGCButton {
                    width:  parent.width
                    text:   "Blue Robotics BlueROV2"
                    property var file:   _oldFW ? "Sub/bluerov2-3_5.params" : "Sub/bluerov2-3_5_2.params"

                    onClicked : {
                        controller.loadParameters(file)
                        hideDialog()
                    }
                }

                QGCButton {
                    width:  parent.width
                    text:   "Blue Robotics BlueROV2 Heavy"
                    property var file:  "Sub/bluerov2-heavy-3_5_2.params"

                    onClicked : {
                        controller.loadParameters(file)
                        hideDialog()
                    }
                }
            }
        }
    }
} // SetupPage
