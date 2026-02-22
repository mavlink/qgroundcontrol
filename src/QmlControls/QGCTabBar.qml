import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// We implement our own TabBar to get around the fact that QtQuick.Controls TabBar does not
// support hiding tabs. This version supports hiding tabs by setting the visible property
// on the QGCTabButton instances.
RowLayout {
    property int currentIndex: 0

    id: control
    spacing: 0

    property bool _preventCurrentIndexBindingLoop: false
    property var _buttons: []

    function _updateButtons() {
        _buttons = []
        for (var i = 0; i < control.children.length; i++) {
            if (control.children[i].hasOwnProperty("checkable")) {
                _buttons.push(control.children[i])
            }
        }
        buttonGroup.buttons = _buttons
        _selectCurrentIndexButton()
    }

    function _selectCurrentIndexButton() {
        if (_buttons.length === 0) return

        _preventCurrentIndexBindingLoop = true

        if (control.currentIndex === -1) {
            for (var i = 0; i < _buttons.length; i++) {
                _buttons[i].checked = false
            }
        } else {
            let index = Math.min(Math.max(control.currentIndex, 0), _buttons.length - 1)
            if (_buttons[index].visible) {
                _buttons[index].checked = true
            } else {
                for (var j = 0; j < _buttons.length; j++) {
                    if (_buttons[j].visible) {
                        _buttons[j].checked = true
                        break
                    }
                }
            }
        }

        _preventCurrentIndexBindingLoop = false
    }

    Component.onCompleted: _updateButtons()
    onChildrenChanged: _updateButtons()
    onCurrentIndexChanged: _selectCurrentIndexButton()
    onVisibleChildrenChanged: _selectCurrentIndexButton()

    ButtonGroup {
        id: buttonGroup

        onCheckedButtonChanged: {
            if (control._preventCurrentIndexBindingLoop) return

            for (var i = 0; i < control._buttons.length; i++) {
                if (control._buttons[i] === checkedButton) {
                    control.currentIndex = i
                    break
                }
            }
        }
    }
}
