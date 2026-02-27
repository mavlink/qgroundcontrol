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

    function _selectCurrentIndexButton() {
        if (control.children.length > 0) {
            _preventCurrentIndexBindingLoop = true
            if (control.currentIndex == -1) {
                // No tab selected, select none
                for (var i = 0; i < control.children.length; i++) {
                    control.children[i].checked = false
                }
            } else {
                let filteredCurrentIndex = Math.min(Math.max(control.currentIndex, 0), control.children.length - 1)
                if (control.children[filteredCurrentIndex].visible) {
                    control.children[filteredCurrentIndex].checked = true
                } else {
                    // We select the first visible tab if the current index is not visible
                    for (var j = 0; j < control.children.length; j++) {
                        if (control.children[j].visible) {
                            control.children[j].checked = true
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

    ButtonGroup {
        buttons: control.children

        onCheckedButtonChanged: {
            for (var i = 0; i < control.children.length; i++) {
                if (control.children[i] === checkedButton) {
                    control.currentIndex = i
                    break
                }
            }
        }
    }
}
