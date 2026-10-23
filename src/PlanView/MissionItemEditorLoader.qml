import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Loads the editor QML of a mission item. The editors rely on some sizing properties of the enclosing item for their
/// layout, which this Loader provides so every host gets the same editor behavior.
Loader {
    id: root

    required property var missionItem   ///< null unloads the editor
    property real availableWidth:       0

    readonly property real  _margin:        ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _innerMargin:   2
    readonly property real  _radius:        ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _editFieldWidth: Math.min(availableWidth - _innerMargin * 2, ScreenTools.defaultFontPixelWidth * 12)

    function _load() {
        if (missionItem) {
            setSource(missionItem.editorQml, {
                missionItem:    root.missionItem,
                availableWidth: Qt.binding(() => root.availableWidth)
            })
        } else {
            setSource("")
        }
    }

    onMissionItemChanged: _load()

    // Deliberately not asynchronous. An editor destroyed while still loading warns "Cannot create a component in an
    // invalid context", which synchronous loading avoids.
    Component.onCompleted: _load()
}
