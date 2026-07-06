/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FactControls

Item {
    id:         _root

    property Fact   _editorDialogFact: Fact { }
    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10 // Dynamic adjusted at runtime
    property bool   _searchFilter:      searchText.text.trim() != "" || controller.showModifiedOnly  ///< true: showing results of search
    property var    _searchResults      ///< List of parameter names from search results
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showRCToParam:     _activeVehicle.px4Firmware
    property var    _appSettings:       QGroundControl.settingsManager.appSettings
    property var    _controller:        controller
    property real   _panelRadius:       Math.round(ScreenTools.defaultFontPixelWidth * 0.75)
    property real   _panelPadding:      Math.round(ScreenTools.defaultFontPixelHeight * 0.55)
    property color  _panelColor:        Qt.rgba(1, 1, 1, 0.026)
    property color  _rowColor:          Qt.rgba(1, 1, 1, 0.030)
    property color  _borderColor:       Qt.rgba(0.82, 0.88, 0.94, 0.12)

    ParameterEditorController {
        id: controller
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: _root.enabled }

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

    QGCMenu {
        id:                 toolsMenu
        QGCMenuItem {
            text:           qsTr("Refresh")
            onTriggered:	controller.refresh()
        }
        QGCMenuItem {
            text:           qsTr("Reset all to firmware's defaults")
            onTriggered:    mainWindow.showMessageDialog(qsTr("Reset All"),
                                                         qsTr("Select Reset to reset all parameters to their defaults.\n\nNote that this will also completely reset everything, including UAVCAN nodes, all vehicle settings, setup and calibrations."),
                                                         Dialog.Cancel | Dialog.Reset,
                                                         function() { controller.resetAllToDefaults() })
        }
        QGCMenuItem {
            text:           qsTr("Reset to vehicle's configuration defaults")
            visible:        !_activeVehicle.apmFirmware
            onTriggered:    mainWindow.showMessageDialog(qsTr("Reset All"),
                                                         qsTr("Select Reset to reset all parameters to the vehicle's configuration defaults."),
                                                         Dialog.Cancel | Dialog.Reset,
                                                         function() { controller.resetAllToVehicleConfiguration() })
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Load from file for review...")
            onTriggered: {
                fileDialog.title =          qsTr("Load Parameters")
                fileDialog.openForLoad()
            }
        }
        QGCMenuItem {
            text:           qsTr("Save to file...")
            onTriggered: {
                fileDialog.title =          qsTr("Save Parameters")
                fileDialog.openForSave()
            }
        }
        QGCMenuSeparator { visible: _showRCToParam }
        QGCMenuItem {
            text:           qsTr("Clear all RC to Param")
            onTriggered:	_activeVehicle.clearAllParamMapRC()
            visible:        _showRCToParam
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Reboot Vehicle")
            onTriggered:    mainWindow.showMessageDialog(qsTr("Reboot Vehicle"),
                                                         qsTr("Select Ok to reboot vehicle."),
                                                         Dialog.Cancel | Dialog.Ok,
                                                         function() { _activeVehicle.rebootVehicle() })
        }
    }


    QGCFileDialog {
        id:             fileDialog
        folder:         _appSettings.parameterSavePath
        nameFilters:    [ qsTr("Parameter Files (*.%1)").arg(_appSettings.parameterFileExtension) , qsTr("All Files (*)") ]

        onAcceptedForSave: (file) => {
            controller.saveToFile(file)
            close()
        }

        onAcceptedForLoad: (file) => {
            close()
            if (controller.buildDiffFromFile(file)) {
                parameterDiffDialog.createObject(mainWindow).open()
            }
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
        id: parameterDiffDialog

        ParameterDiffDialog {
            paramController: _controller
        }
    }

    Rectangle {
        id:             header
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         headerLayout.implicitHeight + (_panelPadding * 2)
        color:          _panelColor
        radius:         _panelRadius
        border.color:   _borderColor
        border.width:   1

        RowLayout {
            id:             headerLayout
            anchors.fill:   parent
            anchors.margins: _panelPadding
            spacing:        ScreenTools.defaultFontPixelWidth

            QGCTextField {
                id:                     searchText
                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 26
                placeholderText:        qsTr("Search")
                onDisplayTextChanged:   controller.searchText = displayText
            }

            QGCButton {
                text: qsTr("Clear")
                onClicked: {
                    if(ScreenTools.isMobile) {
                        Qt.inputMethod.hide();
                    }
                    clearTimer.start()
                }
            }

            QGCCheckBox {
                text:       qsTr("Show modified only")
                checked:    controller.showModifiedOnly
                onClicked:  controller.showModifiedOnly = checked
                visible:    QGroundControl.multiVehicleManager.activeVehicle.px4Firmware
            }

            Item { Layout.fillWidth: true }

            QGCButton {
                text:       qsTr("Tools")
                iconSource: "/res/gear-white.svg"
                onClicked:  toolsMenu.popup()
            }
        }
    }

    /// Group buttons
    Rectangle {
        id:                 groupPanel
        width:              ScreenTools.defaultFontPixelWidth * 27
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.5
        anchors.top:        header.bottom
        anchors.bottom:     parent.bottom
        color:              _panelColor
        radius:             _panelRadius
        border.color:       _borderColor
        border.width:       1
        clip:               true
        visible:            !_searchFilter

        QGCFlickable {
            id :                groupScroll
            anchors.fill:       parent
            anchors.margins:    _panelPadding
            clip:               true
            pixelAligned:       true
            contentHeight:      groupedViewCategoryColumn.height
            flickableDirection: Flickable.VerticalFlick

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


                        SectionHeader {
                            id:             categoryHeader
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           object.name
                            checked:        object == controller.currentCategory

                            onCheckedChanged: {
                                if (checked) {
                                    controller.currentCategory  = object
                                }
                            }
                        }

                        Repeater {
                            model: categoryHeader.checked ? object.groups : 0

                            QGCButton {
                                width:          groupScroll.width
                                text:           object.name
                                height:         _rowHeight
                                checked:        object == controller.currentGroup
                                autoExclusive:  true
                                showBorder:     false
                                backRadius:     Math.round(ScreenTools.defaultFontPixelWidth * 0.55)
                                backgroundColor: checked ? Qt.rgba(1, 1, 1, 0.075) : Qt.rgba(1, 1, 1, 0.020)

                                onClicked: {
                                    if (!checked) _rowWidth = 10
                                    checked = true
                                    controller.currentGroup = object
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id:                 tablePanel
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.5
        anchors.top:        header.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       _searchFilter ? parent.left : groupPanel.right
        anchors.right:      parent.right
        color:              _panelColor
        radius:             _panelRadius
        border.color:       _borderColor
        border.width:       1
        clip:               true

        TableView {
            id:                 tableView
            anchors.fill:       parent
            anchors.margins:    _panelPadding
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            rowSpacing:         ScreenTools.defaultFontPixelHeight / 4
            model:              controller.parameters
            contentWidth:       width
            clip:               true

            // Qt is supposed to adjust column widths automatically when larger widths come into view.
            // But it doesn't work. So we have to do it force a layout manually when we scroll.
            Timer {
                id:             forceLayoutTimer
                interval:       500
                repeat:         false
                onTriggered:    tableView.forceLayout()
            }

            onTopRowChanged: forceLayoutTimer.start()
            onModelChanged: {
                positionViewAtRow(0, TableView.AlignLeft | TableView.AlignTop)
                forceLayoutTimer.start()
            }

            delegate: Item {
                implicitWidth:  column == 1 ? ScreenTools.defaultFontPixelWidth * 15 : label.contentWidth + ScreenTools.defaultFontPixelWidth
                implicitHeight: _rowHeight
                clip:           true

                Rectangle {
                    anchors.fill:   parent
                    color:          _rowColor
                    radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.35)
                }

                QGCLabel {
                    id:                     label
                    anchors.left:           parent.left
                    anchors.right:          parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.rightMargin:    anchors.leftMargin
                    text:                   column == 1 ? col1String() : display
                    color:                  column == 1 ? col1Color() : qgcPal.text
                    maximumLineCount:       1
                    elide:                  column == 1 ? Text.ElideRight : Text.ElideNone

                    Component.onCompleted: {
                        return
                        if (tableView.columnWidth(column) < width) {
                            console.log("setColumnWidth", column, width)
                            tableView.setColumnWidth(column, width)
                        }
                    }

                    function col1String() {
                        if (fact.enumStrings.length === 0) {
                            return fact.valueString + " " + fact.units
                        }
                        if (fact.bitmaskStrings.length != 0) {
                            return fact.selectedBitmaskStrings.join(',')
                        }
                        return fact.enumStringValue
                    }

                    function col1Color() {
                        if (fact.defaultValueAvailable) {
                            return fact.valueEqualsDefault ? qgcPal.text : qgcPal.warningText
                        } else {
                            return qgcPal.text
                        }
                    }
                }

                QGCMouseArea {
                    anchors.fill: parent
                    onClicked: mouse => {
                        _editorDialogFact = fact
                        editorDialogComponent.createObject(mainWindow).open()
                    }
                }
            }
        }
    }
}
