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
import QtQuick.Dialogs      1.2

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
    property Fact             _editorParameterValue: Fact { }

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
                percentDownloadedText:       percentDownloadedText
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
                                text:   mixerParamDelegate.mixerParamInfo.mixerID
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
                                width:  ScreenTools.defaultFontPixelWidth  * 20
                                text:   mixerParamDelegate.mixerParamInfo.paramName
                                horizontalAlignment:    Text.AlignHCenter
                                verticalAlignment:      Text.AlignVCenter
                                clip:   true
                                color:  "white"
                            }

                            QGCLabel {
                                id:     paramValueLabel
                                width:  ScreenTools.defaultFontPixelWidth  * 20
                                text:   "[ " + mixerParamDelegate.mixerParamInfo.valuesString + " ]"
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
                                if(_editorParameter.values.count > 0) {
                                    _editorParameterValue = _editorParameter.values.get(0)
                                } else {
                                   _editorParameterValue = {}
                                }
                            }
                        }
                    } //Rectangle
                } //QGCListView

                ColumnLayout {
                    Layout.fillWidth:   true
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    spacing:            ScreenTools.defaultFontPixelHeight * 2

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
                        }

                        // Status Text
                        QGCLabel {
                            id:         mixersManagerStatusText
                            width:      ScreenTools.defaultFontPixelWidth * 25
                            wrapMode:   Text.WordWrap
                            font.pointSize: ScreenTools.largeFontPointSize
                        }

                        // Percent Downloaded Text
                        QGCLabel {
                            id:           percentDownloadedText
                            width:        ScreenTools.defaultFontPixelWidth * 25
//                            wrapMode:       Text.WordWrap
                            font.pointSize: ScreenTools.largeFontPointSize
                        }

                        QGCButton {
                            id:         storeButton
                            text:       qsTr("STORE PARAMETERS")
                            onClicked: {
                                mixers.storeSelectedGroup();
                            }
                        }

                    } // Row

                    Row {
                        id: editParamRow
                        spacing: ScreenTools.defaultFontPixelWidth * 2

                        QGCLabel {
                            id:     editParamNameLabel
                            width:  ScreenTools.defaultFontPixelWidth  * 20
                            text:   _editorParameterValue.name
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            clip:   true
                            color:  "white"
                        }

                        QGCLabel {
                            id:     editParamValueLabel
                            width:  ScreenTools.defaultFontPixelWidth  * 10
                            text:   _editorParameterValue.valueString
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            clip:   true
                            color:  "white"
                        }

//                        QGCTextField {
//                            id:                 paramEditField
//                            text:               _editorParameterValue.valueString
////                                   Layout.fillWidth:   true
//                            width:               ScreenTools.defaultFontPixelWidth  * 20
//                        }
                    } //Row

                    Rectangle {
                        id: editParamFillerRectange
                        Layout.fillWidth:   true
                        anchors.top:        editParamRow.bottom
                        anchors.left:       editParamRow.left
                        anchors.right:      editParamRow.right
                        height:             ScreenTools.defaultFontPixelHeight * 2
                        color:              qgcPal.windowShade
                   }

                   QGCListView {
                        id:                 paramValueListView
                        anchors.top:        editParamFillerRectange.bottom
                        anchors.bottom:     parent.bottom
                        width:              ScreenTools.defaultFontPixelWidth  * 32
                        orientation:        ListView.Vertical
                        model:              _editorParameter.values
                        cacheBuffer:        height > 0 ? height * 2 : 0
                        clip:               true
                        focus:              true

                        delegate: Rectangle {
                            id: paramValueDelegate
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            height:         30
                            color:          "black"
                            border.color:   "dark grey"

                            property Fact valueFact: object

                            Row {
                                id:     paramValueRow
                                spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                                anchors.verticalCenter: parent.verticalCenter

                                QGCLabel {
                                    id:     paramValueIDLabel
                                    width:  ScreenTools.defaultFontPixelWidth  * 20
                                    text:   paramValueDelegate.valueFact.name
                                    horizontalAlignment:    Text.AlignHCenter
                                    verticalAlignment:      Text.AlignVCenter
                                    clip:   true
                                    color:  "white"
                                }

                                QGCLabel {
                                    id:     paramValueLabel2
                                    width:  ScreenTools.defaultFontPixelWidth  * 12
                                    text:   paramValueDelegate.valueFact.valueString
                                    horizontalAlignment:    Text.AlignHCenter
                                    verticalAlignment:      Text.AlignVCenter
                                    clip:   true
                                    color:  "white"
                                }
                            } //Row

                            MouseArea {
                                anchors.fill:       parent
                                acceptedButtons:    Qt.LeftButton
                                onClicked: {
                                    if( _editorParameter.readOnly == false ) {
                                        _editorParameterValue = paramValueDelegate.valueFact
                                        showDialog(editorDialogComponent, qsTr("Mixer Parameter Value Editor"), qgcView.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Save)
                                    }
                                }
                            }

                            Component.onCompleted: {
                                editParamRow.update()
                            }
                        } //Rectangle
                    } //ListView
                } //ColumnLayout
            } //RowLayout
        } // Item
    } // Component

    Component {
        id: editorDialogComponent

        ParameterEditorDialog {
            fact:           _editorParameterValue
//            showRCToParam:  _showRCToParam
        }
    }

} // SetupView


