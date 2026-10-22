import QtQuick
import QtQml.Models

ObjectModel {
    id: _root
    property bool enforceOrder: true    ///< true: a group stays locked until every group above it has passed
    property bool prerequisitesPassed: true ///< false: locks every group until checks outside this model have passed

    onPrerequisitesPassedChanged: updateGroupLocks()

    function reset() {
        for (var i=0; i<_root.count; i++) {
            _root.get(i).reset()
        }
        updateGroupLocks()
    }

    function updateGroupLocks() {
        let previousGroupsPassed = true
        for (let i = 0; i < _root.count; i++) {
            const group = _root.get(i)
            group.enabled = prerequisitesPassed && (!enforceOrder || previousGroupsPassed)
            previousGroupsPassed = previousGroupsPassed && group.passed
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
