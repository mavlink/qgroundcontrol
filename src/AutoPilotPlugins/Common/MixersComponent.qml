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

//TODO Look at QGroundControl.Mixer module again
//import QGroundControl.Mixer         1.0



// Mixer Tuning setup page
SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }

    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10      // Dynamic adjusted at runtime
    property MixerParameter   _editorParameter: MixerParameter { }

    Component {
        id: tuningPageComponent

        Item {
            width:      availableWidth
            height:     availableHeight

            FactPanelController { id: controller; factPanel: tuningPage.viewPanel }

            MixersComponentController {
                id:         mixers
                factPanel:  tuningPage.viewPanel
                mixersManagerStatusText:     mixersManagerStatusText
            }

            Component.onCompleted: {
                mixers.guiUpdated()
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            RowLayout {
                anchors.fill: parent

                /// Mixer list
                QGCListView {
                    id:                 mixerListView
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    width:              ScreenTools.defaultFontPixelWidth  * 100
                    orientation:        ListView.Vertical
                    model:              mixers.mixersList
//                    model:              mockList
                    cacheBuffer:        height > 0 ? height * 2 : 0
                    clip:               true
                    focus:              true

                    delegate: Rectangle {
                        id: mixerParamDelegate
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         30
                        color:          "black"
                        border.color:   "dark grey"
//                        focus: true
//                        highlight: Rectangle {
//                            color: "lightblue"
//                            width: parent.width
//                        }

    //                    height: _rowHeight
    //                    width:  _rowWidth
    //                    color:  Qt.rgba(0,0,0,0)

                        property MixerParameter mixerParamInfo: object

                        Row {
                            id:     factRow
                            spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                            anchors.verticalCenter: parent.verticalCenter

                            property MixerParameter modelFact: object

                            QGCLabel {
                                id:     mixerIDLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
                                text:   mixerParamDelegate.mixerParamInfo.index
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                clip:   true
                                color:  "white"
                            }

                            QGCLabel {
                                id:     mixerTypeLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
                                text:   mixerParamDelegate.mixerParamInfo.Type
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                clip:   true
                                color:  "white"
                            }

                            QGCLabel {
                                id:     submixerIDLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
                                text:   mixerParamDelegate.mixerParamInfo.submixerID
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                clip:   true
                                color:  "white"
                            }

                            QGCLabel {
                                id:     paramNameLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
                                text:   mixerParamDelegate.mixerParamInfo.param.name
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                clip:   true
                                color:  "white"
                            }

                            QGCLabel {
                                id:     paramValueLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 10
                                text:   mixerParamDelegate.mixerParamInfo.param.valueString
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                clip:   true
                                color:  "white"
                            }

//                            Component.onCompleted: {
//                                if(_rowWidth < factRow.width + ScreenTools.defaultFontPixelWidth) {
//                                    _rowWidth = factRow.width + ScreenTools.defaultFontPixelWidth
//                                }
//                            }
                        } // Row

                        MouseArea {
                            anchors.fill:       parent
                            acceptedButtons:    Qt.LeftButton
                            onClicked: {
                                _editorParameter = mixerParamDelegate.mixerParamInfo
//                                root.reject()
                            }
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
                    } //Rectangle
                } //QGCListView

                ColumnLayout {
                    Layout.fillWidth:   true
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    spacing:            ScreenTools.defaultFontPixelHeight

                    Row {
                        id: controlsRow
                        spacing: ScreenTools.defaultFontPixelWidth * 2
                        QGCPalette { id: palette; colorGroupEnabled: true }

                        QGCLabel { text: qsTr("Group") }

                        QGCComboBox {
                            id:                 mixerGroupCombo
                            anchors.top:        parent.top
                            width:              ScreenTools.defaultFontPixelWidth * 30
                            model: ListModel {
                                id: comboBoxModel
                                ListElement { groupName: "LOCAL";       groupID: 1 }
                                ListElement { groupName: "FAILSAFE";    groupID: 2 }
                            }
                            currentIndex:       mixers.selectedGroup - 1
                            textRole:           "groupName"
                            focus:              true

                            onCurrentIndexChanged: {
                                    mixers.selectedGroup = currentIndex + 1
                            }
//                            onActivated: mixers.selectedGroup = get(index)
                        }

//                        QGCButton {
//                            id:refreshGUI
//                            text: qsTr("REFRESH GUI")
//                            onClicked: {
//                                mixers.refreshGUIButtonClicked()
//                            }
//                        }

                        // Status Text
                        QGCLabel {
                            id:         mixersManagerStatusText
                            width:      ScreenTools.defaultFontPixelWidth * 30
                            wrapMode:   Text.WordWrap
                            font.pointSize: ScreenTools.largeFontPointSize
                        }

                        QGCLabel {
                            id:     selectedParamIDLabel
                            width:  ScreenTools.defaultFontPixelWidth  * 10
                            text:   mixers.selectedParamID
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            clip:   true
                            color:  "white"
                        }

                    } // Row


                    QGCLabel {
                        id:     paramValueLabel2
                        width:  ScreenTools.defaultFontPixelWidth  * 10
//                        Property MixerParameter mixParam:  mixers.selectedParam
//                        text:   mixers.selectedParamID
                        text:  _editorParameter.param.name
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        clip:   true
                        color:  "white"
                    }

                    Rectangle {
                        Layout.fillWidth:   true
                        anchors.top:        paramValueLabel2.bottom
                        anchors.bottom:     parent.bottom
                        color:              qgcPal.windowShade
                   }
                } //ColumnLayout

            } //RowLayout
        } // Item
    } // Component
} // SetupView


