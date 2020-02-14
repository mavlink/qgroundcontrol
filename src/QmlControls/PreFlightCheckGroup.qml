/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick      2.3
import QtQml.Models 2.1

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// A PreFlightCheckGroup manages a set of PreFlightCheckButtons as a single entity.
Column  {
    property string name
    property bool   passed: false
    property bool   failed: false

    spacing: ScreenTools.defaultFontPixelHeight / 2

    property alias _checked: header.checked

    onPassedChanged: parent.groupPassedChanged(ObjectModel.index, passed)

    Component.onCompleted: {
        enabled = _checked
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
        id:             header
        anchors.left:   parent.left
        anchors.right:  parent.right
        text:           name + (passed ? qsTr(" (passed)") : "")
        color:          failed ? qgcPal.statusFailedText : (passed ? qgcPal.statusPassedText : qgcPal.statusPendingText)
    }

    Column {
        id:         innerColumn
        spacing:    ScreenTools.defaultFontPixelHeight / 2
        visible:    header.checked

        function buttonPassedChanged() {
            for (var i=0; i<children.length; i++) {
                if (!children[i].passed) {
                    passed = false
                    failed = children[i].failed
                    return
                }
            }
            failed = false
            passed = true
        }
    }
}
