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
    property var    dialogComponent
    property string dialogSource
    property var    dialogProperties

    id:                 _root
    anchors.centerIn:   parent
    width:              mainColumnLayout.width + (padding * 2)
    height:             mainColumnLayout.y + mainColumnLayout.height + padding
    padding:            2
    modal:              true
    focus:              true

    property var    _pal:               QGroundControl.globalPalette
    property real   _frameSize:         ScreenTools.defaultFontPixelWidth
    property string _dialogTitle
    property real   _contentMargin:     ScreenTools.defaultFontPixelHeight / 2
    property real   _popupDoubleInset:  ScreenTools.defaultFontPixelHeight * 2
    property real   _maxContentWidth:   parent.width - _popupDoubleInset
    property real   _maxContentHeight:  parent.height - titleRowLayout.height - _popupDoubleInset

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
            anchors.margins:    _root.padding
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

    function disableAcceptButton() {
        acceptButton.enabled = false
    }

    Connections {
        target:                 dialogComponentLoader.item
        onHideDialog:           close()
        onEnableAcceptButton:   acceptButton.enabled = true
        onEnableRejectButton:   rejectButton.enabled = true
        onDisableAcceptButton:  acceptButton.enabled = false
        onDisableRejectButton:  rejectButton.enabled = false
    }

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
                text:               _dialogTitle
                font.pointSize:     ScreenTools.mediumFontPointSize
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

        QGCFlickable {
            id:                     mainFlickable
            Layout.preferredWidth:  Math.min(marginItem.width, _maxContentWidth)
            Layout.preferredHeight: Math.min(marginItem.height, _maxContentHeight)
            contentWidth:           marginItem.width
            contentHeight:          marginItem.height

            Item {
                id:     marginItem
                width:  dialogComponentLoader.width + (_contentMargin * 2)
                height: dialogComponentLoader.height + _contentMargin

                Loader {
                    id:                 dialogComponentLoader
                    x:                  _contentMargin
                    source:             dialogSource
                    sourceComponent:    dialogComponent
                    focus:              true

                    property var dialogProperties:  _root.dialogProperties
                    property bool acceptAllowed:    acceptButton.visible
                    property bool rejectAllowed:    rejectButton.visible
                }
            }
        }
    }
}
