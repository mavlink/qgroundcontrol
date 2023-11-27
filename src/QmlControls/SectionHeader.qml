import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.ScreenTools
import QGroundControl.Palette

FocusScope {
    id:     _root
    height: column.height

    property alias          color:          label.color
    property alias          text:           label.text
    property bool           checked:        true
    property bool           showSpacer:     true
    property ButtonGroup    buttonGroup:    null

    property real   _sectionSpacer: ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings

    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(_root)
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCMouseArea {
        anchors.fill: parent

        onClicked: {
            _root.focus = true
            checked = !checked
        }

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

                QGCColoredImage {
                    id:                     image
                    anchors.right:          parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width:                  label.height / 2
                    height:                 width
                    source:                 "/qmlimages/arrow-down.png"
                    color:                  qgcPal.text
                    visible:                !_root.checked
                }
            }

            Rectangle {
                Layout.fillWidth:   true
                height:             1
                color:              qgcPal.text
            }
        }
    }
}
