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

import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

FactPanel {
    id: __rootItem

    property var qgcView:               __rootItem  ///< Used by Fact controls for validation dialogs
    property bool completedSignalled:   false
    property real topDialogMargin:      0           ///< Set a top margin for dialog

    property var viewPanel

    /// This is signalled when the top level Item reaches Component.onCompleted. This allows
    /// the view subcomponent to connect to this signal and do work once the full ui is ready
    /// to go.
    signal completed

    function __setupDialogButtons(buttons) {
        __acceptButton.visible = false
        __rejectButton.visible = false

        // Accept role buttons
        if (buttons & StandardButton.Ok) {
            __acceptButton.text = qsTr("Ok")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            __acceptButton.text = qsTr("Open")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Save) {
            __acceptButton.text = qsTr("Save")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Apply) {
            __acceptButton.text = qsTr("Apply")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            __acceptButton.text = qsTr("Open")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.SaveAll) {
            __acceptButton.text = qsTr("Save All")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Yes) {
            __acceptButton.text = qsTr("Yes")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.YesToAll) {
            __acceptButton.text = qsTr("Yes to All")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Retry) {
            __acceptButton.text = qsTr("Retry")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Reset) {
            __acceptButton.text = qsTr("Reset")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.RestoreToDefaults) {
            __acceptButton.text = qsTr("Restore to Defaults")
            __acceptButton.visible = true
        } else if (buttons & StandardButton.Ignore) {
            __acceptButton.text = qsTr("Ignore")
            __acceptButton.visible = true
        }

        // Reject role buttons
        if (buttons & StandardButton.Cancel) {
            __rejectButton.text = qsTr("Cancel")
            __rejectButton.visible = true
        } else if (buttons & StandardButton.Close) {
            __rejectButton.text = qsTr("Close")
            __rejectButton.visible = true
        } else if (buttons & StandardButton.No) {
            __rejectButton.text = qsTr("No")
            __rejectButton.visible = true
        } else if (buttons & StandardButton.NoToAll) {
            __rejectButton.text = qsTr("No to All")
            __rejectButton.visible = true
        } else if (buttons & StandardButton.Abort) {
            __rejectButton.text = qsTr("Abort")
            __rejectButton.visible = true
        }
    }

    function __stopAllAnimations() {
        if (__animateHideDialog.running) {
            __animateHideDialog.stop()
        }
    }

    function __checkForEarlyDialog(title) {
        if (!completedSignalled) {
            console.warn(qsTr("showDialog|Message called before QGCView.completed signalled"), title)
        }
    }

    /// Shows a QGCViewDialog component
    ///     @param compoent QGCViewDialog component
    ///     @param title Title for dialog
    ///     @param charWidth Width of dialog in characters
    ///     @param buttons Buttons to show in dialog using StandardButton enum

    readonly property int showDialogFullWidth:      -1  ///< Use for full width dialog
    readonly property int showDialogDefaultWidth:   40  ///< Use for default dialog width

    function showDialog(component, title, charWidth, buttons) {
        if (__checkForEarlyDialog(title)) {
            return
        }

        __stopAllAnimations()

        __dialogCharWidth = charWidth
        __dialogTitle = title

        __setupDialogButtons(buttons)

        __dialogComponent = component
        viewPanel.enabled = false
        __dialogOverlay.visible = true

        //__dialogComponentLoader.item.forceActiveFocus()

        __animateShowDialog.start()
    }

    function showMessage(title, message, buttons) {
        if (__checkForEarlyDialog(title)) {
            return
        }

        __stopAllAnimations()

        __dialogCharWidth = showDialogDefaultWidth
        __dialogTitle = title
        __messageDialogText = message

        __setupDialogButtons(buttons)

        __dialogComponent = __messageDialog
        viewPanel.enabled = false
        __dialogOverlay.visible = true

        __dialogComponentLoader.item.forceActiveFocus()

        __animateShowDialog.start()
    }

    function hideDialog() {
        //__dialogComponentLoader.item.focus = false
        viewPanel.enabled = true
        __animateHideDialog.start()
    }

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }
    QGCLabel { id: __textMeasure; text: "X"; visible: false }

    property real defaultTextHeight: __textMeasure.contentHeight
    property real defaultTextWidth:  __textMeasure.contentWidth

    /// The width of the dialog panel in characters
    property int __dialogCharWidth: 75

    /// The title for the dialog panel
    property string __dialogTitle

    property string __messageDialogText

    property Component __dialogComponent

    function __signalCompleted() {
        // When we use this control inside a QGCQmlWidgetHolder Component.onCompleted is signalled
        // before the width and height are adjusted. So we need to wait for width and heigth to be
        // set before we signal our own completed signal.
        if (!completedSignalled && width != 0 && height != 0) {
            completedSignalled = true
            completed()
        }
    }

    Component.onCompleted:  __signalCompleted()
    onWidthChanged:         __signalCompleted()
    onHeightChanged:        __signalCompleted()

    Connections {
        target: __dialogComponentLoader.item

        onHideDialog: __rootItem.hideDialog()
    }

    Item {
        id:             __dialogOverlay
        visible:        false
        anchors.fill:   parent
        z:              5000

        readonly property int __animationDuration: 200

        ParallelAnimation {
            id: __animateShowDialog


            NumberAnimation {
                target:     __transparentSection
                properties: "opacity"
                from:       0.0
                to:         0.8
                duration:   __dialogOverlay.__animationDuration
            }

            NumberAnimation {
                target:     __transparentSection
                properties: "width"
                from:       __dialogOverlay.width
                to:         __dialogOverlay.width - __dialogPanel.width
                duration:   __dialogOverlay.__animationDuration
            }
        }

        ParallelAnimation {
            id: __animateHideDialog

            NumberAnimation {
                target:     __transparentSection
                properties: "opacity"
                from:       0.8
                to:         0.0
                duration:   __dialogOverlay.__animationDuration
            }

            NumberAnimation {
                target:     __transparentSection
                properties: "width"
                from:       __dialogOverlay.width - __dialogPanel.width
                to:         __dialogOverlay.width
                duration:   __dialogOverlay.__animationDuration
            }

            onRunningChanged: {
                if (!running) {
                    __dialogComponent = null
                    __dialogOverlay.visible = false
                }
            }
        }

        // This covers the parent with an transparent section
        Rectangle {
            id:             __transparentSection
            height:         ScreenTools.availableHeight ? ScreenTools.availableHeight : parent.height
            anchors.bottom: parent.bottom
            anchors.left:   parent.left
            anchors.right:  __dialogPanel.left
            opacity:        0.0
            color:          __qgcPal.window
        }

        // This is the main dialog panel which is anchored to the right edge
        Rectangle {
            id:                 __dialogPanel
            width:              __dialogCharWidth == showDialogFullWidth ? parent.width : defaultTextWidth * __dialogCharWidth
            anchors.topMargin:  topDialogMargin
            height:             ScreenTools.availableHeight ? ScreenTools.availableHeight : parent.height
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            color:              __qgcPal.windowShadeDark

            Rectangle {
                id:     __header
                width:  parent.width
                height: __acceptButton.height
                color:  __qgcPal.windowShade

                function __hidePanel() {
                    __fullPanel.visible = false
                }

                QGCLabel {
                    x:                  defaultTextWidth
                    height:             parent.height
                    verticalAlignment:	Text.AlignVCenter
                    text:               __dialogTitle
                }

                QGCButton {
                    id:             __rejectButton
                    anchors.right:  __acceptButton.visible ?  __acceptButton.left : parent.right
                    anchors.bottom: parent.bottom

                    onClicked: __dialogComponentLoader.item.reject()
                }

                QGCButton {
                    id:             __acceptButton
                    anchors.right:  parent.right
                    primary:        true

                    onClicked: {
                       __dialogComponentLoader.item.accept()
                    }
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

                property bool acceptAllowed: __acceptButton.visible
                property bool rejectAllowed: __rejectButton.visible
            }
        } // Rectangle - Dialog panel
    } // Item - Dialog overlay

    Component {
        id: __messageDialog

        QGCViewMessage {
            message: __messageDialogText
        }
    }
}
