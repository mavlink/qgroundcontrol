/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtQuick.Dialogs              1.2
import QtQuick.Layouts              1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    id:         _root

    property Fact   _editorDialogFact: Fact { }
    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10 // Dynamic adjusted at runtime
    property bool   _searchFilter:      searchText.text.trim() != ""   ///< true: showing results of search
    property var    _searchResults      ///< List of parameter names from search results
    property bool   _showRCToParam:     !ScreenTools.isMobile && QGroundControl.multiVehicleManager.activeVehicle.px4Firmware
    property var    _appSettings:       QGroundControl.settingsManager.appSettings

    ParameterEditorController {
        id:                 controller
        onShowErrorMessage: mainWindow.showMessageDialog(qsTr("Parameter Load Errors"), errorMsg)
    }

    ExclusiveGroup { id: sectionGroup }

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
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Search:")
        }

        QGCTextField {
            id:                 searchText
            text:               controller.searchText
            onDisplayTextChanged: controller.searchText = displayText
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCButton {
            text: qsTr("Clear")
            onClicked: {
                if(ScreenTools.isMobile) {
                    Qt.inputMethod.hide();
                }
                clearTimer.start()
            }
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCCheckBox {
            text:       qsTr("Show modified only")
            checked:    controller.showModifiedOnly
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                controller.showModifiedOnly = !controller.showModifiedOnly
            }
        }
    } // Row - Header

    QGCButton {
        anchors.top:    header.top
        anchors.bottom: header.bottom
        anchors.right:  parent.right
        text:           qsTr("Tools")
        visible:        !_searchFilter
        onClicked:      toolsMenu.popup()
    }

    QGCMenu {
        id:                 toolsMenu
        QGCMenuItem {
            text:           qsTr("Refresh")
            onTriggered:	controller.refresh()
        }
        QGCMenuItem {
            text:           qsTr("Reset all to firmware's defaults")
            onTriggered:    mainWindow.showComponentDialog(resetToDefaultConfirmComponent, qsTr("Reset All"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Reset)
        }
        QGCMenuItem {
            text:           qsTr("Reset to vehicle's configuration defaults")
            visible:        !activeVehicle.apmFirmware
            onTriggered:    mainWindow.showComponentDialog(resetToVehicleConfigurationConfirmComponent, qsTr("Reset All"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Reset)
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Load from file...")
            onTriggered: {
                fileDialog.title =          qsTr("Load Parameters")
                fileDialog.selectExisting = true
                fileDialog.openForLoad()
            }
        }
        QGCMenuItem {
            text:           qsTr("Save to file...")
            onTriggered: {
                fileDialog.title =          qsTr("Save Parameters")
                fileDialog.selectExisting = false
                fileDialog.openForSave()
            }
        }
        QGCMenuSeparator { visible: _showRCToParam }
        QGCMenuItem {
            text:           qsTr("Clear RC to Param")
            onTriggered:	controller.clearRCToParam()
            visible:        _showRCToParam
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Reboot Vehicle")
            onTriggered:    mainWindow.showComponentDialog(rebootVehicleConfirmComponent, qsTr("Reboot Vehicle"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
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
        contentHeight:      groupedViewCategoryColumn.height
        flickableDirection: Flickable.VerticalFlick
        visible:            !_searchFilter && !controller.showModifiedOnly

        ColumnLayout {
            id:             groupedViewCategoryColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

            Repeater {
                model: controller.categories

                Column {
                    Layout.fillWidth:   true
                    spacing:            Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

                    readonly property string category: modelData

                    SectionHeader {
                        id:             categoryHeader
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           category
                        checked:        controller.currentCategory === text
                        exclusiveGroup: sectionGroup

                        onCheckedChanged: {
                            if (checked) {
                                controller.currentCategory  = category
                                controller.currentGroup     = controller.getGroupsForCategory(category)[0]
                            }
                        }
                    }

                    ExclusiveGroup { id: buttonGroup }

                    Repeater {
                        model: categoryHeader.checked ? controller.getGroupsForCategory(category) : 0

                        QGCButton {
                            width:          ScreenTools.defaultFontPixelWidth * 25
                            text:           groupName
                            height:         _rowHeight
                            checked:        controller.currentGroup === text
                            exclusiveGroup: buttonGroup

                            readonly property string groupName: modelData

                            onClicked: {
                                if (!checked) _rowWidth = 10
                                checked = true
                                controller.currentCategory  = category
                                controller.currentGroup     = groupName
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
        anchors.left:       (_searchFilter || controller.showModifiedOnly) ? parent.left : groupScroll.right
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
                    color:  factRow.modelFact.defaultValueAvailable ? (factRow.modelFact.valueEqualsDefault ? qgcPal.text : qgcPal.warningText) : qgcPal.text
                    text:   factRow.modelFact.enumStrings.length === 0 ? factRow.modelFact.valueString + " " + factRow.modelFact.units : factRow.modelFact.enumStringValue
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
                color:  qgcPal.text
                opacity: 0.15
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
            }

            MouseArea {
                anchors.fill:       parent
                acceptedButtons:    Qt.LeftButton
                onClicked: {
                    _editorDialogFact = factRow.modelFact
                    mainWindow.showComponentDialog(editorDialogComponent, qsTr("Parameter Editor"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Save)
                }
            }
        }
    }

    QGCFileDialog {
        id:             fileDialog
        folder:         _appSettings.parameterSavePath
        fileExtension:  _appSettings.parameterFileExtension
        nameFilters:    [ qsTr("Parameter Files (*.%1)").arg(_appSettings.parameterFileExtension) , qsTr("All Files (*.*)") ]

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
                text:               qsTr("Select Reset to reset all parameters to their defaults.\n\nNote that this will also completely reset everything, including UAVCAN nodes.")
            }
        }
    }

    Component {
        id: resetToVehicleConfigurationConfirmComponent
        QGCViewDialog {
            function accept() {
                controller.resetAllToVehicleConfiguration()
                hideDialog()
            }
            QGCLabel {
                width:              parent.width
                wrapMode:           Text.WordWrap
                text:               qsTr("Select Reset to reset all parameters to the vehicle's configuration defaults.")
            }
        }
    }

    Component {
        id: rebootVehicleConfirmComponent

        QGCViewDialog {
            function accept() {
                activeVehicle.rebootVehicle()
                hideDialog()
            }

            QGCLabel {
                width:              parent.width
                wrapMode:           Text.WordWrap
                text:               qsTr("Select Ok to reboot vehicle.")
            }
        }
    }
}
