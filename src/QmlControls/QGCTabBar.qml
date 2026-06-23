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
        let btns = []
        for (var i = 0; i < control.children.length; i++) {
            if (control.children[i].hasOwnProperty("checkable")) {
                btns.push(control.children[i])
            }
        }
        _buttons = btns
        buttonGroup.buttons = _buttons
        _selectCurrentIndexButton()
    }

    function _updateSeparators() {
        for (var i = 0; i < _buttons.length; i++) {
            if (!_buttons[i].visible || _buttons[i].checked) {
                _buttons[i]._showSeparator = false
                continue
            }
            // Find the next visible button
            var nextVisible = null
            for (var j = i + 1; j < _buttons.length; j++) {
                if (_buttons[j].visible) {
                    nextVisible = _buttons[j]
                    break
                }
            }
            _buttons[i]._showSeparator = nextVisible !== null && !nextVisible.checked
        }
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
                // Select the first visible tab if the current index is not visible
                index = -1
                for (var j = 0; j < _buttons.length; j++) {
                    if (_buttons[j].visible) {
                        _buttons[j].checked = true
                        index = j
                        break
                    }
                }
            }
            // Sync currentIndex to the actually-selected button so it never remains stale
            if (control.currentIndex !== index) {
                control.currentIndex = index
            }
        }

        _preventCurrentIndexBindingLoop = false
        _updateSeparators()
    }

    Component.onCompleted: _updateButtons()
    onChildrenChanged: _updateButtons()
    onCurrentIndexChanged: _selectCurrentIndexButton()
    onVisibleChildrenChanged: _selectCurrentIndexButton()

    onVisibleChanged: {
        if (visible) {
            // When becoming visible, ensure the current index is valid and a button is selected
            currentIndex = 0
            _selectCurrentIndexButton()
        }
    }


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
