/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

Item {
    property Component viewComponent

    function __showDialog(component, title, charWidth, buttons) {
        __acceptButton.visible = false
        __rejectButton.visible = false
        __dialogCharWidth = charWidth
        __dialogTitle = title

        if (buttons & StandardButton.Ok) {
            __acceptButton.text = "Ok"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            __acceptButton.text = "Open"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Save) {
            __acceptButton.text = "Save"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Cancel) {
            __rejectButton.text = "Cancel"
            __rejectButton.visible = true
        } else if (buttons & StandardButton.Close) {
            __rejectButton.text = "Cancel"
            __rejectButton.visible = true
        } else if (buttons & StandardButton.Apply) {
            __acceptButton.text = "Apply"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            __acceptButton.text = "Open"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.SaveAll) {
            __acceptButton.text = "Save All"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Yes) {
            __acceptButton.text = "Yes"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.YesToAll) {
            __acceptButton.text = "Yes to All"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.No) {
            __rejectButton.text = "No"
            __rejectButton.visible = true
        } else if (buttons & StandardButton.NoToAll) {
            __rejectButton.text = "No to All"
            __rejectButton.visible = true
        } else if (buttons & StandardButton.Abort) {
            __rejectButton.text = "Abort"
            __rejectButton.visible = true
        } else if (buttons & StandardButton.Retry) {
            __acceptButton.text = "Retry"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Reset) {
            __acceptButton.text = "Reset"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.RestoreToDefaults) {
            __acceptButton.text = "Restore to Defaults"
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Ignore) {
            __acceptButton.text = "Ignore"
            __acceptButton.visible = true
        }
        __dialogComponent = component
        __viewPanel.enabled = false
        __dialogOverlay.visible = true
    }

    function __hideDialog() {
        __dialogComponent = null
        __viewPanel.enabled = true
        __dialogOverlay.visible = false
    }

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }
    QGCLabel { id: __textMeasure; text: "X"; visible: false }

    property real __textHeight: __textMeasure.contentHeight
    property real __textWidth:  __textMeasure.contentWidth

    /// The width of the dialog panel in characters
    property int __dialogCharWidth: 75

    /// The title for the dialog panel
    property string __dialogTitle

    property Component __dialogComponent

    Component.onCompleted: __viewPanel.item.doWorkAfterComponentCompleted()

    Connections {
        target: __viewPanel.item

        onShowDialog: __showDialog(component, title, charWidth, buttons)
        onHideDialog: __hideDialog()
    }

    Connections {
        target: __dialogComponentLoader.item

        onHideDialog: __hideDialog()
    }

    Loader {
        id:                 __viewPanel
        anchors.fill:       parent
        focus:              true
        sourceComponent:    viewComponent
    }

    Item {
        id:             __dialogOverlay
        visible:        false
        anchors.fill:   parent
        z:              5000

        // This covers the parent with an transparent section
        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            anchors.left:   parent.left
            anchors.right:  __dialogPanel.left
            opacity:        0.80
            color:          __qgcPal.window
        }

        // This is the main dialog panel which is anchored to the right edge
        Rectangle {
            id:             __dialogPanel
            width:          __textWidth * __dialogCharWidth
            height:         parent.height
            anchors.right:  parent.right
            color:          __qgcPal.windowShadeDark

            Rectangle {
                id:     __header
                width:  parent.width
                height: __acceptButton.height
                color:  __qgcPal.windowShade

                function __hidePanel() {
                    __fullPanel.visible = false
                }

                QGCLabel {
                    x:                  __textWidth
                    height:             parent.height
                    verticalAlignment:	Text.AlignVCenter
                    text:               __dialogTitle
                }

                QGCButton {
                    id:             __rejectButton
                    anchors.right:  __acceptButton.left
                    anchors.bottom: parent.bottom

                    onClicked: __dialogComponentLoader.item.reject()
                }

                QGCButton {
                    id:             __acceptButton
                    anchors.right:  parent.right
                    primary:        true

                    onClicked: __dialogComponentLoader.item.accept()
                }
            }

            Item {
                id:             __spacer
                width:          10
                height:         10
                anchors.top:    __header.bottom
            }

            Loader {
                id:                 __dialogComponentLoader
                anchors.margins:    5
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        __spacer.bottom
                anchors.bottom:     parent.bottom
                sourceComponent:    __dialogComponent
            }
        } // Rectangle - Dialog panel
    } // Item - Dialog overlay
}