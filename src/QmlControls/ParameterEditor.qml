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

import QtQuick                  2.5
import QtQuick.Controls         1.3
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
    property bool   _searchFilter:      false   ///< true: showing results of search
    property var    _searchResults              ///< List of parameter names from search results
    property string _currentGroup:      ""
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
            height:         searchText.height + ScreenTools.defaultFontPixelHeight / 3
            spacing:        ScreenTools.defaultFontPixelWidth

            QGCTextField {
                id: searchText
            }

            QGCButton {
                anchors.top:    searchText.top
                anchors.bottom: searchText.bottom
                text:           qsTr("Search")
                onClicked: {
                    _searchResults = controller.searchParametersForComponent(-1, searchText.text)
                    _searchFilter = true
                }
            }

            QGCButton {
                anchors.top:    searchText.top
                anchors.bottom: searchText.bottom
                text:           qsTr("Clear")
                visible:        _searchFilter

                onClicked: {
                    searchText.text = ""
                    _searchFilter = false
                    hideDialog()
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
                        if (ScreenTools.isMobile) {
                            qgcView.showDialog(mobileFilePicker, qsTr("Select Parameter File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            controller.loadFromFilePicker()
                        }
                    }
                }
                MenuItem {
                    text:           qsTr("Save to file...")
                    onTriggered: {
                        if (ScreenTools.isMobile) {
                            qgcView.showDialog(mobileFileSaver, qsTr("Save Parameter File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
                        } else {
                            controller.saveToFilePicker()
                        }
                    }
                }
                MenuSeparator { visible: _showRCToParam }
                MenuItem {
                    text:           qsTr("Clear RC to Param")
                    onTriggered:	controller.clearRCToParam()
                    visible:        _showRCToParam
                }
            }
        }

        //---------------------------------------------
        //-- Contents
        Loader {
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        header.bottom
            anchors.bottom:     parent.bottom
            sourceComponent:    _searchFilter ? searchResultsViewComponent: groupedViewComponent
        }
    }

    //-- Parameter Groups
    Component {
        id: groupedViewComponent
        Row {
            spacing: ScreenTools.defaultFontPixelWidth * 0.5
            //-- Parameter Groups
            QGCFlickable {
                id :                groupScroll
                width:              ScreenTools.defaultFontPixelWidth * 25
                height:             parent.height
                clip:               true
                pixelAligned:       true
                contentHeight:      groupedViewComponentColumn.height
                contentWidth:       groupedViewComponentColumn.width
                flickableDirection: Flickable.VerticalFlick
                Column {
                    id: groupedViewComponentColumn
                    spacing: Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)
                    Repeater {
                        model: controller.componentIds
                        Column {
                            id: componentColumn
                            readonly property int componentId: parseInt(modelData)
                            spacing: Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)
                            QGCLabel {
                                text: qsTr("Component #: %1").arg(componentId.toString())
                                font.family: ScreenTools.demiboldFontFamily
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            ExclusiveGroup { id: groupGroup }
                            Repeater {
                                model: controller.getGroupsForComponent(componentId)
                                QGCButton {
                                    width:  ScreenTools.defaultFontPixelWidth * 25
                                    text:	modelData
                                    height: _rowHeight
                                    exclusiveGroup: setupButtonGroup
                                    onClicked: {
                                        checked = true
                                        // Clear the rows from the component first. This allows us to change the componentId without
                                        // breaking any bindings.
                                        factRowsLoader.parameterNames   = [ ]
                                        _rowWidth                       = 10
                                        factRowsLoader.componentId      = componentId
                                        factRowsLoader.parameterNames   = controller.getParametersForGroup(componentId, modelData)
                                        _currentGroup                   = modelData
                                        factScrollView.contentX         = 0
                                        factScrollView.contentY         = 0
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Rectangle {
                color:      __qgcPal.text
                width:      1
                height:     parent.height
                opacity:    0.1
            }
            //-- Parameters
            QGCFlickable {
                id:             factScrollView
                width:          parent.width - groupScroll.width
                height:         parent.height
                contentHeight:  factRowsLoader.height
                contentWidth:   _rowWidth
                boundsBehavior: Flickable.OvershootBounds
                pixelAligned:   true
                clip:           true
                Loader {
                    id:                 factRowsLoader
                    sourceComponent:    factRowsComponent
                    property int    componentId:    controller.componentIds[0]
                    property var    parameterNames: controller.getParametersForGroup(componentId, controller.getGroupsForComponent(componentId)[0])
                    onLoaded: {
                        _currentGroup = controller.getGroupsForComponent(controller.componentIds[0])[0]
                    }
                }
            }
        }
    }

    //---------------------------------------------
    // Search result view
    Component {
        id: searchResultsViewComponent
        Item {
            QGCFlickable {
                id:             factScrollView
                width:          parent.width
                height:         parent.height
                contentHeight:  factRowsLoader.height
                contentWidth:   _rowWidth
                boundsBehavior: Flickable.OvershootBounds
                pixelAligned:   true
                clip:           true
                Loader {
                    id:                 factRowsLoader
                    sourceComponent:    factRowsComponent
                    property int    componentId:       -1
                    property var    parameterNames:    _searchResults
                }
            }
        }
    }

    //---------------------------------------------
    // Paremeters view
    Component {
        id: factRowsComponent
        Column {
            spacing: Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)
            Repeater {
                model: parameterNames
                Rectangle {
                    height: _rowHeight
                    width:  _rowWidth
                    color:  Qt.rgba(0,0,0,0)
                    Row {
                        id:     factRow
                        property Fact modelFact: controller.getParameterFact(componentId, modelData)
                        spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                        anchors.verticalCenter: parent.verticalCenter
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
        id: searchDialogComponent

        QGCViewDialog {

            function accept() {
                _searchResults = controller.searchParametersForComponent(-1, searchFor.text, true /*searchInName.checked*/, true /*searchInDescriptions.checked*/)
                _searchFilter = true
                hideDialog()
            }

            function reject() {
                _searchFilter = false
                hideDialog()
            }

            QGCLabel {
                id:     searchForLabel
                text:   qsTr("Search for:")
            }

            QGCTextField {
                id:                 searchFor
                anchors.topMargin:  defaultTextHeight / 3
                anchors.top:        searchForLabel.bottom
                width:              ScreenTools.defaultFontPixelWidth * 20
            }

            QGCLabel {
                anchors.topMargin:  defaultTextHeight
                anchors.top:        searchFor.bottom
                width:              parent.width
                wrapMode:           Text.WordWrap
                text:               qsTr("Hint: Leave 'Search For' blank and click Apply to list all parameters sorted by name.")
            }
        }
    }

    Component {
        id: mobileFilePicker

        QGCMobileFileDialog {
            fileExtension:      QGroundControl.parameterFileExtension
            onFilenameReturned: controller.loadFromFile(filename)
        }
    }

    Component {
        id: mobileFileSaver

        QGCMobileFileDialog {
            openDialog:         false
            fileExtension:      QGroundControl.parameterFileExtension
            onFilenameReturned: controller.saveToFile(filename)
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
} // QGCView
