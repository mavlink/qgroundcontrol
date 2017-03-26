/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Dialogs          1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }

    property Fact   _editorDialogFact: Fact { }
    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10      // Dynamic adjusted at runtime
    property bool   _searchFilter:      searchText.text != ""   ///< true: showing results of search
    property var    _searchResults              ///< List of parameter names from search results
    property bool   _showRCToParam:     !ScreenTools.isMobile && QGroundControl.multiVehicleManager.activeVehicle.px4Firmware

    ParameterEditorController {
        id:         controller;
        factPanel:  panel
        onShowErrorMessage: {
            showMessage(qsTr("Parameter Load Errors"), errorMsg, StandardButton.Ok)
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        //---------------------------------------------
        //-- Header
        Row {
            id:             header
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        ScreenTools.defaultFontPixelWidth

            Timer {
                id:         clearTimer
                interval:   100;
                running:    false;
                repeat:     false
                onTriggered: {
                    searchText.text = ""
                    controller.searchText = ""
                }
            }

            QGCLabel {
                anchors.baseline:   clearButton.baseline
                text:               qsTr("Search:")
            }

            QGCTextField {
                id:                 searchText
                anchors.baseline:   clearButton.baseline
                text:               controller.searchText
                onDisplayTextChanged: controller.searchText = displayText
            }

            QGCButton {
                id:         clearButton
                text:       qsTr("Clear")
                onClicked: {
                    if(ScreenTools.isMobile) {
                        Qt.inputMethod.hide();
                    }
                    clearTimer.start()
                }
            }
        } // Row - Header

        QGCButton {
            anchors.top:    header.top
            anchors.bottom: header.bottom
            anchors.right:  parent.right
            text:           qsTr("Tools")
            visible:        !_searchFilter

            menu: Menu {
                MenuItem {
                    text:           qsTr("Refresh")
                    onTriggered:	controller.refresh()
                }
                MenuItem {
                    text:           qsTr("Reset all to defaults")
                    onTriggered:    showDialog(resetToDefaultConfirmComponent, qsTr("Reset All"), qgcView.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Reset)
                }
                MenuSeparator { }
                MenuItem {
                    text:           qsTr("Load from file...")
                    onTriggered: {
                        var appSettings = QGroundControl.settingsManager.appSettings

                        fileDialog.qgcView =        qgcView
                        fileDialog.title =          qsTr("Select Parameter File")
                        fileDialog.selectExisting = true
                        fileDialog.folder =         appSettings.parameterSavePath
                        fileDialog.fileExtension =  appSettings.parameterFileExtension
                        fileDialog.nameFilters =    [ qsTr("Parameter Files (*.%1)").arg(appSettings.parameterFileExtension) , qsTr("All Files (*.*)") ]
                        fileDialog.openForLoad()
                    }
                }
                MenuItem {
                    text:           qsTr("Save to file...")
                    onTriggered: {
                        var appSettings = QGroundControl.settingsManager.appSettings

                        fileDialog.qgcView =        qgcView
                        fileDialog.title =          qsTr("Save Parameters")
                        fileDialog.selectExisting = false
                        fileDialog.folder =         appSettings.parameterSavePath
                        fileDialog.fileExtension =  appSettings.parameterFileExtension
                        fileDialog.nameFilters =    [ qsTr("Parameter Files (*.%1)").arg(appSettings.parameterFileExtension) , qsTr("All Files (*.*)") ]
                        fileDialog.openForSave()
                    }
                }
                MenuSeparator { visible: _showRCToParam }
                MenuItem {
                    text:           qsTr("Clear RC to Param")
                    onTriggered:	controller.clearRCToParam()
                    visible:        _showRCToParam
                }
                MenuSeparator { }
                MenuItem {
                    text:           qsTr("Reboot Vehicle")
                    onTriggered:    showDialog(rebootVehicleConfirmComponent, qsTr("Reboot Vehicle"), qgcView.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                }
            }
        }

        /// Group buttons
        QGCFlickable {
            id :                groupScroll
            width:              ScreenTools.defaultFontPixelWidth * 25
            anchors.top:        header.bottom
            anchors.bottom:     parent.bottom
            clip:               true
            pixelAligned:       true
            contentHeight:      groupedViewComponentColumn.height
            contentWidth:       groupedViewComponentColumn.width
            flickableDirection: Flickable.VerticalFlick
            visible:            !_searchFilter

            Column {
                id:         groupedViewComponentColumn
                spacing:    Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

                Repeater {
                    model: controller.componentIds

                    Column {
                        id:     componentColumn
                        spacing: Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

                        readonly property int componentId: modelData

                        QGCLabel {
                            text: qsTr("Component #: %1").arg(componentId.toString())
                            font.family: ScreenTools.demiboldFontFamily
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        ExclusiveGroup { id: groupGroup }

                        Repeater {
                            model: controller.getGroupsForComponent(componentId)

                            QGCButton {
                                width:          ScreenTools.defaultFontPixelWidth * 25
                                text:           groupName
                                height:         _rowHeight
                                exclusiveGroup: setupButtonGroup

                                readonly property string groupName: modelData

                                onClicked: {
                                    checked = true
                                    _rowWidth                       = 10
                                    controller.currentComponentId   = componentId
                                    controller.currentGroup         = groupName
                                }
                            }
                        }
                    }
                }
            }
        }

        /// Parameter list
        QGCListView {
            id:                 editorListView
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
            anchors.left:       _searchFilter ? parent.left : groupScroll.right
            anchors.right:      parent.right
            anchors.top:        header.bottom
            anchors.bottom:     parent.bottom
            orientation:        ListView.Vertical
            model:              controller.parameters
            cacheBuffer:        height > 0 ? height * 2 : 0
            clip:               true

            delegate: Rectangle {
                height: _rowHeight
                width:  _rowWidth
                color:  Qt.rgba(0,0,0,0)

                Row {
                    id:     factRow
                    spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                    anchors.verticalCenter: parent.verticalCenter

                    property Fact modelFact: object

                    QGCLabel {
                        id:     nameLabel
                        width:  ScreenTools.defaultFontPixelWidth  * 20
                        text:   factRow.modelFact.name
                        clip:   true
                    }

                    QGCLabel {
                        id:     valueLabel
                        width:  ScreenTools.defaultFontPixelWidth  * 20
                        color:  factRow.modelFact.defaultValueAvailable ? (factRow.modelFact.valueEqualsDefault ? __qgcPal.text : __qgcPal.warningText) : __qgcPal.text
                        text:   factRow.modelFact.enumStrings.length == 0 ? factRow.modelFact.valueString + " " + factRow.modelFact.units : factRow.modelFact.enumStringValue
                        clip:   true
                    }

                    QGCLabel {
                        text:   factRow.modelFact.shortDescription
                    }

                    Component.onCompleted: {
                        if(_rowWidth < factRow.width + ScreenTools.defaultFontPixelWidth) {
                            _rowWidth = factRow.width + ScreenTools.defaultFontPixelWidth
                        }
                    }
                }

                Rectangle {
                    width:  _rowWidth
                    height: 1
                    color:  __qgcPal.text
                    opacity: 0.15
                    anchors.bottom: parent.bottom
                    anchors.left:   parent.left
                }

                MouseArea {
                    anchors.fill:       parent
                    acceptedButtons:    Qt.LeftButton
                    onClicked: {
                        _editorDialogFact = factRow.modelFact
                        showDialog(editorDialogComponent, qsTr("Parameter Editor"), qgcView.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Save)
                    }
                }
            }
        }
    } // QGCViewPanel

    QGCFileDialog {
        id: fileDialog

        onAcceptedForSave: {
            controller.saveToFile(file)
            close()
        }

        onAcceptedForLoad: {
            controller.loadFromFile(file)
            close()
        }
    }

    Component {
        id: editorDialogComponent

        ParameterEditorDialog {
            fact:           _editorDialogFact
            showRCToParam:  _showRCToParam
        }
    }

    Component {
        id: resetToDefaultConfirmComponent

        QGCViewDialog {
            function accept() {
                controller.resetAllToDefaults()
                hideDialog()
            }

            QGCLabel {
                width:              parent.width
                wrapMode:           Text.WordWrap
                text:               qsTr("Select Reset to reset all parameters to their defaults.")
            }
        }
    }

    Component {
        id: rebootVehicleConfirmComponent

        QGCViewDialog {
            function accept() {
                QGroundControl.multiVehicleManager.activeVehicle.rebootVehicle()
                hideDialog()
            }

            QGCLabel {
                width:              parent.width
                wrapMode:           Text.WordWrap
                text:               qsTr("Select Ok to reboot vehicle.")
            }
        }
    }
} // QGCView
