/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.12
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.12
import QtQuick.Dialogs              1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Popup {
    property var dialogComponent
    property var dialogProperties

    id:                 popupRoot
    anchors.centerIn:   parent
    width:              mainFlickable.width + (padding * 2)
    height:             mainFlickable.height + (padding * 2)
    padding:            2
    modal:              true
    focus:              true

    property var    _pal:                   QGroundControl.globalPalette
    property real   _frameSize:             ScreenTools.defaultFontPixelWidth
    property string _dialogTitle
    property real   _contentMargin:         ScreenTools.defaultFontPixelHeight / 2
    property real   _popupDoubleInset:      ScreenTools.defaultFontPixelHeight * 2
    property real   _maxAvailableWidth:     parent.width - _popupDoubleInset
    property real   _maxAvailableHeight:    parent.height - _popupDoubleInset

    background: Item {
        Rectangle {
            anchors.left:   parent.left
            anchors.top:    parent.top
            width:          _frameSize
            height:         _frameSize
            color:          _pal.text
            visible:        enabled
        }

        Rectangle {
            anchors.right:  parent.right
            anchors.top:    parent.top
            width:          _frameSize
            height:         _frameSize
            color:          _pal.text
            visible:        enabled
        }

        Rectangle {
            anchors.left:   parent.left
            anchors.bottom: parent.bottom
            width:          _frameSize
            height:         _frameSize
            color:          _pal.text
            visible:        enabled
        }

        Rectangle {
            anchors.right:  parent.right
            anchors.bottom: parent.bottom
            width:          _frameSize
            height:         _frameSize
            color:          _pal.text
            visible:        enabled
        }

        Rectangle {
            anchors.margins:    popupRoot.padding
            anchors.fill:       parent
            color:              _pal.window
        }
    }

    Component.onCompleted: {
        _dialogTitle = dialogComponentLoader.item.title
        setupDialogButtons(dialogComponentLoader.item.buttons)
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: parent.enabled }

    function setupDialogButtons(buttons) {
        acceptButton.visible = false
        rejectButton.visible = false
        // Accept role buttons
        if (buttons & StandardButton.Ok) {
            acceptButton.text = qsTr("Ok")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            acceptButton.text = qsTr("Open")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Save) {
            acceptButton.text = qsTr("Save")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Apply) {
            acceptButton.text = qsTr("Apply")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            acceptButton.text = qsTr("Open")
            acceptButton.visible = true
        } else if (buttons & StandardButton.SaveAll) {
            acceptButton.text = qsTr("Save All")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Yes) {
            acceptButton.text = qsTr("Yes")
            acceptButton.visible = true
        } else if (buttons & StandardButton.YesToAll) {
            acceptButton.text = qsTr("Yes to All")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Retry) {
            acceptButton.text = qsTr("Retry")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Reset) {
            acceptButton.text = qsTr("Reset")
            acceptButton.visible = true
        } else if (buttons & StandardButton.RestoreToDefaults) {
            acceptButton.text = qsTr("Restore to Defaults")
            acceptButton.visible = true
        } else if (buttons & StandardButton.Ignore) {
            acceptButton.text = qsTr("Ignore")
            acceptButton.visible = true
        }

        // Reject role buttons
        if (buttons & StandardButton.Cancel) {
            rejectButton.text = qsTr("Cancel")
            rejectButton.visible = true
        } else if (buttons & StandardButton.Close) {
            rejectButton.text = qsTr("Close")
            rejectButton.visible = true
        } else if (buttons & StandardButton.No) {
            rejectButton.text = qsTr("No")
            rejectButton.visible = true
        } else if (buttons & StandardButton.NoToAll) {
            rejectButton.text = qsTr("No to All")
            rejectButton.visible = true
        } else if (buttons & StandardButton.Abort) {
            rejectButton.text = qsTr("Abort")
            rejectButton.visible = true
        }

        if (rejectButton.visible) {
            closePolicy = Popup.NoAutoClose | Popup.CloseOnEscape
        } else {
            closePolicy = Popup.NoAutoClose
        }
    }

    Connections {
        target:         dialogComponentLoader.item
        onHideDialog:   close()
    }

    QGCFlickable {
        id:             mainFlickable
        width:          Math.min(mainColumnLayout.width, _maxAvailableWidth)
        height:         Math.min(mainColumnLayout.height, _maxAvailableHeight)
        contentWidth:   mainColumnLayout.width
        contentHeight:  mainColumnLayout.height

        Rectangle {
            width:  titleRowLayout.width
            height: titleRowLayout.height
            color:  qgcPal.windowShade
        }

        ColumnLayout {
            id:         mainColumnLayout
            spacing:    _contentMargin

            RowLayout {
                id:                 titleRowLayout
                Layout.fillWidth:   true

                QGCLabel {
                    Layout.leftMargin:  ScreenTools.defaultFontPixelWidth
                    Layout.fillWidth:   true
                    text:               title
                    height:             parent.height
                    verticalAlignment:	Text.AlignVCenter
                }

                QGCButton {
                    id:         rejectButton
                    onClicked:  dialogComponentLoader.item.reject()
                }

                QGCButton {
                    id:         acceptButton
                    primary:    true
                    onClicked:  dialogComponentLoader.item.accept()
                }
            }

            Item {
                id:                     item
                Layout.preferredWidth:  dialogComponentLoader.width + (_contentMargin * 2)
                Layout.preferredHeight: dialogComponentLoader.height + _contentMargin

                Loader {
                    id:                 dialogComponentLoader
                    x:                  _contentMargin
                    sourceComponent:    dialogComponent
                    focus:              true

                    property var dialogProperties:  popupRoot.dialogProperties
                    property bool acceptAllowed:    acceptButton.visible
                    property bool rejectAllowed:    rejectButton.visible
                }
            }
        }
    }
}
