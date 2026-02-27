import QtQuick
import QtQml.Models

ObjectModel {
    id: _root
    property bool enforceOrder: true

    function reset() {
        for (var i=0; i<_root.count; i++) {
            var group = _root.get(i)
            group.reset()
            if (enforceOrder) {
                group.enabled = i === 0
            } else {
                group.enabled = true
            }
            group._checked = i === 0
        }
    }

    function isPassed() {
        for (var i = 0; i < _root.count; i++) {
            var group = _root.get(i)
            if(!group.passed) {
                return false
            }
        }
        return true
    }

    Component.onCompleted: reset()

}
