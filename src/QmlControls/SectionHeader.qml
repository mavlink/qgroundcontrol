import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.ScreenTools
import QGroundControl.Palette

CheckBox {
    id:         control
    focusPolicy: Qt.ClickFocus
    checked:    true

    property var            color:          qgcPal.text
    property bool           showSpacer:     true
    property ButtonGroup    buttonGroup:    null

    property real _sectionSpacer: ScreenTools.defaultFontPixelWidth / 2  // spacing between section headings

    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(control)
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    contentItem: ColumnLayout {
        Item {
            Layout.preferredHeight: control._sectionSpacer
            width:                  1
            visible:                control.showSpacer
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text:               control.text
                color:              control.color
                Layout.fillWidth:   true
            }

            QGCColoredImage {
                id: arrowIcon
                anchors.verticalCenter: parent.verticalCenter
                width:                  ScreenTools.defaultFontPixelHeight / 2
                height:                 width
                source:                 "/qmlimages/arrow-down.png"
                color:                  qgcPal.text
                rotation:               control.checked ? 0 : -90

                smooth: false  // Prevents unwanted blurring
                antialiasing: false  // Ensures sharp edges
                layer.enabled: true  // Helps avoid transparency artifacts
                layer.smooth: false
                layer.textureSize: Qt.size(width, height)

                Behavior on rotation {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth:   true
            height:             1
            color:              qgcPal.text
        }
    }

    indicator: Item {}
} 
