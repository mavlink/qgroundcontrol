/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.5
import QtQuick.Controls     1.4
import QtQuick.Layouts      1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0


// Mixer Tuning setup page
SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }

    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10      // Dynamic adjusted at runtime

    ListModel {
        id: mockList
      ListElement { name: "1.jpg"; value: "flower" }
      ListElement { name: "2.jpg"; value: "house" }
      ListElement { name: "3.jpg"; value: "water" }
    }

//    Component {
//        id: upperControls
//        Row {
//            id: row
//            spacing: ScreenTools.defaultFontPixelWidth * 2
//            QGCPalette { id: palette; colorGroupEnabled: true }

//            QGCLabel { text: qsTr("Group") }

//            QGCTextField {
//                id:                 groupText
//                text:               "0"
//            }

//            QGCButton {
//                id:getMixersCountButton
//                text: qsTr("Request mixer count")
//                onClicked: {
//                    mixers.getMixersCountButtonClicked()
//                }
//            }

//            QGCButton {
//                id:requestAllButton
//                text: qsTr("Request all")
//                onClicked: {
//                    mixers.requestAllButtonClicked()
//                }
//            }

//            QGCButton {
//                id:requestMissing
//                text: qsTr("Request missing")
//                onClicked: {
//                    mixers.requestGUIButtonClicked()
//                }
//            }

//            QGCButton {
//                id:refreshGUI
//                text: qsTr("REFRESH GUI")
//                onClicked: {
//                    mixers.refreshGUIButtonClicked()
//                }
//            }
//        } // Row
//    }

    Component {
        id: tuningPageComponent

        Item {
            width:      availableWidth
            height:     availableHeight

            FactPanelController { id: controller; factPanel: tuningPage.viewPanel }

            MixersComponentController {
                id:         mixers
                factPanel:  tuningPage.viewPanel
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            RowLayout {
                anchors.fill: parent

                /// Parameter list
                QGCListView {
                    id:                 mixerListView
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    width:              ScreenTools.defaultFontPixelWidth  * 30
                    orientation:        ListView.Vertical
                    model:              mixers.mixersList
//                    model:              mockList
                    cacheBuffer:        height > 0 ? height * 2 : 0
                    clip:               true

                    delegate: Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         30
                        color:          "red"
                        border.color:   "white"

    //                    height: _rowHeight
    //                    width:  _rowWidth
    //                    color:  Qt.rgba(0,0,0,0)

                        Row {
                            id:     factRow
                            spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                            anchors.verticalCenter: parent.verticalCenter

                            property Fact modelFact: object

                            QGCLabel {
                                id:     mixerIDLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
//                               text:   name
                                text:   factRow.modelFact.name
                                clip:   true
                            }

                            QGCLabel {
                                id:     mixerTypeLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
//                                text:   value
                                text:   factRow.modelFact.valueString
                                clip:   true
                            }

//                            Component.onCompleted: {
//                                if(_rowWidth < factRow.width + ScreenTools.defaultFontPixelWidth) {
//                                    _rowWidth = factRow.width + ScreenTools.defaultFontPixelWidth
//                                }
//                            }
                        }

    //                    Rectangle {
    //                        width:  _rowWidth
    //                        height: 1
    //                        color:  __qgcPal.text
    //                        opacity: 0.15
    //                        anchors.bottom: parent.bottom
    //                        anchors.left:   parent.left
    //                    }

    //                    MouseArea {
    //                        anchors.fill:       parent
    //                        acceptedButtons:    Qt.LeftButton
    //                        onClicked: {
    //                            _editorDialogFact = factRow.modelFact
    ////                            showDialog(editorDialogComponent, qsTr("Mixer Editor"), qgcView.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Save)
    //                        }
    //                    }
                    } //Row
                } //QGCListView

                ColumnLayout {
                    Layout.fillWidth:   true
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom

                    Row {
                        id: controlsRow
                        spacing: ScreenTools.defaultFontPixelWidth * 2
                        QGCPalette { id: palette; colorGroupEnabled: true }

                        QGCLabel { text: qsTr("Group") }

                        QGCTextField {
                            id:                 groupText
                            text:               "0"
                        }

                        QGCButton {
                            id:getMixersCountButton
                            text: qsTr("Request mixer count")
                            onClicked: {
                                mixers.getMixersCountButtonClicked()
                            }
                        }

                        QGCButton {
                            id:requestAllButton
                            text: qsTr("Request all")
                            onClicked: {
                                mixers.requestAllButtonClicked()
                            }
                        }

                        QGCButton {
                            id:requestMissing
                            text: qsTr("Request missing")
                            onClicked: {
                                mixers.requestGUIButtonClicked()
                            }
                        }

                        QGCButton {
                            id:refreshGUI
                            text: qsTr("REFRESH GUI")
                            onClicked: {
                                mixers.refreshGUIButtonClicked()
                            }
                        }
                    } // Row

                    Rectangle {
                        Layout.fillWidth:   true
                        anchors.top:        controlsRow.bottom
                        anchors.bottom:     parent.bottom
                        color:              qgcPal.windowShade
                   }
                } //ColumnLayout

            } //RowLayout
        } // Item
    } // Component
} // SetupView


