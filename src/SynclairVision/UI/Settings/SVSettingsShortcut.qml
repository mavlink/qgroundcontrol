import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id: root

    property string labelText: ''
    property string targetPropertyName: ''
    property int shortcutValue: 0
    readonly property int wheelUpShortcutValue: -1001
    readonly property int wheelDownShortcutValue: -1002
    readonly property int mouseButtonShortcutBaseValue: -2000
    property real labelColumnWidth: ScreenTools.defaultFontPixelWidth * 18
    property real controlColumnWidth: ScreenTools.defaultFontPixelWidth * 25
    property real columnSpacing: ScreenTools.defaultFontPixelWidth * 1.5

    spacing: columnSpacing

    function keyToString(key) {
        if (key === undefined || key === 0) {
            return qsTr('Not Set')
        }

        if (key < root.mouseButtonShortcutBaseValue) {
            const button = root.shortcutToMouseButton(key)

            switch (button) {
            case Qt.LeftButton: return qsTr('Mouse Left Button')
            case Qt.RightButton: return qsTr('Mouse Right Button')
            case Qt.MiddleButton: return qsTr('Mouse Middle Button')
            case Qt.BackButton: return qsTr('Mouse Back Button')
            case Qt.ForwardButton: return qsTr('Mouse Forward Button')

            default:
                return qsTr('Mouse Button %1').arg(button)
            }
        }

        if (key >= Qt.Key_A && key <= Qt.Key_Z) {
            return String.fromCharCode(key)
        }

        if (key >= Qt.Key_0 && key <= Qt.Key_9) {
            return String.fromCharCode(key)
        }

        if (key >= Qt.Key_F1 && key <= Qt.Key_F35) {
            return 'F' + (key - Qt.Key_F1 + 1)
        }

        switch (key) {
        case root.wheelUpShortcutValue: return qsTr('Wheel Up')
        case root.wheelDownShortcutValue: return qsTr('Wheel Down')
        case Qt.Key_Up: return 'Up'
        case Qt.Key_Down: return 'Down'
        case Qt.Key_Left: return 'Left'
        case Qt.Key_Right: return 'Right'
        case Qt.Key_Space: return 'Space'
        case Qt.Key_Return: return 'Enter'
        case Qt.Key_Enter: return 'Enter'
        case Qt.Key_Tab: return 'Tab'
        case Qt.Key_Backspace: return 'Backspace'
        case Qt.Key_Delete: return 'Delete'
        case Qt.Key_Escape: return 'Escape'
        case Qt.Key_Shift: return 'Shift'
        case Qt.Key_Control: return 'Ctrl'
        case Qt.Key_Alt: return 'Alt'
        case Qt.Key_Meta: return 'Meta'
        case Qt.Key_Home: return 'Home'
        case Qt.Key_End: return 'End'
        case Qt.Key_PageUp: return 'Page Up'
        case Qt.Key_PageDown: return 'Page Down'
        case Qt.Key_Plus: return '+'
        case Qt.Key_Minus: return '-'
        case Qt.Key_Period: return '.'
        case Qt.Key_Comma: return ','

        default:
            return qsTr('Key %1').arg(key)
        }
    }

    function mouseButtonToShortcut(button) {
        return root.mouseButtonShortcutBaseValue - button
    }

    function shortcutToMouseButton(shortcut) {
        return root.mouseButtonShortcutBaseValue - shortcut
    }

    function isAssignableMouseButton(button) {
        switch (button) {
        case Qt.MiddleButton:
        case Qt.BackButton:
        case Qt.ForwardButton:
            return true

        default:
            return false
        }
    }

    function assignWheelShortcut(angleDeltaY) {
        if (angleDeltaY === 0) {
            return false
        }

        root.setShortcutValue(angleDeltaY > 0 ? root.wheelUpShortcutValue : root.wheelDownShortcutValue)

        return true
    }

    function setShortcutValue(key) {
        if (targetPropertyName === '' || SVSettings[targetPropertyName] === undefined) {
            return
        }

        SVSettings[targetPropertyName] = key
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredWidth: root.labelColumnWidth
        spacing: 0

        QGCLabel {
            Layout.fillWidth: true
            text: root.labelText
            wrapMode: Text.WordWrap
        }
    }

    QGCButton {
        Layout.alignment: Qt.AlignTop | Qt.AlignRight
        Layout.preferredWidth: root.controlColumnWidth
        enabled: root.targetPropertyName !== '' && SVSettings[root.targetPropertyName] !== undefined
        text: root.keyToString(root.shortcutValue)

        onClicked: shortcutDialogFactory.open()
    }

    QGCPopupDialogFactory {
        id: shortcutDialogFactory
        dialogComponent: shortcutDialogComponent
    }

    Component {
        id: shortcutDialogComponent

        QGCPopupDialog {
            id: shortcutDialog
            title: root.labelText === '' ? qsTr('Set Shortcut') : root.labelText
            buttons: Dialog.Cancel

            Item {
                id: captureArea
                width: Math.max(root.controlColumnWidth, 300)
                implicitWidth: width
                height: contentLayout.implicitHeight
                implicitHeight: height

                ColumnLayout {
                    id: contentLayout
                    anchors.fill: parent
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCLabel {
                        text: root.labelText === ''
                            ? qsTr('Press any key, click a mouse button, or scroll to assign a shortcut.')
                            : qsTr('Press any key, click a mouse button, or scroll to assign %1.').arg(root.labelText)
                        wrapMode: Text.WordWrap
                    }

                    QGCLabel {
                        text: qsTr('Press Cancel or Escape to keep the current shortcut.')
                        wrapMode: Text.WordWrap
                        opacity: 0.7
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: promptLabel.implicitHeight + ScreenTools.defaultFontPixelHeight
                        color: QGroundControl.globalPalette.windowShade
                        radius: ScreenTools.defaultFontPixelHeight / 4

                        QGCLabel {
                            id: promptLabel
                            anchors.left: parent.left
                            anchors.leftMargin: SVUnits.bigMargin
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr('Waiting for key press, mouse click, or scroll...')
                        }
                    }
                }

                Item {
                    id: keyCapture
                    anchors.fill: parent
                    focus: true

                    Keys.onPressed: (event) => {
                        if (event.isAutoRepeat) {
                            event.accepted = true
                            return
                        }

                        if (event.key === Qt.Key_Escape) {
                            event.accepted = true
                            shortcutDialog.close()
                            return
                        }

                        root.setShortcutValue(event.key)
                        event.accepted = true
                        shortcutDialog.close()
                    }

                    Component.onCompleted: forceActiveFocus()
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons

                    onWheel: (wheel) => {
                        wheel.accepted = true

                        if (!root.assignWheelShortcut(wheel.angleDelta.y)) {
                            return
                        }

                        shortcutDialog.close()
                    }

                    onPressed: (mouse) => {
                        mouse.accepted = true

                        if (!root.isAssignableMouseButton(mouse.button)) {
                            return
                        }

                        root.setShortcutValue(root.mouseButtonToShortcut(mouse.button))
                        shortcutDialog.close()
                    }
                }
            }
        }
    }
}
