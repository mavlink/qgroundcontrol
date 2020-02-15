/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    anchors.fill:   parent

    property real   _defaultTextHeight: _textMeasure.contentHeight
    property real   _defaultTextWidth:  _textMeasure.contentWidth

    function setupDialogButtons() {
        _acceptButton.visible = false
        _rejectButton.visible = false
        var buttons = mainWindowDialog.dialogButtons
        // Accept role buttons
        if (buttons & StandardButton.Ok) {
            _acceptButton.text = qsTr("Ok")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            _acceptButton.text = qsTr("Open")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Save) {
            _acceptButton.text = qsTr("Save")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Apply) {
            _acceptButton.text = qsTr("Apply")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Open) {
            _acceptButton.text = qsTr("Open")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.SaveAll) {
            _acceptButton.text = qsTr("Save All")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Yes) {
            _acceptButton.text = qsTr("Yes")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.YesToAll) {
            _acceptButton.text = qsTr("Yes to All")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Retry) {
            _acceptButton.text = qsTr("Retry")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Reset) {
            _acceptButton.text = qsTr("Reset")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.RestoreToDefaults) {
            _acceptButton.text = qsTr("Restore to Defaults")
            _acceptButton.visible = true
        } else if (buttons & StandardButton.Ignore) {
            _acceptButton.text = qsTr("Ignore")
            _acceptButton.visible = true
        }

        // Reject role buttons
        if (buttons & StandardButton.Cancel) {
            _rejectButton.text = qsTr("Cancel")
            _rejectButton.visible = true
        } else if (buttons & StandardButton.Close) {
            _rejectButton.text = qsTr("Close")
            _rejectButton.visible = true
        } else if (buttons & StandardButton.No) {
            _rejectButton.text = qsTr("No")
            _rejectButton.visible = true
        } else if (buttons & StandardButton.NoToAll) {
            _rejectButton.text = qsTr("No to All")
            _rejectButton.visible = true
        } else if (buttons & StandardButton.Abort) {
            _rejectButton.text = qsTr("Abort")
            _rejectButton.visible = true
        }
    }

    Connections {
        target: _dialogComponentLoader.item
        onHideDialog: {
            mainWindowDialog.close()
        }
    }

    QGCLabel { id: _textMeasure; text: "X"; visible: false }

    // This is the main dialog panel
    Item {
        id:                 _dialogPanel
        anchors.fill:       parent
        Rectangle {
            id:             _header
            width:          parent.width
            height:         _acceptButton.visible ? _acceptButton.height : _rejectButton.height
            color:          qgcPal.windowShade
            QGCLabel {
                id:                 titleLabel
                x:                  _defaultTextWidth
                text:               mainWindowDialog.dialogTitle
                height:             parent.height
                verticalAlignment:	Text.AlignVCenter
            }
            QGCButton {
                id:                 _rejectButton
                anchors.right:      _acceptButton.visible ?  _acceptButton.left : parent.right
                anchors.bottom:     parent.bottom
                onClicked: {
                    _dialogComponentLoader.item.reject()
                    mainWindowDialog.close()
                }
            }
            QGCButton {
                id:                 _acceptButton
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                primary:            true
                onClicked: {
                    _dialogComponentLoader.item.accept()
                    mainWindowDialog.close()
                }
            }
        }
        Item {
            id:                     _spacer
            width:                  10
            height:                 10
            anchors.top:            _header.bottom
        }
        Loader {
            id:                 _dialogComponentLoader
            anchors.margins:    5
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        _spacer.bottom
            anchors.bottom:     parent.bottom
            sourceComponent:    mainWindowDialog.dialogComponent
            focus:              true
            property bool acceptAllowed: _acceptButton.visible
            property bool rejectAllowed: _rejectButton.visible
        }
    }
}
