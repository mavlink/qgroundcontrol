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

// Provides the standard dialog mechanism for QGC. Works 99% like Qml Dialog.
//
// Example usage:
//      Component {
//          id: dialogComponent
//
//          QGCPopupDialog {
//              ...
//          }
//      }
//
//      onFoo: dialogComponent.createObject(mainWindow).open()
//
// Notes:
//  * QGCPopupDialog should be created from a component to limit the memory usage of the dialog
//      to only when it is displayed.
//  * Parent for createObject should always be mainWindow.
// Differences from standard Qml Dialog:
//  * The QGCPopupDialog object will automatically be destroyed when it closed. You can override this
//      behaviour by setting destroyOnClose to false if it was not created dynamically.
//  * Dialog will automatically close after accepted/rejected signal processing. You can prevent this by setting
//      preventClose = true prior to returning from your signal handlers.
Popup {
    id:                 _root
    parent:             Overlay.overlay
    anchors.centerIn:   parent
    width:              mainColumnLayout.width + (padding * 2)
    height:             mainColumnLayout.y + mainColumnLayout.height + padding
    padding:            2
    modal:              true
    focus:              true

    property string title
    property var    buttons:                StandardButton.Ok
    property bool   acceptAllowed:          acceptButton.visible
    property bool   rejectAllowed:          rejectButton.visible
    property alias  acceptButtonEnabled:    acceptButton.enabled
    property alias  rejectButtonEnabled:    rejectButton.enabled
    property var    dialogProperties
    property bool   destroyOnClose:         true
    property bool   preventClose:           false

    signal accepted
    signal rejected

    property var    _pal:               QGroundControl.globalPalette
    property real   _frameSize:         ScreenTools.defaultFontPixelWidth
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
        // The last child item will be the true dialog content.
        // Re-Parent it to the right place in the ui hierarchy.
        contentChildren[contentChildren.length - 1].parent = dialogContentParent
    }

    onAboutToShow: setupDialogButtons(buttons)
    onClosed: {
        Qt.inputMethod.hide()
        if (destroyOnClose) {
            _root.destroy()
        }
    }

    function _accept() {
        if (acceptAllowed) {
            accepted()
            if (preventClose) {
                preventClose = false
            } else {
                close()
            }
        }
    }

    function _reject() {
        if (rejectAllowed) {
            rejected()
            if (preventClose) {
                preventClose = false
            } else {
                close()
            }
        }
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
                text:               _root.title
                font.pointSize:     ScreenTools.mediumFontPointSize
                verticalAlignment:	Text.AlignVCenter
            }

            QGCButton {
                id:         rejectButton
                onClicked:  _reject()
            }

            QGCButton {
                id:         acceptButton
                primary:    true
                onClicked:  _accept()
            }
        }

        QGCFlickable {
            id:                     mainFlickable
            Layout.preferredWidth:  Math.min(Math.max(marginItem.width, mainColumnLayout.width), _maxContentWidth)
            Layout.preferredHeight: Math.min(marginItem.height, _maxContentHeight)
            contentWidth:           marginItem.width
            contentHeight:          marginItem.height

            Item {
                id:     marginItem
                width:  dialogContentParent.width + (_contentMargin * 2)
                height: dialogContentParent.height + _contentMargin

                Item {
                    id:     dialogContentParent
                    x:      _contentMargin
                    width:  childrenRect.width
                    height: childrenRect.height
                    focus:  true

                    Keys.onReleased: {
                        if (event.key === Qt.Key_Escape) {
                            _reject()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            _accept()
                            event.accepted = true
                        }
                    }

                }
            }
        }
    }
}
