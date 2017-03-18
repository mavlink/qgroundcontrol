import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

QGCMouseArea {
    id:             _root
    anchors.left:   parent.left
    anchors.right:  parent.right
    height:         column.height
    onClicked:      checked = !checked

    property alias          text:           label.text
    property bool           checked:        true
    property bool           showSpacer:     true
    property ExclusiveGroup exclusiveGroup: null

    property real   _sectionSpacer: ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings

    onExclusiveGroupChanged: {
        if (exclusiveGroup)
            exclusiveGroup.bindCheckable(_root)
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        id:             column
        anchors.left:   parent.left
        anchors.right:  parent.right

        Item {
            height:     _sectionSpacer
            width:      1
            visible:    showSpacer
        }

        QGCLabel {
            id:                 label
            Layout.fillWidth:   true

            Image {
                anchors.right:          parent.right
                anchors.verticalCenter: parent.verticalCenter
                source:                 "/qmlimages/arrow-down.png"
                visible:                !_root.checked
            }
        }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }
    }
}
