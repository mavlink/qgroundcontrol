import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// A PreFlightCheckGroup manages a set of PreFlightCheckButtons as a single entity. Groups are always expanded;
/// PreFlightCheckList disables (locks) a group until every group above it has passed.
Column  {
    property string name
    property bool   passed: false
    readonly property bool failed: failedCount > 0
    readonly property Item headerItem: groupHeader

    readonly property int failedCount: {
        let count = 0
        for (const button of innerColumn.children) {
            if (button.failed) {
                count++
            }
        }
        return count
    }

    spacing: ScreenTools.defaultFontPixelHeight / 2

    Layout.fillWidth:       true
    Layout.preferredWidth:  0
    Layout.minimumWidth:    0

    onPassedChanged: parent.groupPassedChanged()

    Component.onCompleted: {
        var moveList = []
        var i = 0
        for (i = 2; i < children.length; i++) {
            moveList.push(children[i])
        }
        for (i = 0; i < moveList.length; i++) {
            moveList[i].parent = innerColumn
        }
    }

    function reset() {
        for (var i=0; i<innerColumn.children.length; i++) {
            innerColumn.children[i].reset()
        }
    }

    SectionHeader {
        id:             groupHeader
        anchors.left:   parent.left
        anchors.right:  parent.right
        checkable:      false
        text:           name + (passed ? qsTr(" (passed)") : (enabled ? "" : qsTr(" (locked)")))
        color:          !enabled ? groupPalette.text :
                            (failed ? groupPalette.errorText : (passed ? groupPalette.successText : groupPalette.warningText))
    }

    QGCPalette { id: groupPalette; colorGroupEnabled: enabled }

    Column {
        id:         innerColumn
        width:      parent.width
        spacing:    ScreenTools.defaultFontPixelHeight / 2

        function buttonPassedChanged() {
            for (var i=0; i<children.length; i++) {
                if (!children[i].passed) {
                    passed = false
                    return
                }
            }
            passed = true
        }
    }
}
