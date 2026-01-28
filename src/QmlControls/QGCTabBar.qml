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

    function _buttons() {
        var result = []
        for (var i = 0; i < control.children.length; i++) {
            if (control.children[i] instanceof AbstractButton)
                result.push(control.children[i])
        }
        return result
    }

    function _selectCurrentIndexButton() {
        var btns = _buttons()
        if (btns.length > 0) {
            _preventCurrentIndexBindingLoop = true
            if (control.currentIndex == -1) {
                // No tab selected, select none
                for (var i = 0; i < btns.length; i++) {
                    btns[i].checked = false
                }
            } else {
                let filteredCurrentIndex = Math.min(Math.max(control.currentIndex, 0), btns.length - 1)
                if (btns[filteredCurrentIndex].visible) {
                    btns[filteredCurrentIndex].checked = true
                } else {
                    // We select the first visible tab if the current index is not visible
                    for (var j = 0; j < btns.length; j++) {
                        if (btns[j].visible) {
                            btns[j].checked = true
                            break
                        }
                    }
                }
            }
            _preventCurrentIndexBindingLoop = false
        }
    }

    Component.onCompleted: _selectCurrentIndexButton()
    onCurrentIndexChanged: _selectCurrentIndexButton()
    onVisibleChildrenChanged: _selectCurrentIndexButton()
    onChildrenChanged: tabButtonGroup.buttons = _buttons()

    ButtonGroup {
        id: tabButtonGroup

        onCheckedButtonChanged: {
            var btns = control._buttons()
            for (var i = 0; i < btns.length; i++) {
                if (btns[i] === checkedButton) {
                    control.currentIndex = i
                    break
                }
            }
        }
    }
}
