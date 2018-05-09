/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

FocusScope {
    id:     _root
    z:      5000
    focus:  true

    property alias  dialogWidth:     _dialogPanel.width
    property alias  dialogTitle:     titleLabel.text
    property alias  dialogComponent: _dialogComponentLoader.sourceComponent
    property var    viewPanel

    property real _defaultTextHeight:   _textMeasure.contentHeight
    property real _defaultTextWidth:    _textMeasure.contentWidth

    function setupDialogButtons(buttons) {
        _acceptButton.visible = false
        _rejectButton.visible = false

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
            viewPanel.enabled = true
            _root.destroy()
        }
    }

    QGCPalette { id: _qgcPal; colorGroupEnabled: true }
    QGCLabel { id: _textMeasure; text: "X"; visible: false }

    Rectangle {
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  _dialogPanel.left
        opacity:        0.5
        color:          _qgcPal.window
        z:              5000
    }

    // This is the main dialog panel which is anchored to the right edge
    Rectangle {
        id:                 _dialogPanel
        height:             ScreenTools.availableHeight ? ScreenTools.availableHeight : parent.height
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        color:              _qgcPal.windowShadeDark

        Rectangle {
            id:     _header
            width:  parent.width
            height: _acceptButton.visible ? _acceptButton.height : _rejectButton.height
            color:  _qgcPal.windowShade

            function _hidePanel() {
                _fullPanel.visible = false
            }

            QGCLabel {
                id:                 titleLabel
                x:                  _defaultTextWidth
                height:             parent.height
                verticalAlignment:	Text.AlignVCenter
            }

            QGCButton {
                id:             _rejectButton
                anchors.right:  _acceptButton.visible ?  _acceptButton.left : parent.right
                anchors.bottom: parent.bottom

                onClicked: {
                    enabled = false // prevent multiple clicks
                    _dialogComponentLoader.item.reject()
                    if (!viewPanel.enabled) {
                        // Dialog was not closed, re-enable button
                        enabled = true
                    }
                }
            }

            QGCButton {
                id:             _acceptButton
                anchors.right:  parent.right
                primary:        true

                onClicked: {
                    enabled = false // prevent multiple clicks
                    _dialogComponentLoader.item.accept()
                    if (!viewPanel.enabled) {
                        // Dialog was not closed, re-enable button
                        enabled = true
                    }
                }
            }
        }

        Item {
            id:             _spacer
            width:          10
            height:         10
            anchors.top:    _header.bottom
        }

        Loader {
            id:                 _dialogComponentLoader
            anchors.margins:    5
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        _spacer.bottom
            anchors.bottom:     parent.bottom
            sourceComponent:    _dialogComponent
            focus:              true

            property bool acceptAllowed: _acceptButton.visible
            property bool rejectAllowed: _rejectButton.visible
        }
    } // Rectangle - Dialog panel
}
